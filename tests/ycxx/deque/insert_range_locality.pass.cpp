// deque::insert(p, i, j) / insert_range(p, rg) in the middle ([sequence.reqmts]: "Inserts
// copies of elements in [i, j) before p"; [deque.modifiers]/1: "linear in the number of
// elements inserted plus the lesser of the distances to the beginning and end of the deque")
// inserts at the closer end and rotates the elements into place; the rotation must walk them
// sequentially (through the iterator's increments), not follow the cycles of a rotation by
// the number inserted, which visits them with that stride (one cache miss per element moved
// once the deque outgrows the cache, and an iterator advance by an arbitrary distance per
// access). Measured with an element type that logs its assignments (support/move_locality.hpp):
// at most a few far jumps and a number of assignments linear in the elements moved, for
// forward, input and sized input sources at either end's side; the result is checked against
// a model.
#include <cstddef>
#include <deque>
#include <vector>
#include "check.hpp"
#include "move_locality.hpp"
#include "test_iterators.hpp"

using locality::E;
using locality::stats;

enum Source { forward, input, sized_input };

static void check_insert(Source kind, long n0, long pos, long k) {
  std::deque<E> d;
  for (long i = 0; i != n0; ++i) d.emplace_back(static_cast<int>(i));
  std::vector<int> src;
  for (long i = 0; i != k; ++i) src.push_back(static_cast<int>(100000 + i));
  const auto p = d.begin() + pos;
  locality::start();
  std::deque<E>::iterator r;
  switch (kind) {
  case forward: r = d.insert(p, ForwardIter<int>(src.data()), ForwardIter<int>(src.data() + k)); break;
  case input: r = d.insert_range(p, InputRange<int>{src.data(), src.data() + k}); break;
  case sized_input:
    r = d.insert_range(p, locality::SizedInput<InputIter<int>, int>{src.data(), src.data() + k});
    break;
  }
  const long assigns = stats.assigns;
  const long jumps = locality::far_jumps(d);
  const long moved = (pos < n0 - pos ? pos : n0 - pos) + k;
  CHECK_SAY(jumps <= 8, "source %d n0 %ld pos %ld k %ld: %ld far jumps", kind, n0, pos, k, jumps);
  CHECK_SAY(assigns <= 5 * moved + 8, "source %d n0 %ld pos %ld k %ld: %ld assignments", kind, n0, pos, k, assigns);
  CHECK(r == d.begin() + pos);
  CHECK(static_cast<long>(d.size()) == n0 + k);
  for (long i = 0; i != n0 + k; ++i) {
    const int want = i < pos ? static_cast<int>(i) : i < pos + k ? static_cast<int>(100000 + i - pos) : static_cast<int>(i - k);
    CHECK(d[static_cast<std::size_t>(i)].v == want);
  }
}

int main() {
  for (Source kind : {forward, input, sized_input})
    for (long pos : {0L, 1L, 300L, 999L, 1001L, 1700L, 1999L, 2000L})
      for (long k : {1L, 2L, 7L, 100L, 1000L, 3000L}) check_insert(kind, 2000, pos, k);
  CHECK(stats.live == 0);
}
