// indirect and polymorphic assignment with unequal allocators, propagation, valueless
// operands and throwing copies/moves, checked against a ledger that records which allocator
// object allocated each block (a block must be deallocated by an allocator equal to the one
// that allocated it, [allocator.requirements.general]: a.deallocate(p, n) requires p from an
// allocator comparing equal to a) and which constructs/destroys went through it.
// [indirect.assign]/2: (2.1) the allocator needs updating if POCCA; (2.2) other valueless:
// *this becomes valueless, its owned object destroyed with allocator_traits::destroy and the
// storage deallocated; (2.3) equal allocators and *this not valueless: **this = *other;
// (2.4) otherwise a new owned object is constructed with allocator_traits::construct "using
// either the allocator in *this or the allocator in other if the allocator needs updating";
// (2.5) the previous object is destroyed and its storage deallocated; (2.6) then the
// allocator is replaced. /4: if T's copy constructor throws, no effect (and
// valueless_after_move() is unchanged). /6-7, /9: move assignment: (6.3) takes ownership if
// the allocator needs updating or the allocators are equal, (6.4) otherwise constructs from
// the rvalue with *this's allocator; other is valueless afterwards; if an exception is
// thrown, no effects on *this or other. /11: operator=(U&&) on a valueless *this constructs
// with alloc. [indirect.general]/3: construction and destruction via allocator_traits.
// [polymorphic.assign]/2-8: the same for polymorphic, constructing an object of the dynamic
// type U of other's owned object; /4: no effects on *this if an exception is thrown; a
// valueless other leaves *this valueless (nothing is constructed, the previous object is
// destroyed).
// REQUIRES: exceptions
#include <cstddef>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Ledger {
  std::map<void*, int> live;  // block -> id of the allocating allocator
  int bad = 0;                // deallocations by a different allocator, or of unknown blocks
  int constructs[10] = {}, destroys[10] = {};
};
Ledger ledger;

template <class T, bool Copy, bool Move>
struct LAlloc {
  using value_type = T;
  using propagate_on_container_copy_assignment = std::bool_constant<Copy>;
  using propagate_on_container_move_assignment = std::bool_constant<Move>;
  using propagate_on_container_swap = std::false_type;
  using is_always_equal = std::false_type;
  template <class U>
  struct rebind {
    using other = LAlloc<U, Copy, Move>;
  };
  int id = 0;
  LAlloc() = default;
  explicit LAlloc(int i) : id(i) {}
  template <class U>
  LAlloc(const LAlloc<U, Copy, Move>& o) noexcept : id(o.id) {}
  T* allocate(std::size_t n) {
    T* p = std::allocator<T>{}.allocate(n);
    ledger.live[p] = id;
    return p;
  }
  void deallocate(T* p, std::size_t n) {
    auto it = ledger.live.find(p);
    if (it == ledger.live.end() || it->second != id)
      ++ledger.bad;
    else
      ledger.live.erase(it);
    std::allocator<T>{}.deallocate(p, n);
  }
  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
    ++ledger.constructs[id];
  }
  template <class U>
  void destroy(U* p) {
    ++ledger.destroys[id];
    p->~U();
  }
  template <class U>
  friend bool operator==(const LAlloc& a, const LAlloc<U, Copy, Move>& b) noexcept {
    return a.id == b.id;
  }
};

int copy_assigns = 0;
bool throw_on_copy = false, throw_on_move = false;
struct Val {
  std::string s;
  explicit Val(std::string v) : s(std::move(v)) {}
  Val(const Val& o) : s(o.s) {
    if (throw_on_copy) throw std::runtime_error("copy");
  }
  Val(Val&& o) : s(o.s) {
    if (throw_on_move) throw std::runtime_error("move");
  }
  Val& operator=(const Val& o) {
    ++copy_assigns;
    s = o.s;
    return *this;
  }
  Val& operator=(Val&&) = default;
};

bool clean() { return ledger.bad == 0; }

template <bool C, bool M>
using Ind = std::indirect<Val, LAlloc<Val, C, M>>;

void indirect_copy() {
  using A = LAlloc<Val, false, false>;
  using I = Ind<false, false>;
  {
    // (2.4)/(2.5) unequal, no POCCA: construct with *this's allocator (1), destroy the old one
    // with allocator 1 as well; the allocator is kept.
    I a(std::allocator_arg, A(1), "old"), b(std::allocator_arg, A(2), "new");
    const Val* old = &*a;
    a = b;
    CHECK(a->s == "new" && b->s == "new" && a.get_allocator().id == 1);
    CHECK(ledger.constructs[1] == 2 && ledger.destroys[1] == 1 && ledger.constructs[2] == 1);
    CHECK(&*a != old && clean());
    // /4: the copy constructor throws: no effect.
    I c(std::allocator_arg, A(3), "keep");
    const Val* pc = &*c;
    throw_on_copy = true;
    bool threw = false;
    try {
      c = b;
    } catch (const std::runtime_error&) {
      threw = true;
    }
    throw_on_copy = false;
    CHECK(threw && &*c == pc && c->s == "keep" && !c.valueless_after_move() && c.get_allocator().id == 3);
    CHECK(clean());
    // (2.3) equal allocators: T's copy assignment, no new object.
    I d(std::allocator_arg, A(2), "d");
    const Val* pd = &*d;
    copy_assigns = 0;
    d = b;
    CHECK(copy_assigns == 1 && &*d == pd && d->s == "new");
    // (2.2) other valueless: *this becomes valueless; its object destroyed by its allocator.
    I gone(std::allocator_arg, A(2), "x");
    I moved(std::move(gone));
    CHECK(gone.valueless_after_move());
    const int destroys4 = ledger.destroys[4];
    I e(std::allocator_arg, A(4), "e");
    e = gone;
    CHECK(e.valueless_after_move() && ledger.destroys[4] == destroys4 + 1 && clean());
    // (2.4) *this valueless, equal allocators: a new object is constructed (not assigned).
    copy_assigns = 0;
    e = d;  // allocator 4 vs 2: constructs with 4
    CHECK(!e.valueless_after_move() && e->s == "new" && copy_assigns == 0 && e.get_allocator().id == 4);
    I f(std::allocator_arg, A(2), "f");
    I tmp(std::move(f));
    copy_assigns = 0;
    f = d;  // equal allocators (2 and 2) but *this valueless: construct
    CHECK(!f.valueless_after_move() && f->s == "new" && copy_assigns == 0);
    // Self-assignment: no effect, even for a valueless object.
    I& fr = f;
    f = fr;
    CHECK(f->s == "new");
    I& gr = gone;
    gone = gr;
    CHECK(gone.valueless_after_move());
  }
  CHECK(clean() && ledger.live.empty());

  // POCCA: construct with other's allocator (2), destroy the old object with the old
  // allocator (1), then adopt allocator 2.
  {
    using AP = LAlloc<Val, true, false>;
    using IP = Ind<true, false>;
    IP a(std::allocator_arg, AP(1), "old"), b(std::allocator_arg, AP(2), "new");
    const int c2 = ledger.constructs[2], d1 = ledger.destroys[1];
    a = b;
    CHECK(a->s == "new" && a.get_allocator().id == 2);
    CHECK(ledger.constructs[2] == c2 + 1 && ledger.destroys[1] == d1 + 1);
    CHECK(clean());
  }
  CHECK(clean() && ledger.live.empty());
}

void indirect_move() {
  using A = LAlloc<Val, false, false>;
  using I = Ind<false, false>;
  {
    // (6.4): unequal, no POCMA: constructed from the rvalue with allocator 5; other valueless.
    I a(std::allocator_arg, A(5), "old"), b(std::allocator_arg, A(6), "new");
    const int c5 = ledger.constructs[5], d6 = ledger.destroys[6];
    a = std::move(b);
    CHECK(a->s == "new" && a.get_allocator().id == 5 && ledger.constructs[5] == c5 + 1);
    CHECK(b.valueless_after_move() && ledger.destroys[6] == d6 + 1 && clean());
    // /9: the move constructor throws: no effects on *this or other.
    I c(std::allocator_arg, A(5), "c"), d(std::allocator_arg, A(6), "d");
    const Val *pc = &*c, *pd = &*d;
    throw_on_move = true;
    bool threw = false;
    try {
      c = std::move(d);
    } catch (const std::runtime_error&) {
      threw = true;
    }
    throw_on_move = false;
    CHECK(threw && &*c == pc && c->s == "c" && &*d == pd && d->s == "d" && clean());
    // (6.3) equal allocators: ownership is taken, nothing constructed.
    I e(std::allocator_arg, A(6), "e");
    const Val* pe = &*e;
    const int c6 = ledger.constructs[6];
    d = std::move(e);
    CHECK(&*d == pe && e.valueless_after_move() && ledger.constructs[6] == c6 && clean());
    // (6.2) other valueless.
    c = std::move(e);
    CHECK(c.valueless_after_move() && e.valueless_after_move() && clean());
    // /11: operator=(U&&) on a valueless object constructs with alloc (5).
    const int c5b = ledger.constructs[5];
    c = Val("u");
    CHECK(!c.valueless_after_move() && c->s == "u" && ledger.constructs[5] == c5b + 1);
  }
  {
    // POCMA with unequal allocators: ownership taken, allocator adopted; the block is later
    // deallocated by the adopted allocator (7), which allocated it.
    using AP = LAlloc<Val, false, true>;
    Ind<false, true> a(std::allocator_arg, AP(6), "old"), b(std::allocator_arg, AP(7), "new");
    const Val* pb = &*b;
    a = std::move(b);
    CHECK(&*a == pb && a.get_allocator().id == 7 && b.valueless_after_move() && clean());
  }
  CHECK(clean() && ledger.live.empty());
}

struct Base {
  virtual ~Base() = default;
  virtual std::string name() const { return "base"; }
};
struct Derived : Base {
  std::string v;
  explicit Derived(std::string s) : v(std::move(s)) {}
  Derived(const Derived& o) : Base(o), v(o.v) {
    if (throw_on_copy) throw std::runtime_error("copy");
  }
  Derived(Derived&& o) : Base(o), v(o.v) {
    if (throw_on_move) throw std::runtime_error("move");
  }
  std::string name() const override { return "derived:" + v; }
};

void polymorphic() {
  using A = LAlloc<Base, false, false>;
  using P = std::polymorphic<Base, A>;
  {
    P a(std::allocator_arg, A(1)), b(std::allocator_arg, A(2), std::in_place_type<Derived>, "x");
    CHECK(a->name() == "base");
    a = b;  // a Derived constructed with allocator 1
    CHECK(a->name() == "derived:x" && b->name() == "derived:x" && a.get_allocator().id == 1);
    CHECK(clean());
    P c(std::allocator_arg, A(3), std::in_place_type<Derived>, "c");
    const Base* pc = &*c;
    throw_on_copy = true;
    bool threw = false;
    try {
      c = b;
    } catch (const std::runtime_error&) {
      threw = true;
    }
    throw_on_copy = false;
    CHECK(threw && &*c == pc && c->name() == "derived:c" && clean());
    // Unequal, no POCMA: the dynamic type is move-constructed with allocator 3.
    P d(std::allocator_arg, A(2), std::in_place_type<Derived>, "d");
    c = std::move(d);
    CHECK(c->name() == "derived:d" && c.get_allocator().id == 3 && clean());
    // A throwing move: no effects on either.
    P e(std::allocator_arg, A(4), std::in_place_type<Derived>, "e");
    const Base* pe = &*e;
    const Base* pcc = &*c;
    throw_on_move = true;
    threw = false;
    try {
      c = std::move(e);
    } catch (const std::runtime_error&) {
      threw = true;
    }
    throw_on_move = false;
    CHECK(threw && &*e == pe && e->name() == "derived:e" && &*c == pcc && c->name() == "derived:d" && clean());
    // Copy from a valueless polymorphic: *this becomes valueless.
    P gone(std::allocator_arg, A(4), std::in_place_type<Derived>, "g");
    P keep(std::move(gone));
    CHECK(gone.valueless_after_move());
    const int d3 = ledger.destroys[3];
    c = gone;
    CHECK(c.valueless_after_move() && ledger.destroys[3] == d3 + 1 && clean());
    // And copying into the valueless object again.
    c = keep;
    CHECK(!c.valueless_after_move() && c->name() == "derived:g" && c.get_allocator().id == 3);
  }
  {
    using AP = LAlloc<Base, true, false>;
    std::polymorphic<Base, AP> a(std::allocator_arg, AP(5), std::in_place_type<Derived>, "a"),
        b(std::allocator_arg, AP(6), std::in_place_type<Derived>, "b");
    const int c6 = ledger.constructs[6], d5 = ledger.destroys[5];
    a = b;
    CHECK(a->name() == "derived:b" && a.get_allocator().id == 6);
    CHECK(ledger.constructs[6] == c6 + 1 && ledger.destroys[5] == d5 + 1 && clean());
  }
  CHECK(clean() && ledger.live.empty());
}

int main() {
  indirect_copy();
  indirect_move();
  polymorphic();
  return 0;
}
