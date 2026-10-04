// [sequence.reqmts]/24-44: a.insert(p, t), a.insert(p, rv), a.insert(p, n, t),
// a.insert(p, i, j), a.insert_range(p, rg) and a.insert(p, il) all have type iterator and
// insert before p (a const_iterator). insert(p, t) / insert(p, rv) return an iterator to the
// inserted copy; insert(p, n, t), insert(p, i, j) and insert_range(p, rg) return an
// iterator to the first inserted element, or p if nothing was inserted; insert(p, il) is
// insert(p, il.begin(), il.end()). Exercised at the beginning, middle and end, and with
// enough elements to force reallocation.
#include <vector>
#include <string>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

#include "reqs/sequence_insert.hpp"

using namespace reqs::sequence_insert;

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  return 0;
}
