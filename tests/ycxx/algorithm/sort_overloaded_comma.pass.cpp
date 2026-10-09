// [sort] accepts Cpp17RandomAccessIterator; ranges::sort accepts sortable random-access
// iterators. [random.access.iterators] and [iterator.concept.contiguous] impose no requirement
// on a comma expression involving an iterator. Algorithm loop bookkeeping must use the
// built-in comma operator rather than select an unrelated program-provided overload.
#include <algorithm>
#include <compare>
#include <cstddef>
#include <iterator>
#include "sort_support.hpp"
#include "check.hpp"

struct ContiguousIter {
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept = std::contiguous_iterator_tag;
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using pointer = int*;
  using reference = int&;
  int* p = nullptr;
  constexpr reference operator*() const { return *p; }
  constexpr pointer operator->() const { return p; }
  constexpr reference operator[](difference_type n) const { return p[n]; }
  constexpr ContiguousIter& operator++() { ++p; return *this; }
  constexpr ContiguousIter operator++(int) { auto old = *this; ++*this; return old; }
  constexpr ContiguousIter& operator--() { --p; return *this; }
  constexpr ContiguousIter operator--(int) { auto old = *this; --*this; return old; }
  constexpr ContiguousIter& operator+=(difference_type n) { p += n; return *this; }
  constexpr ContiguousIter& operator-=(difference_type n) { p -= n; return *this; }
  friend constexpr ContiguousIter operator+(ContiguousIter i, difference_type n) { return i += n; }
  friend constexpr ContiguousIter operator+(difference_type n, ContiguousIter i) { return i += n; }
  friend constexpr ContiguousIter operator-(ContiguousIter i, difference_type n) { return i -= n; }
  friend constexpr difference_type operator-(ContiguousIter a, ContiguousIter b) { return a.p - b.p; }
  friend constexpr auto operator<=>(ContiguousIter, ContiguousIter) = default;
  friend void operator,(int, const ContiguousIter&) = delete;
};
static_assert(std::contiguous_iterator<ContiguousIter>);
static_assert(std::sortable<ContiguousIter>);

int main() {
  constexpr int n = 300;
  int a[n], original[n];
  for (Pattern pattern : all_patterns) {
    fill_pattern(a, n, pattern);
    for (int i = 0; i < n; ++i) original[i] = a[i];
    std::sort(ContiguousIter{a}, ContiguousIter{a + n});
    CHECK(sorted_by(a, a + n) && same_multiset(a, original, n));
    fill_pattern(a, n, pattern);
    auto end = std::ranges::sort(ContiguousIter{a}, ContiguousIter{a + n});
    CHECK(end.p == a + n && sorted_by(a, a + n) && same_multiset(a, original, n));
  }
}
