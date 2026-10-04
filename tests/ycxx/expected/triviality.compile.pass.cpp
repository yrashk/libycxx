// [expected.object.cons]/9-10,16: copy ctor deleted unless T and E copy-constructible; copy and
// move ctors trivial when T's and E's are. [expected.object.dtor]/2: trivial dtor.
// [expected.object.assign]/4-5,6,10: copy assignment deleted unless T, E copy-assignable and
// copy-constructible AND (nothrow-move T || nothrow-move E); move assignment constrained
// likewise; triviality conditions.
#include <expected>
#include <type_traits>

struct NTDtor { ~NTDtor() {} };
struct NTCopy { NTCopy() = default; NTCopy(const NTCopy&) {} NTCopy& operator=(const NTCopy&) = default; };
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(const ThrowingMove&) = default;
  ThrowingMove(ThrowingMove&&) noexcept(false) {}
  ThrowingMove& operator=(const ThrowingMove&) = default;
  ThrowingMove& operator=(ThrowingMove&&) = default;
};
struct MoveOnly { MoveOnly() = default; MoveOnly(MoveOnly&&) = default; MoveOnly& operator=(MoveOnly&&) = default; };
struct NoCopyAssign { NoCopyAssign(const NoCopyAssign&) = default; NoCopyAssign& operator=(const NoCopyAssign&) = delete; };

using E = std::expected<int, long>;
static_assert(std::is_trivially_copy_constructible_v<E>);
static_assert(std::is_trivially_move_constructible_v<E>);
static_assert(std::is_trivially_copy_assignable_v<E>);
static_assert(std::is_trivially_move_assignable_v<E>);
static_assert(std::is_trivially_destructible_v<E>);
static_assert(std::is_trivially_copyable_v<E>);

static_assert(!std::is_trivially_destructible_v<std::expected<NTDtor, int>>);
static_assert(!std::is_trivially_destructible_v<std::expected<int, NTDtor>>);
static_assert(!std::is_trivially_copy_constructible_v<std::expected<NTCopy, int>>);
static_assert(!std::is_trivially_copy_constructible_v<std::expected<int, NTCopy>>);
static_assert(!std::is_trivially_copy_assignable_v<std::expected<int, NTCopy>>);
static_assert(!std::is_trivially_copy_assignable_v<std::expected<NTDtor, int>>);

// (4.5)/(6.5): at least one of T, E must be nothrow move constructible
static_assert(std::is_copy_assignable_v<std::expected<ThrowingMove, int>>);
static_assert(std::is_copy_assignable_v<std::expected<int, ThrowingMove>>);
static_assert(!std::is_copy_assignable_v<std::expected<ThrowingMove, ThrowingMove>>);
static_assert(!std::is_move_assignable_v<std::expected<ThrowingMove, ThrowingMove>>);
static_assert(std::is_copy_constructible_v<std::expected<ThrowingMove, ThrowingMove>>);
static_assert(!std::is_copy_assignable_v<std::expected<NoCopyAssign, int>>);
static_assert(!std::is_copy_assignable_v<std::expected<MoveOnly, int>>);
static_assert(std::is_move_assignable_v<std::expected<MoveOnly, int>>);
static_assert(std::is_nothrow_move_assignable_v<std::expected<MoveOnly, int>>);
static_assert(!std::is_nothrow_move_assignable_v<std::expected<ThrowingMove, int>>);
