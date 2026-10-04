// [sequence.reqmts]/5-7: "X u(n, t);" constructs a sequence container with n copies of t;
// distance(u.begin(), u.end()) == n. /8-10: "X u(i, j);" constructs a sequence container
// equal to the range [i, j) (input or forward iterators whose elements are implicitly
// convertible to value_type); distance == distance(i, j). /11-14: "X(from_range, rg)"
// constructs a container equal to rg for any container-compatible-range<T>. /15: "X(il)" is
// X(il.begin(), il.end()).
#include <vector>
#include <string>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

#include "reqs/sequence_construct.hpp"

using namespace reqs::sequence_construct;

// Elements implicitly convertible to value_type (container-compatible-range only needs
// convertible_to<range_reference_t<R>, T>; the iterator forms need implicit convertibility).
constexpr bool converting() {
  int ints[] = {1, 2, 3};
  std::vector<long> a(ints, ints + 3);
  std::vector<long> b(std::from_range, ints);
  std::vector<Elem> c(ints, ints + 3);
  std::vector<Elem> d(std::from_range, InputRange<int>{ints, ints + 3});
  std::vector<double> e(InputIter<int>(ints), InputIter<int>(ints + 2));
  return a.size() == 3 && a[2] == 3 && b == a && c.size() == 3 && c[1].value() == 2 && d == c &&
         e.size() == 2 && e[1] == 2.0;
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
  CHECK(test<std::wstring>());
  CHECK(converting());
  return 0;
}
