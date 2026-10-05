// [optional.iterators]: iterator/const_iterator model contiguous_iterator with value type
// remove_cv_t<T>, reference T& / const T&; begin() refers to the value or is past-the-end;
// end() == begin() + has_value(); all noexcept and constexpr.
// COUNTERPART: libcxx:utilities/optional/optional.iterator/iterator.pass.cpp
#include <optional>
#include <iterator>
#include <type_traits>
#include <utility>
#include "check.hpp"

using O = std::optional<int>;
static_assert(std::contiguous_iterator<O::iterator>);
static_assert(std::contiguous_iterator<O::const_iterator>);
static_assert(std::is_same_v<std::iter_value_t<O::iterator>, int>);
static_assert(std::is_same_v<std::iter_reference_t<O::iterator>, int&>);
static_assert(std::is_same_v<std::iter_reference_t<O::const_iterator>, const int&>);
static_assert(std::is_same_v<std::iter_value_t<std::optional<const int>::iterator>, int>);
static_assert(std::is_same_v<std::iter_reference_t<std::optional<const int>::iterator>, const int&>);
static_assert(std::is_same_v<decltype(std::declval<O&>().begin()), O::iterator>);
static_assert(std::is_same_v<decltype(std::declval<const O&>().begin()), O::const_iterator>);
static_assert(std::is_same_v<decltype(std::declval<O&>().end()), O::iterator>);
static_assert(std::is_same_v<decltype(std::declval<const O&>().end()), O::const_iterator>);
static_assert(std::is_same_v<typename std::iterator_traits<O::iterator>::iterator_category,
                             std::random_access_iterator_tag>);
static_assert(noexcept(std::declval<O&>().begin()) && noexcept(std::declval<const O&>().end()));
// const_iterator is constructible from iterator (container iterator requirement)
static_assert(std::is_convertible_v<O::iterator, O::const_iterator>);

constexpr bool test() {
  O o(3), e;
  if (o.end() - o.begin() != 1 || e.end() - e.begin() != 0) return false;
  if (e.begin() != e.end()) return false;
  if (&*o.begin() != &*o) return false;
  int sum = 0;
  for (int& x : o) { x += 1; sum += x; }
  for (int x : e) sum += 100 + x;
  if (sum != 4 || *o != 4) return false;
  const O& co = o;
  if (*co.begin() != 4 || co.end() != co.begin() + 1) return false;
  O::const_iterator ci = o.begin();
  if (ci != co.begin()) return false;
  if (std::to_address(o.begin()) != &*o) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
