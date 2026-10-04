// Exception-injection sweep over hive's operations and block management: sort, unique,
// splice, reshape, shrink_to_fit, trim_capacity, reserve, and insertion/erasure patterns with
// small blocks: the comparison/predicate, the element type's constructors and assignments,
// the allocator and operator new throw at their k-th call, for every k.
//   [hive.operations]/14: sort: "If an exception is thrown, the order of the elements in *this
//     is unspecified": the elements stay (when only the comparison throws, the same values;
//     in every case the live element objects are exactly the hive's elements).
//   /3: splice: "If an exception is thrown, there are no effects" (on *this and x).
//   /10: unique throws only what the predicate throws: the remaining elements are a
//     subsequence (in iteration order) of the original, the erased ones destroyed.
//   [hive.capacity]/24: reshape: "If an exception is thrown during allocation of a new element
//     block, capacity() may be reduced, reallocation may occur, and current-limits may be
//     assigned a value other than block_limits": the elements themselves (as a multiset) are
//     unchanged by an allocation failure; /9 shrink_to_fit: the same for allocation failures;
//     "Otherwise if an exception is thrown, the effects are unspecified" (accounting only).
//   /3-5: reserve throws what the allocator throws; the elements are unchanged.
// After every run each element object is destroyed exactly once and every allocator block
// (exh::alloc) and operator new block freed.
#include <algorithm>
#include <hive>
#include "exc_new.hpp"

using namespace exh;
using H = std::hive<T, alloc<T>>;

static std::hive_limits small_limits() {
  auto hl = H::block_capacity_hard_limits();
  std::size_t lo = std::max<std::size_t>(hl.min, 4), hi = std::max<std::size_t>(lo * 2, hl.min);
  return {lo, std::min(hi, hl.max)};
}

static H make(int n, bool holes) {
  H h(small_limits());
  for (int i = 0; i < n; ++i) h.emplace((i * 37) % 101);
  if (holes) {
    int i = 0;
    for (auto it = h.begin(); it != h.end(); ++i)
      if (i % 3 == 1)
        it = h.erase(it);
      else
        ++it;
  }
  return h;
}

static bool is_subsequence(const snap& sub, const snap& full) {
  int j = 0;
  for (int i = 0; i < sub.n; ++i) {
    while (j < full.n && full.v[j] != sub.v[i]) ++j;
    if (j == full.n) return false;
    ++j;
  }
  return true;
}

template <class Op, class Check>
void go(const char* name, std::initializer_list<Kind> ks, Op op, Check check) {
  for (int holes : {0, 1})
    for (Kind k : ks) {
      auto scenario = [&] {
        H h = make(30, holes);
        snap before = snap::of(h);
        long live0 = st.live;
        bool threw = attempt([&] { op(h); });
        long n = 0;
        for (auto it = h.begin(); it != h.end(); ++it) ++n;
        EXH_EXPECT(long(h.size()) == n, "size() disagrees with traversal");
        EXH_EXPECT(st.live - live0 == n - before.n, "live element objects do not match the hive's size");
        if (threw) check(before, h, Kind(st.kind));
        return threw;
      };
      if (k == gnew)
        sweep_new(name, scenario);
      else
        sweep(name, k, new_balanced(scenario));
    }
}

int main() {
  go("sort", {compare, move_ctor, move_assign, copy_ctor, allocation, gnew}, [](H& h) { h.sort(exh::less{}); },
     [](const snap& before, H& h, Kind k) {
       if (k == compare || k == allocation || k == gnew)
         EXH_EXPECT(snap::of(h).sorted() == before.sorted(), "[hive.operations]/14: sort lost or changed elements");
     });
  go("unique (no prior sort)", {compare}, [](H& h) { h.unique(equal{}); },
     [](const snap& before, H& h, Kind) {
       EXH_EXPECT(is_subsequence(snap::of(h), before), "unique: remaining elements are not a subsequence");
     });
  for (int xholes : {0, 1})
    go(xholes ? "splice(x with holes)" : "splice(x)", {allocation, gnew}, [xholes](H& h) {
        H x = make(20, xholes);
        snap xb = snap::of(x);
        snap hb = snap::of(h);
        bool threw = true;
        try {
          h.splice(x);
          threw = false;
        } catch (...) {
          EXH_EXPECT(snap::of(x) == xb, "[hive.operations]/3: splice changed x although it threw");
          EXH_EXPECT(snap::of(h).sorted() == hb.sorted(), "[hive.operations]/3: splice changed *this although it threw");
          throw;
        }
        (void)threw;
      },
     [](const snap&, H&, Kind) {});
  go("splice(x) with x's blocks outside current-limits", {allocation}, [](H& h) {
        H x(H::block_capacity_hard_limits(), alloc<T>());
        for (int i = 0; i < 40; ++i) x.emplace(200 + i);
        snap xb = snap::of(x);
        snap hb = snap::of(h);
        try {
          h.splice(x);
        } catch (const std::length_error&) {
          EXH_EXPECT(snap::of(x) == xb && snap::of(h).sorted() == hb.sorted(), "splice: length_error with effects");
        }
      },
     [](const snap&, H&, Kind) {});
  auto keeps_elements = [](const snap& before, H& h, Kind k) {
    if (k == allocation || k == gnew)
      EXH_EXPECT(snap::of(h).sorted() == before.sorted(),
                 "[hive.capacity]: an allocation failure changed the elements (only capacity, reallocation and limits may change)");
  };
  go("reshape(larger blocks)", {allocation, move_ctor, copy_ctor, gnew}, [](H& h) {
       auto hl = H::block_capacity_hard_limits();
       h.reshape({std::min<std::size_t>(hl.max, 64), std::min<std::size_t>(hl.max, 128)});
     },
     keeps_elements);
  go("reshape(smaller blocks)", {allocation, move_ctor, copy_ctor, gnew}, [](H& h) {
       auto hl = H::block_capacity_hard_limits();
       h.reshape({hl.min, hl.min});
     },
     keeps_elements);
  go("shrink_to_fit", {allocation, move_ctor, copy_ctor, gnew}, [](H& h) { h.shrink_to_fit(); }, keeps_elements);
  go("trim_capacity", {allocation, gnew}, [](H& h) { h.trim_capacity(); }, keeps_elements);
  go("reserve(200)", {allocation, gnew}, [](H& h) { h.reserve(200); }, keeps_elements);
  go("erase all then refill", {allocation, copy_ctor}, [](H& h) {
       for (auto it = h.begin(); it != h.end();) it = h.erase(it);
       for (int i = 0; i < 40; ++i) h.insert(T(i));
     },
     [](const snap&, H&, Kind) {});
  go("copy assignment from a hive with other limits", {allocation, copy_ctor, copy_assign}, [](H& h) {
       H x(H::block_capacity_default_limits(), alloc<T>());
       for (int i = 0; i < 50; ++i) x.emplace(i);
       h = x;
     },
     [](const snap&, H&, Kind) {});
  go("swap and move", {allocation, move_ctor}, [](H& h) {
       H x = make(10, true);
       h.swap(x);
       H y(std::move(h));
       h = std::move(y);
     },
     [](const snap&, H&, Kind) {});
  return finish();
}
