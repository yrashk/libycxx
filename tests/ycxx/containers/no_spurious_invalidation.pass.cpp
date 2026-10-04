// [container.reqmts]/67: "Unless otherwise specified (either explicitly or by defining a
// function in terms of other functions), invoking a container member function or passing a
// container as an argument to a library function shall not invalidate iterators to, or
// change the values of, objects within that container." Checked for const observers,
// non-const element access and iteration, comparisons, and the [iterator.range] /
// [range.access] functions; afterwards every element is still at the same address with the
// same value. ([string.require]/4 lists operator[], at, data, front, back, begin, rbegin,
// end and rend as non-invalidating for basic_string, too.)
#include <vector>
#include <string>
#include <iterator>
#include <memory>
#include <ranges>
#include "container_values.hpp"
#include "check.hpp"

#include "reqs/no_spurious_invalidation.hpp"

using namespace reqs::no_spurious_invalidation;

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  return 0;
}
