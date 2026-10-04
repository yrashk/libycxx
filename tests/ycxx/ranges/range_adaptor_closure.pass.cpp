// [range.adaptor.object]: C(R) and R | C are equivalent for a range adaptor closure object;
// C | D is a closure whose call pattern is d(c(arg)) (/1); a class deriving from
// range_adaptor_closure<T> is a closure (/2); adaptor(args...) binds the extra arguments
// (/8); closures are not invocable with a non-range.
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

// A program-defined closure: views::take(2) | views::reverse, written by hand.
struct FirstTwoReversed : rg::range_adaptor_closure<FirstTwoReversed> {
  template <rg::viewable_range R>
  constexpr auto operator()(R&& r) const {
    return std::forward<R>(r) | vw::take(2) | vw::reverse;
  }
};
inline constexpr FirstTwoReversed first_two_reversed{};

template <class C, class R>
concept pipeable = requires(R&& r, C c) { std::forward<R>(r) | c; };
static_assert(pipeable<decltype(vw::reverse), int (&)[3]>);
static_assert(!pipeable<decltype(vw::reverse), int>);
static_assert(pipeable<FirstTwoReversed, int (&)[3]>);

constexpr bool test() {
  int a[5] = {1, 2, 3, 4, 5};
  // C(R) and R | C.
  CHECK(range_equals(vw::reverse(a), {5, 4, 3, 2, 1}));
  CHECK(range_equals(a | vw::reverse, {5, 4, 3, 2, 1}));
  // Partial application.
  auto t3 = vw::take(3);
  CHECK(range_equals(a | t3, {1, 2, 3}) && range_equals(t3(a), {1, 2, 3}));
  // Composition: (C | D)(R) == D(C(R)).
  auto comp = vw::drop(1) | vw::transform([](int x) { return x * 10; }) | vw::take(2);
  CHECK(range_equals(a | comp, {20, 30}) && range_equals(comp(a), {20, 30}));
  auto assoc1 = (vw::reverse | vw::take(2)) | vw::reverse;
  auto assoc2 = vw::reverse | (vw::take(2) | vw::reverse);
  CHECK(range_equals(a | assoc1, {4, 5}) && range_equals(a | assoc2, {4, 5}));
  // Program-defined closure, alone and composed with standard ones.
  CHECK(range_equals(a | first_two_reversed, {2, 1}));
  CHECK(range_equals(first_two_reversed(a), {2, 1}));
  auto mixed = vw::drop(2) | first_two_reversed | vw::transform([](int x) { return -x; });
  CHECK(range_equals(a | mixed, {-4, -3}));
  // Bound arguments are copies (decay_t).
  int n = 2;
  auto dn = vw::drop(n);
  n = 4;
  CHECK(range_equals(a | dn, {3, 4, 5}));
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
