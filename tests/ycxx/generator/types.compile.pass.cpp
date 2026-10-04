// [coro.generator.class]: generator<Ref, Val, Allocator> with
//   value     = conditional_t<is_void_v<Val>, remove_cvref_t<Ref>, Val>
//   reference = conditional_t<is_void_v<Val>, Ref&&, Ref>
//   yielded   = conditional_t<is_reference_v<reference>, reference, const reference&>
// [coro.generator.class]/3: specializations model view and input_range.
// [coro.generator.iterator]: value_type = value, difference_type = ptrdiff_t, operator* returns
// reference and is noexcept(is_nothrow_copy_constructible_v<reference>) (at least); the iterator is
// move-only (only the move constructor and move assignment are declared).
// [coro.generator.members]: copy constructor deleted, noexcept move constructor and assignment,
// end() returns default_sentinel_t. [generator.syn]: pmr::generator.
#include <generator>
#include <coroutine>
#include <cstddef>
#include <iterator>
#include <memory_resource>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

template <class G, class Value, class Reference, class Yielded>
constexpr bool check() {
  using It = std::ranges::iterator_t<G>;
  static_assert(std::is_same_v<typename G::yielded, Yielded>);
  static_assert(std::is_same_v<std::ranges::range_value_t<G>, Value>);
  static_assert(std::is_same_v<std::ranges::range_reference_t<G>, Reference>);
  static_assert(std::is_same_v<std::iter_value_t<It>, Value>);
  static_assert(std::is_same_v<typename It::value_type, Value>);
  static_assert(std::is_same_v<typename It::difference_type, std::ptrdiff_t>);
  static_assert(std::is_same_v<decltype(*std::declval<It&>()), Reference>);
  static_assert(std::is_same_v<decltype(*std::declval<const It&>()), Reference>);
  // [res.on.exception.handling]/5 allows adding noexcept, so only the required direction is checked.
  static_assert(!std::is_nothrow_copy_constructible_v<Reference> || noexcept(*std::declval<const It&>()));
  static_assert(std::input_iterator<It>);
  static_assert(!std::forward_iterator<It>);
  static_assert(std::movable<It> && !std::copyable<It>);
  static_assert(!std::is_copy_constructible_v<It> && !std::is_copy_assignable_v<It>);
  static_assert(std::is_nothrow_move_constructible_v<It> && std::is_nothrow_move_assignable_v<It>);
  static_assert(std::is_same_v<decltype(++std::declval<It&>()), It&>);
  static_assert(std::is_same_v<decltype(std::declval<It&>()++), void>);
  static_assert(std::is_same_v<std::ranges::sentinel_t<G>, std::default_sentinel_t>);
  static_assert(std::is_same_v<decltype(std::declval<const G&>().end()), std::default_sentinel_t>);
  static_assert(noexcept(std::declval<const G&>().end()));
  static_assert(std::sentinel_for<std::default_sentinel_t, It>);
  static_assert(std::ranges::input_range<G> && std::ranges::view<G>);
  static_assert(!std::ranges::forward_range<G> && !std::ranges::common_range<G>);
  static_assert(!std::ranges::sized_range<G> && !std::ranges::borrowed_range<G>);
  static_assert(!std::ranges::range<const G>);  // begin() is non-const
  static_assert(std::derived_from<G, std::ranges::view_interface<G>>);
  static_assert(!std::is_copy_constructible_v<G> && !std::is_copy_assignable_v<G>);
  static_assert(std::is_nothrow_move_constructible_v<G> && std::is_nothrow_move_assignable_v<G>);
  static_assert(!std::is_default_constructible_v<G>);
  return true;
}

struct MoveOnly {
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;
};

static_assert(check<std::generator<int>, int, int&&, int&&>());
static_assert(check<std::generator<const int&>, int, const int&, const int&>());
static_assert(check<std::generator<int&>, int, int&, int&>());
static_assert(check<std::generator<int&&>, int, int&&, int&&>());
static_assert(check<std::generator<const int>, int, const int&&, const int&&>());
static_assert(check<std::generator<std::string>, std::string, std::string&&, std::string&&>());
static_assert(check<std::generator<MoveOnly>, MoveOnly, MoveOnly&&, MoveOnly&&>());
// Val not void: reference is Ref itself; a prvalue reference makes yielded const Ref&.
static_assert(check<std::generator<int, int>, int, int, const int&>());
static_assert(check<std::generator<std::string_view, std::string>, std::string, std::string_view,
                    const std::string_view&>());
static_assert(check<std::generator<const std::string&, std::string>, std::string, const std::string&,
                    const std::string&>());
static_assert(check<std::generator<std::vector<int>&, std::vector<int>>, std::vector<int>, std::vector<int>&,
                    std::vector<int>&>());
struct ThrowingCopy {
  ThrowingCopy(const ThrowingCopy&) noexcept(false);
};
static_assert(check<std::generator<ThrowingCopy, ThrowingCopy>, ThrowingCopy, ThrowingCopy,
                    const ThrowingCopy&>());

// [generator.syn]: pmr::generator<R, V> = generator<R, V, polymorphic_allocator<>>.
static_assert(std::is_same_v<std::pmr::generator<int>, std::generator<int, void, std::pmr::polymorphic_allocator<>>>);
static_assert(std::is_same_v<std::pmr::generator<int&, long>,
                             std::generator<int&, long, std::pmr::polymorphic_allocator<>>>);
static_assert(check<std::pmr::generator<int>, int, int&&, int&&>());

// [coro.generator.promise]: promise_type is a public member; the coroutine traits find it.
using P = std::generator<int>::promise_type;
static_assert(std::is_same_v<std::coroutine_traits<std::generator<int>, int>::promise_type, P>);
static_assert(std::is_same_v<decltype(std::declval<P&>().get_return_object()), std::generator<int>>);
static_assert(noexcept(std::declval<P&>().get_return_object()));
static_assert(std::is_same_v<decltype(std::declval<const P&>().initial_suspend()), std::suspend_always>);
static_assert(noexcept(std::declval<const P&>().initial_suspend()));
static_assert(noexcept(std::declval<P&>().final_suspend()));
static_assert(std::is_same_v<decltype(std::declval<P&>().yield_value(1)), std::suspend_always>);
static_assert(noexcept(std::declval<P&>().yield_value(1)));
static_assert(std::is_same_v<decltype(std::declval<const P&>().return_void()), void>);
static_assert(noexcept(std::declval<const P&>().return_void()));
// The const& overload of yield_value exists only for rvalue-reference yielded types whose value
// type is copy-constructible from a const lvalue.
template <class Pr, class A>
concept can_yield = requires(Pr& p, A&& a) { p.yield_value(static_cast<A&&>(a)); };
static_assert(can_yield<P, const int&> && can_yield<P, int&> && can_yield<P, int>);
using PM = std::generator<MoveOnly>::promise_type;
static_assert(can_yield<PM, MoveOnly> && !can_yield<PM, const MoveOnly&> && !can_yield<PM, MoveOnly&>);
using PR = std::generator<int&>::promise_type;  // yielded = int&: no const& overload
static_assert(can_yield<PR, int&> && !can_yield<PR, const int&> && !can_yield<PR, int>);
// [coro.generator.promise]: yield_value(elements_of<R, Alloc>) requires
// convertible_to<range_reference_t<R>, yielded>: an lvalue int& does not convert to int&&.
using EV = std::ranges::elements_of<std::vector<int>&>;
using EI = std::ranges::elements_of<std::ranges::iota_view<int, int>>;
using ER = std::ranges::elements_of<std::vector<int>&&>;
static_assert(!can_yield<P, EV> && can_yield<P, EI> && !can_yield<P, ER>);
using PC = std::generator<const int&>::promise_type;
static_assert(can_yield<PC, EV> && can_yield<PC, EI> && can_yield<PC, ER>);
static_assert(can_yield<PR, EV> && !can_yield<PR, EI>);

int main() {}
