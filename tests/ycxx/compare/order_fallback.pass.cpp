// [cmp.alg]/1-6: strong_order, weak_order, partial_order and the compare_*_order_fallback
// customization point objects: ADL customization, mixed-type rejection, and the ==/<
// fallback expressions.
#include <compare>
#include <type_traits>
#include "check.hpp"

// Only == and < (no <=>).
struct OnlyLess {
  int v;
  friend constexpr bool operator==(OnlyLess a, OnlyLess b) { return a.v == b.v; }
  friend constexpr bool operator<(OnlyLess a, OnlyLess b) { return a.v < b.v; }
};

// == and <, where values with v < 0 are "unordered" with everything else.
struct Partial {
  int v;
  friend constexpr bool operator==(Partial a, Partial b) { return a.v >= 0 && b.v >= 0 && a.v == b.v; }
  friend constexpr bool operator<(Partial a, Partial b) { return a.v >= 0 && b.v >= 0 && a.v < b.v; }
};

// ADL customization of strong_order: reversed order. ([cmp.alg]/1.2)
namespace adl {
  struct Rev {
    int v;
    friend constexpr std::strong_ordering strong_order(Rev a, Rev b) { return b.v <=> a.v; }
    friend constexpr std::weak_ordering weak_order(Rev a, Rev b) { return b.v <=> a.v; }
    friend constexpr std::partial_ordering partial_order(Rev a, Rev b) { return b.v <=> a.v; }
    constexpr auto operator<=>(const Rev&) const = default;
  };
} // namespace adl

// Has <=> returning strong_ordering.
struct Spaceship {
  int v;
  constexpr auto operator<=>(const Spaceship&) const = default;
};

template <class T, class U> concept StrongOrd = requires(T t, U u) { std::strong_order(t, u); };
template <class T, class U> concept WeakOrd = requires(T t, U u) { std::weak_order(t, u); };
template <class T, class U> concept PartialOrd = requires(T t, U u) { std::partial_order(t, u); };
template <class T, class U> concept StrongFB = requires(T t, U u) { std::compare_strong_order_fallback(t, u); };
template <class T, class U> concept WeakFB = requires(T t, U u) { std::compare_weak_order_fallback(t, u); };
template <class T, class U> concept PartialFB = requires(T t, U u) { std::compare_partial_order_fallback(t, u); };

// If the decayed types of E and F differ, the expression is ill-formed (SFINAE-friendly).
static_assert(!StrongOrd<int, long> && !WeakOrd<int, long> && !PartialOrd<int, long>);
static_assert(!StrongOrd<double, float> && !WeakOrd<float, double> && !PartialOrd<double, int>);
static_assert(!StrongFB<int, long> && !WeakFB<int, long> && !PartialFB<int, long>);
static_assert(StrongOrd<const int&, int> && WeakOrd<int&, const int>);
// OnlyLess has no <=>: the plain CPOs are ill-formed, the fallbacks work.
static_assert(!StrongOrd<OnlyLess, OnlyLess> && !WeakOrd<OnlyLess, OnlyLess> && !PartialOrd<OnlyLess, OnlyLess>);
static_assert(StrongFB<OnlyLess, OnlyLess> && WeakFB<OnlyLess, OnlyLess> && PartialFB<OnlyLess, OnlyLess>);
static_assert(std::is_same_v<decltype(std::compare_strong_order_fallback(OnlyLess{}, OnlyLess{})), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::compare_weak_order_fallback(OnlyLess{}, OnlyLess{})), std::weak_ordering>);
static_assert(std::is_same_v<decltype(std::compare_partial_order_fallback(OnlyLess{}, OnlyLess{})), std::partial_ordering>);
// Types: strong_order of a <=>-strong type, weak/partial convert.
static_assert(std::is_same_v<decltype(std::strong_order(Spaceship{}, Spaceship{})), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::weak_order(Spaceship{}, Spaceship{})), std::weak_ordering>);
static_assert(std::is_same_v<decltype(std::partial_order(Spaceship{}, Spaceship{})), std::partial_ordering>);
// Partial-only types: <=> yields partial_ordering, so strong_order/weak_order are ill-formed.
struct PartialSpaceship {
  double v;
  constexpr auto operator<=>(const PartialSpaceship&) const = default;
};
static_assert(!StrongOrd<PartialSpaceship, PartialSpaceship> && !WeakOrd<PartialSpaceship, PartialSpaceship>);
static_assert(PartialOrd<PartialSpaceship, PartialSpaceship>);

constexpr bool test() {
  using SO = std::strong_ordering;
  using WO = std::weak_ordering;
  using PO = std::partial_ordering;
  // Fallback expressions (/4.3, /5.3, /6.3).
  if (std::compare_strong_order_fallback(OnlyLess{1}, OnlyLess{2}) != SO::less) return false;
  if (std::compare_strong_order_fallback(OnlyLess{2}, OnlyLess{2}) != SO::equal) return false;
  if (std::compare_strong_order_fallback(OnlyLess{3}, OnlyLess{2}) != SO::greater) return false;
  if (std::compare_weak_order_fallback(OnlyLess{1}, OnlyLess{2}) != WO::less) return false;
  if (std::compare_weak_order_fallback(OnlyLess{2}, OnlyLess{2}) != WO::equivalent) return false;
  if (std::compare_weak_order_fallback(OnlyLess{3}, OnlyLess{2}) != WO::greater) return false;
  if (std::compare_partial_order_fallback(Partial{1}, Partial{2}) != PO::less) return false;
  if (std::compare_partial_order_fallback(Partial{2}, Partial{2}) != PO::equivalent) return false;
  if (std::compare_partial_order_fallback(Partial{3}, Partial{2}) != PO::greater) return false;
  if (std::compare_partial_order_fallback(Partial{-1}, Partial{2}) != PO::unordered) return false;
  if (std::compare_partial_order_fallback(Partial{-1}, Partial{-1}) != PO::unordered) return false;
  // Fallbacks prefer the CPOs when those are well-formed (/4.2, /5.2, /6.2).
  if (std::compare_strong_order_fallback(-0.0, 0.0) != SO::less) return false;      // strong_order, not ==
  if (std::compare_weak_order_fallback(-0.0, 0.0) != WO::equivalent) return false;
  if (std::compare_partial_order_fallback(0.0 / 1.0, 1.0) != PO::less) return false;
  if (std::compare_strong_order_fallback(adl::Rev{1}, adl::Rev{2}) != SO::greater) return false;
  if (std::compare_weak_order_fallback(adl::Rev{1}, adl::Rev{2}) != WO::greater) return false;
  if (std::compare_partial_order_fallback(adl::Rev{1}, adl::Rev{2}) != PO::greater) return false;
  // ADL customizations take priority over <=> (/1.2, /2.2, /3.2).
  if (std::strong_order(adl::Rev{1}, adl::Rev{2}) != SO::greater) return false;
  if (std::weak_order(adl::Rev{1}, adl::Rev{2}) != WO::greater) return false;
  if (std::partial_order(adl::Rev{1}, adl::Rev{2}) != PO::greater) return false;
  // Plain integer comparisons.
  if (std::strong_order(1, 2) != SO::less || std::weak_order(2, 1) != WO::greater) return false;
  if (std::partial_order(3, 3) != PO::equivalent) return false;
  if (std::strong_order(Spaceship{5}, Spaceship{5}) != SO::equal) return false;
  if (std::weak_order(Spaceship{5}, Spaceship{6}) != WO::less) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
