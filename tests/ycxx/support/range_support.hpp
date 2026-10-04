// Helpers for the <ranges> tests of libycxx's own suite, written from [range.req] and
// [range.adaptors]. Independent of every other test suite.
#pragma once
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

// Element-wise comparison of a range with a list, by plain iteration (no <algorithm>).
template <class R, class T>
constexpr bool range_equals(R&& r, std::initializer_list<T> il) {
  auto i = std::ranges::begin(r);
  auto e = std::ranges::end(r);
  const T* p = il.begin();
  for (; i != e; ++i, ++p) {
    if (p == il.end()) return false;
    if (!(*i == *p)) return false;
  }
  return p == il.end();
}

// Number of elements reached by iteration.
template <class R>
constexpr std::ptrdiff_t count_elements(R&& r) {
  std::ptrdiff_t n = 0;
  auto e = std::ranges::end(r);
  for (auto i = std::ranges::begin(r); i != e; ++i) ++n;
  return n;
}

// Whether I has a member type iterator_category (the [range.adaptors] iterators define it
// only under conditions).
template <class I>
concept has_iterator_category = requires { typename I::iterator_category; };

// A view over a pointer range whose iterator strength and commonness are chosen by the
// archetypes of test_iterators.hpp; see the aliases in the individual tests.
template <class It, class Sent = It>
struct ArchetypeView : std::ranges::view_base {
  It b{};
  Sent e{};
  ArchetypeView() = default;
  constexpr ArchetypeView(It b_, Sent e_) : b(b_), e(e_) {}
  constexpr It begin() const { return b; }
  constexpr Sent end() const { return e; }
};

// A view that can only be iterated when non-const (begin()/end() are non-const members).
template <class T>
struct MutableOnlyView : std::ranges::view_base {
  T* b = nullptr;
  T* e = nullptr;
  MutableOnlyView() = default;
  constexpr MutableOnlyView(T* b_, T* e_) : b(b_), e(e_) {}
  constexpr T* begin() { return b; }
  constexpr T* end() { return e; }
};

// A move-only view over a pointer range.
template <class T>
struct MoveOnlyView : std::ranges::view_base {
  T* b = nullptr;
  T* e = nullptr;
  MoveOnlyView() = default;
  constexpr MoveOnlyView(T* b_, T* e_) : b(b_), e(e_) {}
  MoveOnlyView(MoveOnlyView&&) = default;
  MoveOnlyView& operator=(MoveOnlyView&&) = default;
  constexpr T* begin() const { return b; }
  constexpr T* end() const { return e; }
};

// A borrowed view over a pointer range.
template <class T>
struct BorrowedView : std::ranges::view_base {
  T* b = nullptr;
  T* e = nullptr;
  BorrowedView() = default;
  constexpr BorrowedView(T* b_, T* e_) : b(b_), e(e_) {}
  constexpr T* begin() const { return b; }
  constexpr T* end() const { return e; }
};
template <class T>
inline constexpr bool std::ranges::enable_borrowed_range<BorrowedView<T>> = true;
