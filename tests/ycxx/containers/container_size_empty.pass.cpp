// [container.reqmts]/52-62: c.size() has type size_type and returns distance(c.begin(),
// c.end()); c.max_size() has type size_type and returns distance(begin(), end()) for the
// largest possible container (so it is at least size() and, being a distance, at most the
// largest difference_type value); c.empty() has type bool and returns c.begin() == c.end().
// All three are usable on a const container.
#include <vector>
#include <string>
#include <iterator>
#include <limits>
#include <type_traits>
#include "container_values.hpp"
#include "check.hpp"

template <class X>
constexpr bool test() {
  using S = typename X::size_type;
  using D = typename X::difference_type;
  static_assert(std::is_same_v<decltype(std::declval<const X&>().size()), S>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().max_size()), S>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().empty()), bool>);
  X c;
  for (int n = 0; n < 40; ++n) {
    const X& cc = c;
    if (cc.size() != static_cast<S>(std::distance(cc.begin(), cc.end()))) return false;
    if (cc.size() != static_cast<S>(n)) return false;
    if (cc.empty() != (cc.begin() == cc.end())) return false;
    if (cc.empty() != (n == 0)) return false;
    if (cc.max_size() < cc.size()) return false;
    if (cc.max_size() > static_cast<S>(std::numeric_limits<D>::max())) return false;
    c.insert(c.end(), val<typename X::value_type>(n));
  }
  while (!c.empty()) {
    c.erase(c.begin());
    if (c.size() != static_cast<S>(std::distance(c.begin(), c.end()))) return false;
  }
  return c.size() == 0;
}

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<char>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  CHECK(test<std::u32string>());
  return 0;
}
