// inplace_vector's insertions in the middle ([sequence.reqmts] insert(p, i, j), insert_range,
// insert(p, n, t), emplace; [inplace.vector.modifiers]/1: "linear in the number of elements
// inserted plus the distance to the end of the vector") construct the new elements at the end
// and rotate them into place; the rotation must walk the elements sequentially, not follow
// the cycles of a rotation by the number inserted, which visits them with that stride (one
// cache miss per element moved once the elements outgrow the cache). Measured with an element
// type that logs its assignments (support/move_locality.hpp): at most a few far jumps and a
// number of assignments linear in the elements moved; the result is checked against a model.
#include <cstddef>
#include <inplace_vector>
#include <vector>
#include "check.hpp"
#include "move_locality.hpp"
#include "test_iterators.hpp"

using locality::E;
using locality::stats;
using IV = std::inplace_vector<E, 6000>;

enum Source { forward, input, fill, one };

static void check_insert(Source kind, long n0, long pos, long k) {
  if (kind == one) k = 1;
  IV v;
  for (long i = 0; i != n0; ++i) v.emplace_back(static_cast<int>(i));
  std::vector<int> src;
  for (long i = 0; i != k; ++i) src.push_back(static_cast<int>(100000 + (kind == fill ? 0 : i)));
  const auto p = v.begin() + pos;
  locality::start();
  IV::iterator r;
  switch (kind) {
  case forward: r = v.insert(p, ForwardIter<int>(src.data()), ForwardIter<int>(src.data() + k)); break;
  case input: r = v.insert_range(p, InputRange<int>{src.data(), src.data() + k}); break;
  case fill: r = v.insert(p, static_cast<std::size_t>(k), E(100000)); break;
  case one: r = v.emplace(p, 100000); break;
  }
  const long assigns = stats.assigns;
  const long jumps = locality::far_jumps(v);
  const long moved = n0 - pos + k;
  CHECK_SAY(jumps <= 8, "source %d n0 %ld pos %ld k %ld: %ld far jumps", kind, n0, pos, k, jumps);
  CHECK_SAY(assigns <= 3 * moved + 8, "source %d n0 %ld pos %ld k %ld: %ld assignments", kind, n0, pos, k, assigns);
  CHECK(r == v.begin() + pos);
  CHECK(static_cast<long>(v.size()) == n0 + k);
  for (long i = 0; i != n0 + k; ++i) {
    const int want = i < pos       ? static_cast<int>(i)
                     : i < pos + k ? static_cast<int>(100000 + (kind == fill ? 0 : i - pos))
                                   : static_cast<int>(i - k);
    CHECK(v[static_cast<std::size_t>(i)].v == want);
  }
}

int main() {
  for (Source kind : {forward, input, fill, one})
    for (long pos : {0L, 1L, 700L, 1999L, 2000L})
      for (long k : {1L, 2L, 7L, 100L, 1000L, 3000L}) check_insert(kind, 2000, pos, k);
  CHECK(stats.live == 0);
}
