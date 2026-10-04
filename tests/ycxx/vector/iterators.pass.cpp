// [container.reqmts]/27-41,63: begin/end/cbegin/cend result types, cbegin() ==
// as_const(c).begin(); i <=> j is strong_ordering for random-access iterators; iterator and
// const_iterator may be mixed in comparisons and differences. [container.rev.reqmts]:
// rbegin() == reverse_iterator(end()). [container.reqmts]/68: contiguous iterators
// (to_address(begin()) == data()). [vector.overview]/3: constexpr iterators.
#include <vector>
#include <compare>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

using V = std::vector<int>;
static_assert(std::is_same_v<decltype(std::declval<V&>().begin()), V::iterator>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().begin()), V::const_iterator>);
static_assert(std::is_same_v<decltype(std::declval<V&>().cbegin()), V::const_iterator>);
static_assert(std::is_same_v<decltype(std::declval<V&>().rbegin()), V::reverse_iterator>);
static_assert(std::is_same_v<decltype(std::declval<V&>().crend()), V::const_reverse_iterator>);
static_assert(std::is_same_v<decltype(std::declval<V::iterator>() <=> std::declval<V::const_iterator>()),
                             std::strong_ordering>);
static_assert(std::is_same_v<std::iterator_traits<V::iterator>::iterator_category,
                             std::random_access_iterator_tag>);
static_assert(std::ranges::contiguous_range<V> && std::ranges::sized_range<V>);
static_assert(std::ranges::contiguous_range<const V>);

constexpr bool test() {
  V v{1, 2, 3, 4};
  const V& c = v;
  if (std::to_address(v.begin()) != v.data() || std::to_address(c.cend()) != v.data() + 4) return false;
  if (v.cbegin() != std::as_const(v).begin() || v.end() != v.cend()) return false;
  if (*v.rbegin() != 4 || v.rbegin().base() != v.end() || v.crend().base() != v.cbegin()) return false;
  V::const_iterator ci = v.begin() + 1;
  if (ci - v.begin() != 1 || v.end() - ci != 3 || !(v.begin() < ci) || (ci <=> v.begin()) <= 0) return false;
  for (auto& x : v) x *= 10;
  if (v[3] != 40) return false;
  auto it = v.end();
  --it;
  it -= 2;
  if (*it != 20 || it[2] != 40) return false;
  V e;
  if (e.begin() != e.end() || e.cbegin() != e.cend() || e.rbegin() != e.rend()) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
