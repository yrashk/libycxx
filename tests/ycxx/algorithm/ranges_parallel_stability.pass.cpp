// [algorithm.stable]: the algorithms whose Remarks say "Stable" preserve the relative order of
// equivalent elements, the parallel overloads included:
// [stable.sort], [alg.merge] (merge, and inplace_merge which returns last), [alg.remove]
// (remove_if), [alg.unique] (keeps the first of each group), [alg.partitions]
// (stable_partition, partition_copy), [alg.copy] (copy_if), [set.union] (equivalent elements
// from the first range first).
#include <algorithm>
#include <execution>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;

struct P {
  int k, id;
};
using V = std::vector<P>;

bool ids(const V& v, std::initializer_list<int> want) {
  if (v.size() != want.size()) return false;
  auto w = want.begin();
  for (const P& p : v)
    if (p.id != *w++) return false;
  return true;
}

template <class Pol>
void run(Pol&& pol) {
  {
    V v{{2, 0}, {1, 1}, {2, 2}, {0, 3}, {1, 4}, {0, 5}, {2, 6}};
    rg::stable_sort(pol, v, {}, &P::k);
    CHECK(ids(v, {3, 5, 1, 4, 0, 2, 6}));
  }
  {
    V v{{1, 0}, {2, 1}, {2, 2}, {3, 3}, {1, 4}, {2, 5}, {3, 6}};  // two sorted halves
    auto r = rg::inplace_merge(pol, v, v.begin() + 4, {}, &P::k);
    CHECK(r == v.end());
    CHECK(ids(v, {0, 4, 1, 2, 5, 3, 6}));
    V a{{1, 0}, {2, 1}}, b{{1, 2}, {2, 3}}, out(4);
    rg::merge(pol, a, b, out, {}, &P::k, &P::k);
    CHECK(ids(out, {0, 2, 1, 3}));
    V u(4);
    auto ur = rg::set_union(pol, a, b, u, {}, &P::k, &P::k);
    CHECK(ur.out == u.begin() + 2 && u[0].id == 0 && u[1].id == 1);
  }
  {
    V v{{1, 0}, {0, 1}, {1, 2}, {0, 3}, {1, 4}};
    auto r = rg::remove_if(pol, v, [](int k) { return k == 0; }, &P::k);
    v.erase(r.begin(), r.end());
    CHECK(ids(v, {0, 2, 4}));
  }
  {
    V v{{1, 0}, {1, 1}, {2, 2}, {2, 3}, {1, 4}};
    auto r = rg::unique(pol, v, {}, &P::k);
    v.erase(r.begin(), r.end());
    CHECK(ids(v, {0, 2, 4}));
  }
  {
    V v{{1, 0}, {0, 1}, {1, 2}, {0, 3}, {1, 4}};
    auto sp = v;
    rg::stable_partition(pol, sp, [](int k) { return k == 0; }, &P::k);
    CHECK(ids(sp, {1, 3, 0, 2, 4}));
    V t(5), f(5);
    auto pr = rg::partition_copy(pol, v, t, f, [](int k) { return k == 1; }, &P::k);
    t.erase(pr.out1, t.end());
    f.erase(pr.out2, f.end());
    CHECK(ids(t, {0, 2, 4}) && ids(f, {1, 3}));
    V c(5);
    auto cr = rg::copy_if(pol, v, c, [](int k) { return k == 0; }, &P::k);
    c.erase(cr.out, c.end());
    CHECK(ids(c, {1, 3}));
  }
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
  return 0;
}
