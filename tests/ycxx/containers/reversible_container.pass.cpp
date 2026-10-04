// [container.rev.reqmts]/4-15: a.rbegin() / a.rend() have type const_reverse_iterator for a
// const X and reverse_iterator otherwise, and return reverse_iterator(end()) /
// reverse_iterator(begin()); a.crbegin() / a.crend() have type const_reverse_iterator and
// return const_cast<X const&>(a).rbegin() / .rend(). Iterating [rbegin(), rend()) visits
// the elements in reverse order.
#include <vector>
#include <string>
#include <iterator>
#include <type_traits>
#include "container_values.hpp"
#include "check.hpp"

#include "reqs/reversible_container.hpp"

using namespace reqs::reversible_container;

constexpr bool mutate_through_reverse() {
  std::vector<int> v{1, 2, 3};
  *v.rbegin() = 30;
  *(v.rend() - 1) = 10;
  return v[0] == 10 && v[2] == 30;
}

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::string>());
static_assert(mutate_through_reverse());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::string>());
  CHECK(test<std::u16string>());
  CHECK(mutate_through_reverse());
  return 0;
}
