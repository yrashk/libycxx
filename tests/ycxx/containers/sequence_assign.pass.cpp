// [sequence.reqmts]/16-19: "a = il" has type X&, assigns the range [il.begin(), il.end())
// into a, returns *this. /57-59: a.assign(i, j) has type void and replaces the elements with
// a copy of [i, j). /60-63: a.assign_range(rg) has type void and replaces the elements with
// a copy of each element of rg. /65: a.assign(il) is a.assign(il.begin(), il.end()).
// /66-68: a.assign(n, t) has type void and replaces the elements with n copies of t.
// Each is checked growing, shrinking and to/from empty. (For basic_string the assign
// members return *this, [string.assign].)
#include <vector>
#include <string>
#include <initializer_list>
#include <ranges>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

#include "reqs/sequence_assign.hpp"

using namespace reqs::sequence_assign;

constexpr bool converting() {
  long longs[] = {5, 6, 7};
  std::vector<int> v{1};
  v.assign(longs, longs + 3);
  if (v.size() != 3 || v[0] != 5) return false;
  std::vector<Elem> e;
  int ints[] = {3, 4};
  e.assign_range(ints);
  if (e.size() != 2 || e[1].value() != 4) return false;
  e.assign(InputIter<int>(ints), InputIter<int>(ints + 1));
  return e.size() == 1 && e[0].value() == 3;
}

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::string>());
static_assert(converting());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::u32string>());
  CHECK(converting());
  return 0;
}
