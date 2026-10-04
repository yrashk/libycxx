// [span.cons]/16-20: template<class R> constexpr explicit(extent != dynamic_extent) span(R&& r);
// "Constraints: ... R satisfies ranges::contiguous_range and ranges::sized_range. Either R
// satisfies ranges::borrowed_range or is_const_v<element_type> is true. remove_cvref_t<R> is
// not a specialization of span. remove_cvref_t<R> is not a specialization of array.
// is_array_v<remove_cvref_t<R>> is false. is_convertible_v<U(*)[], element_type(*)[]> is
// true." "Effects: Initializes data_ with ranges::data(r) and size_ with ranges::size(r)."
#include <span>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

// A minimal owning contiguous sized range (not a borrowed range).
struct Vec {
  int buf[4] = {1, 2, 3, 4};
  std::size_t n = 4;
  constexpr int* begin() { return buf; }
  constexpr int* end() { return buf + n; }
  constexpr const int* begin() const { return buf; }
  constexpr const int* end() const { return buf + n; }
  constexpr int* data() { return buf; }
  constexpr const int* data() const { return buf; }
  constexpr std::size_t size() const { return n; }
};
// A contiguous range that is not sized? (contiguous_range with sized_sentinel is always
// sized, so use a non-contiguous range instead.)
struct Linked {
  struct It {
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    int* p;
    int& operator*() const;
    It& operator++();
    It operator++(int);
    bool operator==(const It&) const;
  };
  It begin();
  It end();
};

static_assert(std::is_constructible_v<std::span<int>, Vec&>);
static_assert(std::is_constructible_v<std::span<const int>, Vec&>);
static_assert(std::is_constructible_v<std::span<const int>, const Vec&>);
static_assert(!std::is_constructible_v<std::span<int>, const Vec&>);
// rvalue of a non-borrowed range: only spans of const elements
static_assert(!std::is_constructible_v<std::span<int>, Vec&&>);
static_assert(std::is_constructible_v<std::span<const int>, Vec&&>);
static_assert(!std::is_constructible_v<std::span<long>, Vec&>);
static_assert(!std::is_constructible_v<std::span<int>, Linked&>);
// static extent: explicit
static_assert(std::is_constructible_v<std::span<int, 4>, Vec&>);
static_assert(!std::is_convertible_v<Vec&, std::span<int, 4>>);
static_assert(std::is_convertible_v<Vec&, std::span<int>>);

constexpr bool test() {
  Vec v;
  std::span<int> s = v;
  if (s.data() != v.buf || s.size() != 4) return false;
  s[0] = 10;
  if (v.buf[0] != 10) return false;
  v.n = 2;
  std::span<const int> c = v;
  if (c.size() != 2) return false;
  std::span<int, 2> f(v);
  if (f.data() != v.buf) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // binding a temporary to span<const int> is allowed (the caller must keep it alive for
  // the duration of the full-expression)
  auto sz = [](std::span<const int> s) { return s.size(); };
  CHECK(sz(Vec{}) == 4);
  return 0;
}
