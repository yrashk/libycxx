// [string.iterators]: begin/cbegin refer to the first character, end/cend are past-the-end,
// rbegin/crbegin == reverse_iterator(end()), rend/crend == reverse_iterator(begin()).
// [container.reqmts]/27-38,63: const/non-const result types, cbegin == const begin, iterator
// and const_iterator compare with each other, i <=> j yields strong_ordering for random
// access iterators. [basic.string.general]/5: constexpr iterators.
#include <string>
#include <compare>
#include <iterator>
#include <type_traits>
#include <utility>
#include "check.hpp"

using S = std::string;
static_assert(std::is_same_v<decltype(std::declval<S&>().begin()), S::iterator>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().begin()), S::const_iterator>);
static_assert(std::is_same_v<decltype(std::declval<S&>().cbegin()), S::const_iterator>);
static_assert(std::is_same_v<decltype(std::declval<S&>().cend()), S::const_iterator>);
static_assert(std::is_same_v<decltype(std::declval<S&>().rbegin()), S::reverse_iterator>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().rend()), S::const_reverse_iterator>);
static_assert(std::is_same_v<decltype(std::declval<S&>().crbegin()), S::const_reverse_iterator>);
static_assert(std::is_same_v<decltype(std::declval<S::iterator>() <=> std::declval<S::iterator>()),
                             std::strong_ordering>);
static_assert(std::random_access_iterator<S::iterator>);
static_assert(std::is_same_v<std::iterator_traits<S::iterator>::iterator_category,
                             std::random_access_iterator_tag>);

constexpr bool test() {
  S s = "abcd";
  const S& c = s;
  if (*s.begin() != 'a' || s.end() - s.begin() != 4) return false;
  if (c.begin() != s.cbegin() || c.end() != s.cend()) return false;
  if (*s.rbegin() != 'd' || *(s.rend() - 1) != 'a') return false;
  if (s.rbegin().base() != s.end() || s.crend().base() != s.cbegin()) return false;
  S::const_iterator ci = s.begin();  // conversion
  if (ci != s.begin() || !(s.begin() == ci) || (ci <=> s.begin() + 1) >= 0) return false;
  if (s.begin() + 2 > s.cend() || s.cend() - s.begin() != 4) return false;
  for (auto& ch : s) ch = static_cast<char>(ch - 'a' + 'A');
  if (s != "ABCD") return false;
  S::iterator it = s.begin();
  it += 3;
  *it = 'z';
  if (s[3] != 'z' || it[-1] != 'C') return false;
  S e;
  if (e.begin() != e.end() || e.rbegin() != e.rend()) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
