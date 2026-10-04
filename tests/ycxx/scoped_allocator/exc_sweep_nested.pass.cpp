// Exception-injection sweep over nested allocator-aware containers with
// scoped_allocator_adaptor: vector<vector<T, A>, scoped_allocator_adaptor<A_outer, A>>,
// map<string, vector<T>> and deque<list<T>> with scoped adaptors whose outer and inner
// allocators (exh::alloc with distinct ids) throw at their k-th allocate, and element
// constructors that throw at their k-th call, for every k.
//   [allocator.adaptor.members]/9-10: construct(p, args...) uses uses-allocator construction
//     with inner_allocator(): every inner container is created with the inner allocator (id 2)
//     and every element block of an inner container is allocated by it.
//   [container.alloc.reqmts], [res.on.exception.handling]: on an exception every constructed
//     inner container and element is destroyed exactly once and every block (outer and inner)
//     is deallocated exactly once, with its size, by an allocator equal to the allocating one.
//   [vector.modifiers]/2, [deque.modifiers]/3, [associative.reqmts.except]/2: single-element
//     insertions at the end / at either end / into a map have no effects when they throw.
#include <deque>
#include <list>
#include <map>
#include <scoped_allocator>
#include <string>
#include <vector>
#include "exc_harness.hpp"

using namespace exh;

using IA = alloc<T>;
using IV = std::vector<T, IA>;
using OV = std::vector<IV, std::scoped_allocator_adaptor<alloc<IV>, IA>>;
using SA = alloc<char>;
using S = std::basic_string<char, std::char_traits<char>, SA>;
using MV = std::map<S, IV, std::less<S>, std::scoped_allocator_adaptor<alloc<std::pair<const S, IV>>, IA>>;
using IL = std::list<T, IA>;
using DL = std::deque<IL, std::scoped_allocator_adaptor<alloc<IL>, IA>>;

static const T src_g[5] = {T(1), T(2), T(3), T(4), T(5)};

template <class C>
static bool inner_ids_ok(const C& c) {
  for (auto& x : c)
    if (x.get_allocator().id != 2) return false;
  return true;
}

int main() {
  const auto K = {copy_ctor, move_ctor, allocation};
  sweep_kinds("vector<vector<T>> scoped: emplace_back(n, x), push_back(inner)", K, [] {
    OV v(OV::allocator_type(alloc<IV>(1), IA(2)));
    for (int i = 0; i < 3; ++i) v.emplace_back(std::size_t(i + 1), src_g[i]);
    snap sizes;
    sizes.n = 0;
    for (auto& x : v) sizes.v[sizes.n++] = int(x.size());
    bool threw = attempt([&] {
      v.emplace_back(4, src_g[3]);
      IV extra(src_g, src_g + 5, IA(7));
      v.push_back(extra); // copied with the inner allocator (id 2), not id 7
    });
    EXH_EXPECT(inner_ids_ok(v), "an inner vector does not use the inner allocator");
    if (threw) {
      // each call is a single-element insertion at the end: either it happened or not
      EXH_EXPECT(v.size() >= 3, "elements lost");
      bool same = true;
      for (int i = 0; i < 3; ++i) same = same && int(v[std::size_t(i)].size()) == sizes.v[i];
      EXH_EXPECT(same, "earlier inner vectors changed by a throwing emplace_back/push_back");
    }
    return threw;
  });
  sweep_kinds("vector<vector<T>> scoped: copy construction and assignment", K, [] {
    OV v(OV::allocator_type(alloc<IV>(1), IA(2)));
    for (int i = 0; i < 4; ++i) v.emplace_back(std::size_t(i + 2), src_g[i]);
    OV w(OV::allocator_type(alloc<IV>(1), IA(2)));
    w.emplace_back(1, src_g[4]);
    return attempt([&] {
      OV c(v);
      w = v;
      c.insert(c.begin() + 1, v.begin(), v.end());
    });
  });
  sweep_kinds("map<string, vector<T>> scoped: try_emplace, operator[], insert", K, [] {
    MV m(MV::allocator_type(alloc<std::pair<const S, IV>>(1), IA(2)));
    m.try_emplace(S("first key long enough to be allocated", SA(5)), 3, src_g[0]);
    long n0 = long(m.size());
    bool threw = attempt([&] {
      m.try_emplace(S("second key long enough to be allocated", SA(5)), 2, src_g[1]);
      m[S("third key, also long enough to be allocated", SA(5))].push_back(src_g[2]);
      MV copy(m);
    });
    for (auto& [k, val] : m) EXH_EXPECT(val.get_allocator().id == 2, "an inner vector does not use the inner allocator");
    EXH_EXPECT(long(m.size()) >= n0, "elements lost");
    return threw;
  });
  sweep_kinds("deque<list<T>> scoped: emplace_front, emplace_back, insert(mid)", K, [] {
    DL d(DL::allocator_type(alloc<IL>(1), IA(2)));
    for (int i = 0; i < 5; ++i) d.emplace_back(std::size_t(2), src_g[i]);
    bool threw = attempt([&] {
      d.emplace_front(std::size_t(3), src_g[0]);
      d.emplace_back(std::size_t(3), src_g[1]);
      d.emplace(d.begin() + 2, std::size_t(1), src_g[2]);
      for (int i = 0; i < 40; ++i) d.emplace_back(std::size_t(1), src_g[3]);
    });
    EXH_EXPECT(inner_ids_ok(d), "an inner list does not use the inner allocator");
    return threw;
  });
  return finish();
}
