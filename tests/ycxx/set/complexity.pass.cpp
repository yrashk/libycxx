// The helper also enforces user-approved libycxx comparison budgets. Those finite
// performance policies are stronger than the draft's asymptotic complexity requirements.
// [set.overview]: set and multiset meet the associative container requirements, including their
// complexity clauses ([associative.reqmts.general]), measured in comparisons by the counting
// comparator of support/reqs/assoc_complexity.hpp (see there for the bounds used):
// X(i, j) / X(from_range, rg) are linear for sorted input; emplace_hint, insert(p, t)
// and insert(p, nh) are amortized constant when the element goes right before p (also among
// equivalent keys); find / count / contains / lower_bound / upper_bound / equal_range are
// logarithmic (equal_range also for a key with many equivalent elements); erase(q),
// erase(r) and extract(q) are amortized constant, erase(k) log(size) + count(k); merge is
// N log(size + N).
#include <set>
#include "reqs/assoc_complexity.hpp"
#include "check.hpp"

using reqs::assoc_complexity::CountLess;

int main() {
  bool ok = true;  // run every instantiation, so that every violation is reported
  ok = reqs::assoc_complexity::test<std::set<int, CountLess>>() && ok;
  ok = reqs::assoc_complexity::test<std::multiset<int, CountLess>>() && ok;
  CHECK(ok);
  return 0;
}
