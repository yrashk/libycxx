// [any.cons]/6: any(T&&) "Constraints: VT is not the same type as any, VT is not a
// specialization of in_place_type_t, and is_copy_constructible_v<VT> is true."
// [any.cons]/11,17: in_place_type constructors are explicit and constrained on
// is_copy_constructible_v<VT> and is_constructible_v<VT, Args...>.
// [any.assign]/8: operator=(T&&) constrained on VT != any and is_copy_constructible_v<VT>.
// [any.modifiers]/2,10: emplace constraints.
#include <any>
#include <initializer_list>
#include <type_traits>
#include <utility>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;
};
struct FromInt { FromInt(int) {} };
struct FromList { FromList(std::initializer_list<int>, char) {} };

static_assert(!std::is_constructible_v<std::any, MoveOnly>);
static_assert(!std::is_constructible_v<std::any, MoveOnly&>);
static_assert(!std::is_convertible_v<MoveOnly, std::any>);
static_assert(!std::is_assignable_v<std::any&, MoveOnly>);
static_assert(std::is_assignable_v<std::any&, int>);
static_assert(std::is_assignable_v<std::any&, const FromInt&>);

// in_place_type constructors are explicit.
static_assert(std::is_constructible_v<std::any, std::in_place_type_t<int>>);
static_assert(std::is_constructible_v<std::any, std::in_place_type_t<int>, int>);
static_assert(!std::is_convertible_v<std::in_place_type_t<int>, std::any>);
static_assert(std::is_constructible_v<std::any, std::in_place_type_t<FromInt>, int>);
static_assert(!std::is_constructible_v<std::any, std::in_place_type_t<FromInt>>);
static_assert(!std::is_constructible_v<std::any, std::in_place_type_t<FromInt>, int, int>);
static_assert(!std::is_constructible_v<std::any, std::in_place_type_t<MoveOnly>>);
static_assert(std::is_constructible_v<std::any, std::in_place_type_t<FromList>, std::initializer_list<int>, char>);
static_assert(!std::is_constructible_v<std::any, std::in_place_type_t<FromList>, std::initializer_list<int>>);
static_assert(!std::is_constructible_v<std::any, std::in_place_type_t<FromInt>, std::initializer_list<int>>);

template <class T, class... Args>
concept can_emplace = requires(std::any& a, Args&&... args) { a.emplace<T>(std::forward<Args>(args)...); };
static_assert(can_emplace<int>);
static_assert(can_emplace<int, long>);
static_assert(can_emplace<FromInt, int>);
static_assert(!can_emplace<FromInt>);
static_assert(!can_emplace<MoveOnly>);
static_assert(can_emplace<FromList, std::initializer_list<int>, char>);
static_assert(!can_emplace<FromList, std::initializer_list<int>>);

// emplace returns decay_t<T>&.
static_assert(std::is_same_v<decltype(std::declval<std::any&>().emplace<int>(1)), int&>);
static_assert(std::is_same_v<decltype(std::declval<std::any&>().emplace<const int>(1)), int&>);
static_assert(
    std::is_same_v<decltype(std::declval<std::any&>().emplace<FromList>({1, 2}, 'c')), FromList&>);

// Special members.
static_assert(std::is_nothrow_move_assignable_v<std::any>);
static_assert(std::is_copy_assignable_v<std::any>);
static_assert(std::is_nothrow_destructible_v<std::any>);
static_assert(noexcept(std::declval<std::any&>().reset()));
static_assert(noexcept(std::declval<std::any&>().swap(std::declval<std::any&>())));
static_assert(noexcept(swap(std::declval<std::any&>(), std::declval<std::any&>())));
static_assert(std::is_nothrow_swappable_v<std::any>);
