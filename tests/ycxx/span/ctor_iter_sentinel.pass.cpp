// [span.cons]/8-12: template<class It, class End> constexpr explicit(extent != dynamic_extent)
// span(It first, End last); "Constraints: ... is_convertible_v<U(*)[], element_type(*)[]> is
// true. It satisfies contiguous_iterator. End satisfies sized_sentinel_for<It>.
// is_convertible_v<End, size_t> is false." "size_ with last - first."
#include <span>
#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include "check.hpp"

// A sized sentinel for int* that is not an iterator itself.
struct Sentinel {
  int* end;
  friend constexpr bool operator==(int* p, Sentinel s) { return p == s.end; }
  friend constexpr std::ptrdiff_t operator-(Sentinel s, int* p) { return s.end - p; }
  friend constexpr std::ptrdiff_t operator-(int* p, Sentinel s) { return p - s.end; }
};
static_assert(std::sized_sentinel_for<Sentinel, int*>);

// A sentinel that is also convertible to size_t: rejected by the constraint, so the
// (It, size_type) constructor is used instead.
struct ConvSentinel {
  int* end;
  constexpr operator std::size_t() const { return 1; }
  friend constexpr bool operator==(int* p, ConvSentinel s) { return p == s.end; }
  friend constexpr std::ptrdiff_t operator-(ConvSentinel s, int* p) { return s.end - p; }
  friend constexpr std::ptrdiff_t operator-(int* p, ConvSentinel s) { return p - s.end; }
};
static_assert(std::sized_sentinel_for<ConvSentinel, int*>);

struct Unsized {
  int* end;
  friend constexpr bool operator==(int* p, Unsized s) { return p == s.end; }
};

static_assert(std::is_constructible_v<std::span<int>, int*, int*>);
static_assert(std::is_constructible_v<std::span<const int>, int*, const int*>);
static_assert(std::is_constructible_v<std::span<int>, int*, Sentinel>);
static_assert(!std::is_constructible_v<std::span<int>, int*, Unsized>);
static_assert(!std::is_constructible_v<std::span<int>, const int*, const int*>);
static_assert(std::is_constructible_v<std::span<int, 2>, int*, int*>);

// explicit(extent != dynamic_extent)
template <class S>
void take(S);
template <class S, class... Args>
concept implicitly = requires(Args... args) { take<S>({args...}); };
static_assert(implicitly<std::span<int>, int*, int*>);
static_assert(!implicitly<std::span<int, 2>, int*, int*>);

constexpr bool test() {
  int a[6] = {0, 1, 2, 3, 4, 5};
  std::span<int> s(a + 1, a + 4);
  if (s.data() != a + 1 || s.size() != 3) return false;
  std::span<int> t(a, Sentinel{a + 6});
  if (t.data() != a || t.size() != 6) return false;
  std::span<int> u(a, ConvSentinel{a + 6});  // treated as a count of 1
  if (u.size() != 1) return false;
  std::span<int, 2> f(a + 2, a + 4);
  if (f.size() != 2 || f[0] != 2) return false;
  std::array<int, 3> arr{};
  std::span<int> g(arr.begin(), arr.end());
  if (g.data() != arr.data() || g.size() != 3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
