// [mem.poly.allocator.mem]/14: polymorphic_allocator::construct(p, args...) constructs "by
// uses-allocator construction with allocator *this", i.e. with the arguments of
// uses_allocator_construction_args<T> ([allocator.uses.construction]), which for a pair
// applies uses-allocator construction to each member: /9-10 (no arguments), /11-12 (u, v),
// /13-16 (pair<U,V> lvalue, const rvalue, rvalue), /6-8 (piecewise), recursively for a pair
// of pairs (/8 calls uses_allocator_construction_args<T1> on the member type, which is itself
// a pair). Both conventions of /5 are used for the members: leading allocator_arg_t and
// trailing allocator. (Pair-like and pair-convertible arguments: poly_construct_pair_like.)
// /1, /8: allocate(n) and allocate_object<T>(n) throw bad_array_new_length exactly when
// numeric_limits<size_t>::max() / sizeof(T) < n, without calling the resource; otherwise they
// call resource->allocate(n * sizeof(T), alignof(T)) (/5: allocate_bytes forwards unchanged).
#include <memory_resource>
#include <array>
#include <cstddef>
#include <limits>
#include <new>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include "check.hpp"
#include "recording_resource.hpp"

using PA = std::pmr::polymorphic_allocator<>;

struct Leading {  // leading-allocator convention
  using allocator_type = PA;
  int v = 0;
  std::pmr::memory_resource* r = nullptr;
  Leading(std::allocator_arg_t, const PA& a) : r(a.resource()) {}
  Leading(std::allocator_arg_t, const PA& a, int x) : v(x), r(a.resource()) {}
  Leading(std::allocator_arg_t, const PA& a, const Leading& o) : v(o.v), r(a.resource()) {}
  Leading(std::allocator_arg_t, const PA& a, Leading&& o) : v(o.v + 1000), r(a.resource()) {}
  Leading(int x) : v(x) {}
  Leading(const Leading& o) : v(o.v) {}
  Leading(Leading&& o) : v(o.v + 1000) {}
};

struct Trailing {  // trailing-allocator convention
  using allocator_type = PA;
  int v = 0;
  std::pmr::memory_resource* r = nullptr;
  Trailing(const PA& a) : r(a.resource()) {}
  Trailing(int x, const PA& a) : v(x), r(a.resource()) {}
  Trailing(const Trailing& o, const PA& a) : v(o.v), r(a.resource()) {}
  Trailing(Trailing&& o, const PA& a) : v(o.v + 1000), r(a.resource()) {}
  Trailing(int x) : v(x) {}
  Trailing(const Trailing& o) : v(o.v) {}
  Trailing(Trailing&& o) : v(o.v + 1000) {}
};

using P = std::pair<Leading, Trailing>;

struct ToPair {  // not pair-like, converts to P
  int a, b;
  operator P() const { return P(Leading(a), Trailing(b)); }
};

template <class T, class... Args>
T* make(PA a, Args&&... args) {
  T* p = a.allocate_object<T>();
  a.construct(p, std::forward<Args>(args)...);
  return p;
}
template <class T>
void drop(PA a, T* p) {
  p->~T();
  a.deallocate_object(p);
}

struct SizeOnly : std::pmr::memory_resource {  // records the request, never touches memory
  int calls = 0;
  std::size_t bytes = 0, align = 0;
  alignas(64) unsigned char buf[64];
  void* do_allocate(std::size_t b, std::size_t al) override {
    ++calls;
    bytes = b;
    align = al;
    return buf;
  }
  void do_deallocate(void*, std::size_t, std::size_t) override {}
  bool do_is_equal(const memory_resource& o) const noexcept override { return this == &o; }
};

struct Three { char c[3]; };
struct alignas(32) Wide { char c[96]; };

template <class T>
void overflow_bounds() {
  constexpr std::size_t lim = std::numeric_limits<std::size_t>::max() / sizeof(T);
  SizeOnly r;
  std::pmr::polymorphic_allocator<T> typed(&r);
  PA bytes(&r);
  (void)typed.allocate(lim);  // lim * sizeof(T) does not overflow: forwarded as is
  CHECK(r.calls == 1 && r.bytes == lim * sizeof(T) && r.align == alignof(T));
  (void)bytes.allocate_object<T>(lim);
  CHECK(r.calls == 2 && r.bytes == lim * sizeof(T) && r.align == alignof(T));
  for (std::size_t n : {lim + 1, lim + 2, std::numeric_limits<std::size_t>::max()}) {
    bool t1 = false, t2 = false;
    try { (void)typed.allocate(n); } catch (const std::bad_array_new_length&) { t1 = true; }
    try { (void)bytes.allocate_object<T>(n); } catch (const std::bad_array_new_length&) { t2 = true; }
    CHECK(t1 && t2 && r.calls == 2);
  }
  (void)bytes.allocate_bytes(std::numeric_limits<std::size_t>::max(), 2);
  CHECK(r.calls == 3 && r.bytes == std::numeric_limits<std::size_t>::max() && r.align == 2);
  (void)bytes.allocate_object<T>(0);
  CHECK(r.calls == 4 && r.bytes == 0 && r.align == alignof(T));
}

int main() {
  RecordingResource r;
  PA a(&r);
  auto uses = [&](const P& p) { return p.first.r == &r && p.second.r == &r; };

  P* p = make<P>(a);  // /9-10
  CHECK(uses(*p) && p->first.v == 0 && p->second.v == 0);
  drop(a, p);

  p = make<P>(a, 1, 2);  // /11-12
  CHECK(uses(*p) && p->first.v == 1 && p->second.v == 2);
  drop(a, p);

  std::pair<int, int> src(3, 4);
  p = make<P>(a, src);  // /13-14, lvalue pair<U,V>
  CHECK(uses(*p) && p->first.v == 3 && p->second.v == 4);
  drop(a, p);
  const std::pair<int, int> csrc(5, 6);
  p = make<P>(a, std::move(csrc));  // /15-16, const rvalue
  CHECK(uses(*p) && p->first.v == 5 && p->second.v == 6);
  drop(a, p);

  P other(Leading(7), Trailing(8));  // members made without an allocator
  CHECK(other.first.r == nullptr && other.first.v == 1007);
  p = make<P>(a, other);  // copies, each with the allocator
  CHECK(uses(*p) && p->first.v == 1007 && p->second.v == 1008);
  drop(a, p);
  p = make<P>(a, std::move(other));  // moves, each with the allocator
  CHECK(uses(*p) && p->first.v == 2007 && p->second.v == 2008);
  drop(a, p);

  // Pairs nested in pairs: uses-allocator construction applies at every level.
  using PP = std::pair<P, std::pair<Trailing, Leading>>;
  PP* pp = make<PP>(a);
  CHECK(uses(pp->first) && pp->second.first.r == &r && pp->second.second.r == &r);
  drop(a, pp);
  pp = make<PP>(a, std::pair<int, int>(1, 2), std::pair<int, int>(3, 4));
  CHECK(uses(pp->first) && pp->second.first.r == &r && pp->second.second.r == &r);
  CHECK(pp->first.first.v == 1 && pp->first.second.v == 2 && pp->second.first.v == 3 && pp->second.second.v == 4);
  drop(a, pp);

  // pmr::string members of a pair inside a pair, constructed from a pair of strings that use
  // another resource.
  using SP = std::pair<std::pair<std::pmr::string, int>, std::pmr::string>;
  RecordingResource other_r;
  std::pmr::string longa(100, 'a', &other_r), longb(100, 'b', &other_r);
  SP* sp = make<SP>(a, std::pair<std::pmr::string, int>(longa, 1), longb);
  CHECK(sp->first.first.get_allocator().resource() == &r && sp->second.get_allocator().resource() == &r);
  CHECK(sp->first.first == longa && sp->second == longb && sp->first.second == 1);
  drop(a, sp);

  CHECK(r.outstanding == 0);

  overflow_bounds<Three>();
  overflow_bounds<Wide>();
  overflow_bounds<long double>();
  return 0;
}
