// Values and observers for the generic container-requirement tests in tests/ycxx/containers,
// written from [container.requirements]. Deliberately independent of every other test suite.
// Only iterator operations are used to observe a container, so the helpers work for any
// container (vector, basic_string now; deque, list, forward_list later).
#pragma once
#include <compare>
#include <cstddef>
#include <initializer_list>
#include <type_traits>

// A non-trivial, constexpr-friendly element type with value semantics.
struct Elem {
  int* heap = nullptr;  // owns one int, so copies and destruction are observable
  constexpr Elem() : heap(new int(-1)) {}
  constexpr Elem(int v) : heap(new int(v)) {}
  constexpr Elem(const Elem& o) : heap(new int(*o.heap)) {}
  constexpr Elem(Elem&& o) noexcept : heap(o.heap) { o.heap = nullptr; }
  constexpr Elem& operator=(const Elem& o) {
    if (this != &o) *this = Elem(o);
    return *this;
  }
  constexpr Elem& operator=(Elem&& o) noexcept {
    if (this != &o) {
      delete heap;
      heap = o.heap;
      o.heap = nullptr;
    }
    return *this;
  }
  constexpr ~Elem() { delete heap; }
  constexpr int value() const { return heap ? *heap : -1000; }
  friend constexpr bool operator==(const Elem& a, const Elem& b) { return a.value() == b.value(); }
  friend constexpr std::strong_ordering operator<=>(const Elem& a, const Elem& b) {
    return a.value() <=> b.value();
  }
};

// val<T>(i): a deterministic value of T for index i (0 <= i < 80). Distinct indices give
// distinct values for every T except bool, where val<bool>(i) is i % 2 != 0. Character types
// get printable non-null characters; types constructible from (size_t, char) (strings) get
// a string of length 20 + i, longer than any small-buffer size.
template <class T>
constexpr T val(int i) {
  if constexpr (std::is_same_v<T, bool>) {
    return i % 2 != 0;
  } else if constexpr (std::is_arithmetic_v<T>) {
    return static_cast<T>(40 + i);
  } else if constexpr (std::is_constructible_v<T, std::size_t, char> && !std::is_constructible_v<T, int>) {
    return T(static_cast<std::size_t>(20 + i), static_cast<char>('a' + i % 26));
  } else {
    return T(i);
  }
}

// holds(c, {i0, i1, ...}): c's elements, in iteration order, are val(i0), val(i1), ...
template <class C>
constexpr bool holds(const C& c, std::initializer_list<int> idx) {
  using T = typename C::value_type;
  auto it = c.begin();
  for (int i : idx) {
    if (it == c.end()) return false;
    if (!(*it == val<T>(i))) return false;
    ++it;
  }
  return it == c.end();
}

// make<C>({i0, i1, ...}): a container holding val(i0), val(i1), ... built with insert(end, t).
template <class C>
constexpr C make(std::initializer_list<int> idx) {
  using T = typename C::value_type;
  C c;
  for (int i : idx) c.insert(c.end(), val<T>(i));
  return c;
}

// Number of elements by iteration (container-independent distance).
template <class C>
constexpr std::ptrdiff_t count_elems(const C& c) {
  std::ptrdiff_t n = 0;
  for (auto it = c.begin(); it != c.end(); ++it) ++n;
  return n;
}
