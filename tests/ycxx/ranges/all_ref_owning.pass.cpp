// [range.all], [range.ref.view], [range.owning.view]: views::all yields a copy of a view,
// a ref_view of an lvalue range, or an owning_view of an rvalue range (/2); ref_view binds
// only lvalues ([range.ref.view]/3); owning_view is move-only; borrowed_range follows
// [ranges.syn] (ref_view always, owning_view as its range).
#include <array>
#include <concepts>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"

namespace rg = std::ranges;
using A = std::array<int, 3>;

static_assert(std::is_same_v<std::views::all_t<A&>, rg::ref_view<A>>);
static_assert(std::is_same_v<std::views::all_t<const A&>, rg::ref_view<const A>>);
static_assert(std::is_same_v<std::views::all_t<A>, rg::owning_view<A>>);
static_assert(std::is_same_v<std::views::all_t<A&&>, rg::owning_view<A>>);
static_assert(std::is_same_v<std::views::all_t<int (&)[4]>, rg::ref_view<int[4]>>);
static_assert(std::is_same_v<std::views::all_t<std::string_view>, std::string_view>);
static_assert(std::is_same_v<std::views::all_t<std::span<int>&>, std::span<int>>);
static_assert(std::is_same_v<std::views::all_t<const rg::iota_view<int, int>&>, rg::iota_view<int, int>>);

// Non-viewable: a const rvalue container (neither ref_view nor owning_view applies).
static_assert(!rg::viewable_range<const A>);
template <class R>
concept all_callable = requires { std::views::all(std::declval<R>()); };
static_assert(!all_callable<const A> && all_callable<const A&> && all_callable<A>);
static_assert(rg::viewable_range<A> && rg::viewable_range<A&> && rg::viewable_range<const A&>);

// ref_view
using RV = rg::ref_view<A>;
static_assert(rg::view<RV> && rg::contiguous_range<RV> && rg::sized_range<RV> && rg::borrowed_range<RV>);
static_assert(std::copyable<RV>);
static_assert(std::constructible_from<RV, A&> && !std::constructible_from<RV, A&&> && !std::constructible_from<RV, A>);
static_assert(std::is_convertible_v<A&, RV>);
static_assert(std::is_same_v<decltype(rg::ref_view(std::declval<A&>())), RV>);
static_assert(std::is_same_v<decltype(std::declval<const RV&>().base()), A&>);
static_assert(std::is_same_v<rg::iterator_t<const RV>, A::iterator>); // shallow const

// owning_view
using OV = rg::owning_view<A>;
static_assert(rg::view<OV> && rg::contiguous_range<OV> && rg::sized_range<OV> && rg::common_range<OV>);
static_assert(std::movable<OV> && !std::copyable<OV> && !std::is_copy_constructible_v<OV>);
static_assert(!rg::borrowed_range<OV>);
static_assert(rg::borrowed_range<rg::owning_view<std::span<int>>> == rg::enable_borrowed_range<std::span<int>>);
static_assert(std::is_same_v<rg::iterator_t<const OV>, A::const_iterator>); // deep const
static_assert(std::is_same_v<decltype(std::declval<OV&>().base()), A&>);
static_assert(std::is_same_v<decltype(std::declval<const OV&>().base()), const A&>);
static_assert(std::is_same_v<decltype(std::declval<OV>().base()), A&&>);
static_assert(std::is_same_v<decltype(std::declval<const OV>().base()), const A&&>);
static_assert(noexcept(std::declval<OV&>().base()) && noexcept(std::declval<OV>().base()));
static_assert(std::default_initializable<OV>);
// Not for initializer_list.
template <class R>
concept owning_ok = requires { typename rg::owning_view<R>; };
static_assert(owning_ok<A> && !owning_ok<std::initializer_list<int>>);

constexpr bool test() {
  A a{1, 2, 3};
  auto r = std::views::all(a);
  CHECK(&r.base() == &a && r.size() == 3u && r.data() == a.data() && !r.empty());
  r.begin()[1] = 20;
  CHECK(a[1] == 20);
  auto o = std::views::all(A{4, 5, 6});
  CHECK(range_equals(o, {4, 5, 6}) && o.size() == 3u && o.base()[0] == 4 && !o.empty());
  auto o2 = std::move(o);
  CHECK(range_equals(o2, {4, 5, 6}));
  A moved = std::move(o2).base();
  CHECK(moved[2] == 6);
  // views::all of a view is a copy.
  std::span<int> sp(a);
  auto sv = std::views::all(sp);
  CHECK(sv.data() == a.data());
  // Pipe form.
  auto p = a | std::views::all;
  CHECK(p.data() == a.data());
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
