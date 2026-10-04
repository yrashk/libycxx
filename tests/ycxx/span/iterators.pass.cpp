// [span.iterators]: begin()/end() noexcept; "If empty() is true, then it returns the same
// value as end()"; rbegin() is reverse_iterator(end()), rend() is reverse_iterator(begin());
// cbegin()/cend() return const_iterator { return begin(); }, crbegin()/crend() likewise.
#include <span>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

using S = std::span<int, 4>;
static_assert(noexcept(std::declval<const S&>().begin()));
static_assert(noexcept(std::declval<const S&>().end()));
static_assert(noexcept(std::declval<const S&>().cbegin()));
static_assert(noexcept(std::declval<const S&>().cend()));
static_assert(noexcept(std::declval<const S&>().rbegin()));
static_assert(noexcept(std::declval<const S&>().rend()));
static_assert(noexcept(std::declval<const S&>().crbegin()));
static_assert(noexcept(std::declval<const S&>().crend()));
static_assert(std::is_same_v<decltype(std::declval<const S&>().begin()), S::iterator>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().cbegin()), S::const_iterator>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().rbegin()), S::reverse_iterator>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().crbegin()), S::const_reverse_iterator>);
static_assert(std::is_same_v<decltype(*std::declval<const S&>().cbegin()), const int&>);
static_assert(std::is_same_v<decltype(*std::declval<const S&>().crbegin()), const int&>);
static_assert(std::is_same_v<decltype(*std::declval<const S&>().begin()), int&>);
static_assert(std::random_access_iterator<S::iterator>);
static_assert(std::contiguous_iterator<S::const_iterator>);
static_assert(std::is_default_constructible_v<S::iterator>);

constexpr bool test() {
  int a[4] = {1, 2, 3, 4};
  S s(a);
  if (s.end() - s.begin() != 4) return false;
  if (std::to_address(s.begin()) != a) return false;
  int sum = 0;
  for (int& x : s) sum += x;
  if (sum != 10) return false;
  for (auto it = s.begin(); it != s.end(); ++it) *it *= 2;
  if (a[3] != 8) return false;
  if (*s.rbegin() != 8 || *(s.rend() - 1) != 2) return false;
  if (s.rbegin().base() != s.end() || s.rend().base() != s.begin()) return false;
  if (s.cbegin() != s.begin() || s.cend() != s.end()) return false;
  if (*s.crbegin() != 8) return false;
  std::span<int> e;
  if (e.begin() != e.end() || e.cbegin() != e.cend() || e.rbegin() != e.rend()) return false;
  // iterator comparisons with three-way and ordering
  if (!(s.begin() < s.end()) || s.begin()[2] != 6) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
