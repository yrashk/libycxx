// [iterator.range]/2-23: std::begin / end (c.begin(), c.end(); array, array + N),
// cbegin / cend (std::begin / std::end of a const C&), rbegin / rend (c.rbegin(); for
// arrays reverse_iterator<T*>(array + N) / (array); for initializer_list
// reverse_iterator<const E*>(il.end()) / (il.begin())), crbegin / crend, size (c.size(); N
// for arrays), ssize (common_type_t<ptrdiff_t, make_signed_t<decltype(c.size())>>; ptrdiff_t
// N for arrays), empty (c.empty(); false for arrays), data (c.data(); array). All are
// constexpr; the container forms return exactly what the member returns.
#include <iterator>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include "check.hpp"

// A container whose const and non-const members return distinguishable types.
struct Box {
  int a[3] = {1, 2, 3};
  constexpr int* begin() { return a; }
  constexpr const int* begin() const { return a + 0; }
  constexpr int* end() { return a + 3; }
  constexpr const int* end() const { return a + 3; }
  constexpr std::reverse_iterator<int*> rbegin() { return std::reverse_iterator<int*>(a + 3); }
  constexpr std::reverse_iterator<const int*> rbegin() const { return std::reverse_iterator<const int*>(a + 3); }
  constexpr std::reverse_iterator<int*> rend() { return std::reverse_iterator<int*>(a); }
  constexpr std::reverse_iterator<const int*> rend() const { return std::reverse_iterator<const int*>(a); }
  constexpr unsigned short size() const { return 3; }
  constexpr bool empty() const { return false; }
  constexpr int* data() { return a; }
  constexpr const int* data() const { return a; }
};

// Only size(), returning a wide unsigned type.
struct BigSize {
  constexpr unsigned long long size() const { return 5; }
};

Box box;
const Box cbox;
static_assert(std::is_same_v<decltype(std::begin(box)), int*>);
static_assert(std::is_same_v<decltype(std::begin(cbox)), const int*>);
static_assert(std::is_same_v<decltype(std::end(box)), int*>);
static_assert(std::is_same_v<decltype(std::cbegin(box)), const int*>);
static_assert(std::is_same_v<decltype(std::cend(box)), const int*>);
static_assert(std::is_same_v<decltype(std::rbegin(box)), std::reverse_iterator<int*>>);
static_assert(std::is_same_v<decltype(std::rend(cbox)), std::reverse_iterator<const int*>>);
static_assert(std::is_same_v<decltype(std::crbegin(box)), std::reverse_iterator<const int*>>);
static_assert(std::is_same_v<decltype(std::crend(box)), std::reverse_iterator<const int*>>);
static_assert(std::is_same_v<decltype(std::size(box)), unsigned short>);
static_assert(std::is_same_v<decltype(std::ssize(box)), std::ptrdiff_t>);
static_assert(std::is_same_v<decltype(std::ssize(BigSize{})),
                             std::common_type_t<std::ptrdiff_t, long long>>);
static_assert(std::is_same_v<decltype(std::empty(box)), bool>);
static_assert(std::is_same_v<decltype(std::data(box)), int*>);
static_assert(std::is_same_v<decltype(std::data(cbox)), const int*>);

int arr[4] = {10, 20, 30, 40};
static_assert(std::is_same_v<decltype(std::begin(arr)), int*>);
static_assert(std::is_same_v<decltype(std::cbegin(arr)), const int*>);
static_assert(std::is_same_v<decltype(std::rbegin(arr)), std::reverse_iterator<int*>>);
static_assert(std::is_same_v<decltype(std::crbegin(arr)), std::reverse_iterator<const int*>>);
static_assert(std::is_same_v<decltype(std::size(arr)), std::size_t>);
static_assert(std::is_same_v<decltype(std::ssize(arr)), std::ptrdiff_t>);
static_assert(std::is_same_v<decltype(std::empty(arr)), bool>);
static_assert(std::is_same_v<decltype(std::data(arr)), int*>);
static_assert(std::is_same_v<decltype(std::rbegin(std::initializer_list<int>{})),
                             std::reverse_iterator<const int*>>);
static_assert(std::is_same_v<decltype(std::rend(std::initializer_list<int>{})),
                             std::reverse_iterator<const int*>>);

constexpr bool test() {
  Box b;
  const Box& cb = b;
  if (std::begin(b) != b.a || std::end(b) != b.a + 3) return false;
  if (std::begin(cb) != b.a || std::cend(b) != b.a + 3) return false;
  if (*std::rbegin(b) != 3 || *(std::rend(b) - 1) != 1 || *std::crbegin(b) != 3) return false;
  if (std::size(b) != 3 || std::ssize(b) != 3 || std::empty(b) || std::data(b) != b.a) return false;
  int a[4] = {10, 20, 30, 40};
  if (std::begin(a) != a || std::end(a) != a + 4 || std::cbegin(a) != a || std::cend(a) != a + 4) return false;
  if (*std::rbegin(a) != 40 || std::rbegin(a).base() != a + 4 || std::rend(a).base() != a) return false;
  if (*std::crbegin(a) != 40 || std::crend(a).base() != a) return false;
  if (std::size(a) != 4 || std::ssize(a) != 4 || std::empty(a) || std::data(a) != a) return false;
  const int ca[2] = {1, 2};
  if (std::size(ca) != 2 || std::data(ca) != ca || *std::begin(ca) != 1) return false;
  std::initializer_list<int> il = {7, 8, 9};
  if (*std::rbegin(il) != 9 || std::rbegin(il).base() != il.end()) return false;
  if (std::rend(il).base() != il.begin()) return false;
  if (std::begin(il) != il.begin() || std::size(il) != 3 || std::data(il) != il.begin()) return false;
  if (std::empty(il) || !std::empty(std::initializer_list<int>{})) return false;
  if (std::ssize(BigSize{}) != 5) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
