// C++26 constexpr smart pointers ([memory.syn], [util.smartptr.shared], [util.smartptr.enab]):
// enable_shared_from_this's members, shared_ptr(p, d, a), reset(p, d, a), get_deleter,
// shared_ptr(unique_ptr&&), static_pointer_cast, allocate_shared (and _for_overwrite, arrays)
// are constexpr. Checked during constant evaluation:
// - [util.smartptr.shared.const]/1 enabling shared_from_this through a non-primary base;
// - /11 (p, d, a): "uses a copy of a to allocate memory for internal use" and the deleter is
//   called once; [util.smartptr.getdeleter]/1 get_deleter returns the stored deleter;
// - [util.smartptr.shared.create]/7.5, /7.7 allocate_shared constructs with the allocator,
//   /7.11-12 array elements are destroyed "in the reverse order of their initialization";
// - every allocation is released by the end (constant evaluation would reject a leak).
#include <memory>
#include <utility>
#include "check.hpp"

struct A : std::enable_shared_from_this<A> {
  int a = 1;
  constexpr virtual ~A() {}
};
struct Other {
  int o = 2;
  constexpr virtual ~Other() {}
};
struct B : Other, A {
  int b = 3;
};

template <class T>
struct CA {
  using value_type = T;
  int* live;
  constexpr explicit CA(int* l) : live(l) {}
  template <class U>
  constexpr CA(const CA<U>& o) : live(o.live) {}
  constexpr T* allocate(std::size_t n) {
    ++*live;
    return std::allocator<T>().allocate(n);
  }
  constexpr void deallocate(T* p, std::size_t n) {
    --*live;
    std::allocator<T>().deallocate(p, n);
  }
  template <class U>
  constexpr bool operator==(const CA<U>&) const { return true; }
};
struct Del {
  int* n;
  constexpr void operator()(int* p) const {
    ++*n;
    delete p;
  }
};
struct Log {
  int* order;
  int id;
  constexpr ~Log() { *order = *order * 10 + id; }
};
struct Numbered {
  static constexpr int start = 0;
  int* counter;
  int id;
  constexpr Numbered(int* c) : counter(c), id(++*c) {}
  constexpr ~Numbered() { *counter = *counter * 10 + id; }
};

constexpr bool test() {
  {
    std::shared_ptr<B> b = std::make_shared<B>();
    std::shared_ptr<A> a = b->shared_from_this();
    if (a.get() != static_cast<A*>(b.get()) || b.use_count() != 2) return false;
    std::shared_ptr<Other> o = b;
    std::shared_ptr<B> back = std::static_pointer_cast<B>(o);
    if (back != b || !back.owner_equal(a)) return false;
    std::shared_ptr<B> b3(new B);
    if (b3->weak_from_this().expired()) return false;
    std::weak_ptr<A> w = b3->weak_from_this();
    b3.reset();
    if (!w.expired() || w.lock()) return false;
    std::shared_ptr<const B> cb = std::make_shared<const B>();
    if (cb->shared_from_this() != cb) return false;
  }
  {
    int live = 0, dels = 0;
    {
      std::shared_ptr<int> p(new int(4), Del{&dels}, CA<int>(&live));
      if (live != 1 || *p != 4) return false;
      Del* d = std::get_deleter<Del>(p);
      if (!d || d->n != &dels) return false;
      auto q = std::allocate_shared<int>(CA<int>(&live), 5);
      if (*q != 5 || live != 2) return false;
      auto r = std::allocate_shared<int[]>(CA<int>(&live), 3, 7);
      if (r[2] != 7) return false;
      auto s = std::allocate_shared_for_overwrite<long[4]>(CA<char>(&live));
      s[3] = 1;
      std::unique_ptr<int, Del> u(new int(8), Del{&dels});
      std::shared_ptr<int> fu(std::move(u));
      if (*fu != 8 || !std::get_deleter<Del>(fu) || u) return false;
      p.reset(new int(9), Del{&dels}, CA<int>(&live));
      if (dels != 1 || *p != 9) return false;
    }
    if (live != 0 || dels != 3) return false;
  }
  {
    int counter = 0;
    {
      auto arr = std::make_shared<Numbered[]>(3, Numbered(&counter));  // copies of one value
      counter = 0;
    }
    // three copies with id 1 (the temporary's id), destroyed after the temporary.
    if (counter != 111) return false;
    int order = 0;
    {
      auto arr = std::allocate_shared<Log[3]>(std::allocator<Log>(), Log{&order, 5});
      arr[0].id = 1;
      arr[1].id = 2;
      arr[2].id = 3;
      order = 0;  // the argument temporary has been destroyed already
    }
    if (order != 321) return false;  // reverse order of initialization
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
