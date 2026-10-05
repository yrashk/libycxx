// [container.reqmts]/27-38: b.begin() / b.end() are const_iterator for a const X and iterator
// otherwise; begin() refers to the first element and end() is the past-the-end value;
// b.cbegin() returns const_cast<X const&>(b).begin() and b.cend() returns
// const_cast<X const&>(b).end(), both of type const_iterator. /39-40: i <=> j has type
// strong_ordering when iterator is random access. /63: in i == j, i != j, i < j, i <= j,
// i >= j, i > j, i <=> j and i - j, either or both operands may be a const_iterator referring
// to the same element with no change in semantics. /60: empty() is begin() == end().
// [forward.iterators]/2: value-initialized iterators of the same type compare equal;
// /3-4: two dereferenceable iterators are equal iff they refer to the same object, and the
// multi-pass guarantee holds.
// COUNTERPART: libstdcxx:23_containers/(map/debug/112477|set/debug/114316|unordered_set/debug/114316|vector/debug/114316|vector/debug/n3644|vector/debug/52433).cc
#include <vector>
#include <string>
#include <compare>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

#include "reqs/container_iterators.hpp"

using namespace reqs::container_iterators;

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::string>());
static_assert(test<std::wstring>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  CHECK(test<std::u16string>());
  return 0;
}
