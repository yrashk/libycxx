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

template <class X>
constexpr bool test() {
  using RI = typename X::reverse_iterator;
  using CRI = typename X::const_reverse_iterator;
  X a = make<X>({1, 2, 3, 4});
  const X& ca = a;
  static_assert(std::is_same_v<decltype(a.rbegin()), RI> && std::is_same_v<decltype(a.rend()), RI>);
  static_assert(std::is_same_v<decltype(ca.rbegin()), CRI> && std::is_same_v<decltype(ca.rend()), CRI>);
  static_assert(std::is_same_v<decltype(a.crbegin()), CRI> && std::is_same_v<decltype(a.crend()), CRI>);
  if (a.rbegin() != RI(a.end()) || a.rend() != RI(a.begin())) return false;
  if (ca.rbegin() != CRI(ca.end()) || ca.rend() != CRI(ca.begin())) return false;
  if (a.crbegin() != ca.rbegin() || a.crend() != ca.rend()) return false;
  if (a.rbegin().base() != a.end() || a.rend().base() != a.begin()) return false;
  int expect = 4;
  for (auto it = a.rbegin(); it != a.rend(); ++it, --expect)
    if (!(*it == val<typename X::value_type>(expect))) return false;
  if (expect != 0) return false;
  expect = 4;
  for (auto it = a.crbegin(); it != a.crend(); ++it, --expect)
    if (!(*it == val<typename X::value_type>(expect))) return false;
  if (std::distance(a.rbegin(), a.rend()) != 4) return false;
  X e;
  if (e.rbegin() != e.rend() || e.crbegin() != e.crend()) return false;
  return true;
}

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
