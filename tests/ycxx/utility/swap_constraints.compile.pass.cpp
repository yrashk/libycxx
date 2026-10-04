// [utility.swap]/1: "Constraints: is_move_constructible_v<T> is true and
// is_move_assignable_v<T> is true." /5: the array overload: "Constraints: is_swappable_v<T> is
// true." So std::swap is not viable for other types (observable via is_swappable, which looks
// up swap in a context that includes std::swap).
#include <utility>
#include <type_traits>

struct NoMoveCtor {
  NoMoveCtor(NoMoveCtor&&) = delete;
  NoMoveCtor& operator=(NoMoveCtor&&) = default;
};
struct NoMoveAssign {
  NoMoveAssign(NoMoveAssign&&) = default;
  NoMoveAssign& operator=(NoMoveAssign&&) = delete;
};
struct CopyOnly {
  CopyOnly(const CopyOnly&) = default;
  CopyOnly& operator=(const CopyOnly&) = default;
};

template <class T>
concept std_swappable = requires(T& a, T& b) { std::swap(a, b); };

static_assert(std_swappable<int>);
static_assert(std_swappable<CopyOnly>);
static_assert(!std_swappable<NoMoveCtor>);
static_assert(!std_swappable<NoMoveAssign>);
static_assert(!std_swappable<const int>);
static_assert(std_swappable<int[4]>);
static_assert(!std_swappable<NoMoveCtor[2]>);
static_assert(std_swappable<int[2][3]>);
static_assert(!std::is_swappable_v<NoMoveCtor>);
static_assert(!std::is_swappable_v<NoMoveAssign[3]>);
static_assert(std::is_nothrow_swappable_v<int[5]>);
// arrays of different extents are not swappable with each other
static_assert(!std::is_swappable_with_v<int (&)[2], int (&)[3]>);
