// The helper also enforces user-approved libycxx comparison budgets. Those finite
// performance policies are stronger than the draft's asymptotic complexity requirements.
// [flat.map.overview]: flat_map and flat_multimap meet the associative container requirements, including their
// complexity clauses ([associative.reqmts.general]), measured in comparisons by the counting
// comparator of support/reqs/assoc_complexity.hpp (see there for the bounds used):
// find / count / contains / lower_bound / upper_bound / equal_range are logarithmic (also
// for a key with many equivalent elements); [flat.map.cons]/2, /4, [flat.set.cons]/2: construction
// from sorted containers is linear, from sorted_unique / sorted_equivalent containers constant;
// insert(sorted_unique, first, last) and insert_range(sorted_unique, rg) are linear,
// insert(first, last) / insert_range(rg) N + M log M ([flat.map.modifiers], [flat.set.modifiers]).
#include <flat_map>
#include "reqs/assoc_complexity.hpp"
#include "check.hpp"

using reqs::assoc_complexity::CountLess;

int main() {
  bool ok = true;  // run every instantiation, so that every violation is reported
  ok = reqs::assoc_complexity::test<std::flat_map<int, int, CountLess>>() && ok;
  ok = reqs::assoc_complexity::test<std::flat_multimap<int, int, CountLess>>() && ok;
  CHECK(ok);
  return 0;
}
