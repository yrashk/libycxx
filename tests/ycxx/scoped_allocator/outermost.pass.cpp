// [allocator.adaptor.members]/1: "OUTERMOST(x) is OUTERMOST(x.outer_allocator()) if the
// expression x.outer_allocator() is valid and x otherwise; OUTERMOST_ALLOC_TRAITS(x) is
// allocator_traits<remove_reference_t<decltype(OUTERMOST(x))>>" -- recursive. /9: construct
// calls OUTERMOST_ALLOC_TRAITS(*this)::construct(OUTERMOST(*this), p, newargs...) with the
// uses-allocator arguments for inner_allocator(); /10: destroy calls
// OUTERMOST_ALLOC_TRAITS(*this)::destroy(OUTERMOST(*this), p). So when OuterAlloc itself has
// an outer_allocator() (a user allocator wrapping another, or a scoped_allocator_adaptor used
// as OuterAlloc), the construct/destroy members of the innermost-of-outer allocator in that
// chain are the ones called, never those of the intermediate wrappers; when the outermost
// allocator has no construct/destroy, allocator_traits' defaults are used.
#include <scoped_allocator>
#include <cstddef>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>
#include "check.hpp"

static int rec_constructs = 0, rec_destroys = 0, wrap_constructs = 0, wrap_destroys = 0;

template <class T>
struct Rec {
  using value_type = T;
  int id = 0;
  Rec() = default;
  explicit Rec(int i) : id(i) {}
  template <class U>
  Rec(const Rec<U>& o) : id(o.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ++rec_constructs;
    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
  }
  template <class U>
  void destroy(U* p) {
    ++rec_destroys;
    p->~U();
  }
  template <class U>
  friend bool operator==(const Rec& a, const Rec<U>& b) { return a.id == b.id; }
};

// An allocator that exposes another one through outer_allocator(); its own construct and
// destroy must not be used by the adaptor.
template <class T>
struct Wrap {
  using value_type = T;
  Rec<T> r;
  Wrap() = default;
  explicit Wrap(int i) : r(i) {}
  template <class U>
  Wrap(const Wrap<U>& o) : r(o.r) {}
  Rec<T>& outer_allocator() { return r; }
  const Rec<T>& outer_allocator() const { return r; }
  T* allocate(std::size_t n) { return r.allocate(n); }
  void deallocate(T* p, std::size_t n) { r.deallocate(p, n); }
  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ++wrap_constructs;
    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
  }
  template <class U>
  void destroy(U* p) {
    ++wrap_destroys;
    p->~U();
  }
  template <class U>
  friend bool operator==(const Wrap& a, const Wrap<U>& b) { return a.r == b.r; }
};

template <class T>
struct Plain {  // no construct/destroy
  using value_type = T;
  int id = 0;
  Plain() = default;
  explicit Plain(int i) : id(i) {}
  template <class U>
  Plain(const Plain<U>& o) : id(o.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  template <class U>
  friend bool operator==(const Plain& a, const Plain<U>& b) { return a.id == b.id; }
};

using Str = std::basic_string<char, std::char_traits<char>, Plain<char>>;
static const char* const longs = "a string that is too long for any small-string buffer, 0123456789";

int main() {
  {  // Outer allocator with outer_allocator(): Rec's construct, not Wrap's.
    using S = std::scoped_allocator_adaptor<Wrap<Str>, Plain<char>>;
    S s(Wrap<Str>(1), Plain<char>(2));
    Str* p = s.allocate(1);
    s.construct(p, longs);
    CHECK(rec_constructs == 1 && wrap_constructs == 0);
    CHECK(p->get_allocator().id == 2 && *p == longs);
    s.destroy(p);
    CHECK(rec_destroys == 1 && wrap_destroys == 0);
    s.deallocate(p, 1);

    std::vector<Str, S> v(s);
    v.emplace_back(longs);
    v.emplace_back(3, 'x');
    CHECK(rec_constructs >= 3 && wrap_constructs == 0);
    CHECK(v[0].get_allocator().id == 2 && v[1].get_allocator().id == 2);
    v.clear();
    CHECK(rec_destroys >= 3 && wrap_destroys == 0);
  }
  rec_constructs = rec_destroys = 0;
  {  // A scoped_allocator_adaptor as OuterAlloc: OUTERMOST recurses through it to Rec.
    using InnerS = std::scoped_allocator_adaptor<Rec<Str>, Plain<char>>;
    using S = std::scoped_allocator_adaptor<InnerS, Plain<char>>;
    S s(InnerS(Rec<Str>(1), Plain<char>(5)), Plain<char>(7));
    Str* p = s.allocate(1);
    s.construct(p, longs);
    CHECK(rec_constructs == 1);
    // uses-allocator construction with S::inner_allocator() (Plain(7)), not InnerS's inner.
    CHECK(p->get_allocator().id == 7);
    s.destroy(p);
    CHECK(rec_destroys == 1);
    s.deallocate(p, 1);
    using P = std::pair<Str, int>;
    using SP = std::scoped_allocator_adaptor<std::scoped_allocator_adaptor<Rec<P>, Plain<char>>, Plain<char>>;
    SP sp(std::scoped_allocator_adaptor<Rec<P>, Plain<char>>(Rec<P>(1), Plain<char>(5)), Plain<char>(8));
    P* pp = sp.allocate(1);
    sp.construct(pp, std::piecewise_construct, std::forward_as_tuple(longs), std::forward_as_tuple(4));
    CHECK(rec_constructs == 2 && pp->first.get_allocator().id == 8 && pp->second == 4);
    sp.destroy(pp);
    CHECK(rec_destroys == 2);
    sp.deallocate(pp, 1);
  }
  {  // Outermost without construct/destroy: allocator_traits' defaults.
    using S = std::scoped_allocator_adaptor<Plain<Str>, Plain<char>>;
    S s(Plain<Str>(1), Plain<char>(9));
    Str* p = s.allocate(1);
    s.construct(p, 5, 'y');
    CHECK(p->get_allocator().id == 9 && *p == "yyyyy");
    s.destroy(p);
    s.deallocate(p, 1);
  }
  return 0;
}
