// [range.access.general]/1: the [range.access] CPOs are available via <iterator>.
// [range.access.cbegin]: ranges::cbegin(E) is const_iterator<decltype(U)>(U) with
// U = ranges::begin(possibly-const-range(t)); ill-formed for an rvalue of a
// non-borrowed range. [range.access.cend]: likewise with const_sentinel.
// [range.prim.cdata]: ranges::cdata returns a pointer to const.
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include "check.hpp"

// Shallow-const view-like range: begin() const returns a mutable iterator.
struct Shallow {
  int* b;
  int* e;
  constexpr int* begin() const { return b; }
  constexpr int* end() const { return e; }
  constexpr int* data() const { return b; }
};
// Deep-const container-like range.
struct Deep {
  int a[3] = {1, 2, 3};
  constexpr int* begin() { return a; }
  constexpr int* end() { return a + 3; }
  constexpr const int* begin() const { return a; }
  constexpr const int* end() const { return a + 3; }
};
// Borrowed range.
struct Borrowed {
  int* b;
  int* e;
  constexpr int* begin() const { return b; }
  constexpr int* end() const { return e; }
};
template <> inline constexpr bool std::ranges::enable_borrowed_range<Borrowed> = true;

template <class R> concept CanCBegin = requires(R&& r) { std::ranges::cbegin(static_cast<R&&>(r)); };
template <class R> concept CanCEnd = requires(R&& r) { std::ranges::cend(static_cast<R&&>(r)); };

static_assert(std::is_same_v<decltype(std::ranges::cbegin(std::declval<int(&)[3]>())), const int*>);
static_assert(std::is_same_v<decltype(std::ranges::cend(std::declval<int(&)[3]>())), const int*>);
static_assert(std::is_same_v<decltype(std::ranges::cbegin(std::declval<Shallow&>())), std::basic_const_iterator<int*>>);
static_assert(std::is_same_v<decltype(std::ranges::cend(std::declval<Shallow&>())), std::basic_const_iterator<int*>>);
static_assert(std::is_same_v<decltype(std::ranges::cbegin(std::declval<Deep&>())), const int*>);
static_assert(std::is_same_v<decltype(std::ranges::cbegin(std::declval<const Deep&>())), const int*>);
static_assert(std::is_same_v<decltype(std::ranges::cdata(std::declval<Shallow&>())), const int*>);
static_assert(std::is_same_v<decltype(std::ranges::cbegin(std::declval<Borrowed>())), std::basic_const_iterator<int*>>);
// Rvalues of non-borrowed ranges are rejected.
static_assert(CanCBegin<Shallow&> && !CanCBegin<Shallow> && !CanCBegin<Shallow&&>);
static_assert(CanCEnd<Deep&> && !CanCEnd<Deep>);
static_assert(CanCBegin<Borrowed> && CanCEnd<Borrowed&&>);
static_assert(!CanCBegin<int(&&)[3]>);
static_assert(!CanCBegin<int>);

constexpr bool test() {
  int a[4] = {1, 2, 3, 4};
  Shallow s{a, a + 4};
  auto b = std::ranges::cbegin(s);
  auto e = std::ranges::cend(s);
  int sum = 0;
  for (; b != e; ++b) sum += *b;
  if (sum != 10) return false;
  if (std::ranges::cbegin(s).base() != a || std::ranges::cend(s).base() != a + 4) return false;
  if (std::ranges::cdata(s) != a) return false;
  Deep d;
  if (std::ranges::cbegin(d) != d.a || std::ranges::cend(d) != d.a + 3) return false;
  if (std::ranges::cbegin(a) != a || std::ranges::cend(a) != a + 4) return false;
  if (*std::ranges::cbegin(Borrowed{a + 1, a + 4}) != 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
