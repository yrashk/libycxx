// [deque.overview]/1: "A deque is a sequence container that supports random access
// iterators"; /3: iterator and const_iterator meet the constexpr iterator requirements.
// [random.access.iterators] / [iterator.concept.random.access]: a += n, a + n, n + a,
// a -= n, a - n, b - a, a[n], <, >, <=, >= behave as n increments/decrements. Checked over a
// large deque grown at both ends (so iterators cross internal block boundaries in both
// directions), for every distance, plus the range concepts ([range.refinements]).
#include <deque>
#include <cstddef>
#include <iterator>
#include <ranges>
#include "container_values.hpp"
#include "check.hpp"

static_assert(std::random_access_iterator<std::deque<int>::iterator>);
static_assert(std::random_access_iterator<std::deque<int>::const_iterator>);
static_assert(std::sentinel_for<std::deque<int>::iterator, std::deque<int>::iterator>);
static_assert(std::sized_sentinel_for<std::deque<int>::const_iterator, std::deque<int>::iterator>);
static_assert(std::ranges::random_access_range<std::deque<int>>);
static_assert(std::ranges::sized_range<std::deque<int>>);
static_assert(std::ranges::common_range<std::deque<int>>);
static_assert(std::ranges::random_access_range<const std::deque<int>>);
static_assert(std::same_as<std::iterator_traits<std::deque<int>::iterator>::iterator_category,
                           std::random_access_iterator_tag>);

template <class T>
constexpr bool test(int n) {
  std::deque<T> d;
  for (int i = 0; i < n; ++i) {
    if (i % 3 == 0) d.push_front(val<T>(i % 80));
    else d.push_back(val<T>(i % 80));
  }
  const std::ptrdiff_t sz = static_cast<std::ptrdiff_t>(d.size());
  // walk with ++ to record every element's address
  const T* addr[1200] = {};
  {
    std::ptrdiff_t k = 0;
    for (auto it = d.begin(); it != d.end(); ++it) addr[k++] = &*it;
    if (k != sz) return false;
    for (auto it = d.end(); it != d.begin();) {
      --it;
      if (&*it != addr[--k]) return false;
    }
  }
  auto b = d.begin();
  auto cb = d.cbegin();
  for (std::ptrdiff_t i = 0; i <= sz; i += 7) {
    for (std::ptrdiff_t j = 0; j <= sz; j += 11) {
      auto a = b + i;
      auto c = cb + j;
      if (c - a != j - i || a - c != i - j) return false;
      if ((a < c) != (i < j) || (a > c) != (i > j) || (a <= c) != (i <= j) || (a >= c) != (i >= j)) return false;
      if ((a == c) != (i == j)) return false;
      auto t = a;
      t += j - i;
      if (t != c) return false;
      t -= j - i;
      if (t != a) return false;
      if ((a + (j - i)) != c || ((j - i) + a) != c || (c - (j - i)) != a) return false;
      if (j < sz && i < sz && &a[j - i] != addr[j]) return false;
    }
    if (i < sz && &*(b + i) != addr[i]) return false;
  }
  if (d.end() - d.begin() != sz || d.begin() + sz != d.end() || d.end() - sz != d.begin()) return false;
  return true;
}

int main() {
  CHECK(test<int>(1));
  CHECK(test<int>(1100));
  CHECK(test<Elem>(600));
  CHECK(test<long double>(1000));
  return 0;
}
