// [optional.ctor]/7,12: copy/move constructors trivial when T's are; deleted copy unless
// copy-constructible. [optional.dtor]: trivial destructor when T's is.
// [optional.assign]/7,13: copy/move assignment trivial when T's ctor, assignment and dtor
// are trivial; copy assignment deleted unless T copy-constructible and copy-assignable;
// move assignment constrained. Hence optional<trivially-copyable> is trivially copyable.
// [optional.optional.ref.general]/2: optional<T&> is trivially copyable.
#include <optional>
#include <type_traits>

struct NTDtor { ~NTDtor() {} };
struct NTCopy { NTCopy() = default; NTCopy(const NTCopy&) {} NTCopy& operator=(const NTCopy&) = default; };
struct NTCopyAssign { NTCopyAssign& operator=(const NTCopyAssign&) { return *this; } };
struct NoCopyAssign { NoCopyAssign& operator=(const NoCopyAssign&) = delete; };
struct MoveOnly { MoveOnly() = default; MoveOnly(MoveOnly&&) = default; MoveOnly& operator=(MoveOnly&&) = default; };
struct NoMoveAssign {
  NoMoveAssign() = default;
  NoMoveAssign(NoMoveAssign&&) = default;
  NoMoveAssign& operator=(NoMoveAssign&&) = delete;
};

static_assert(std::is_trivially_copyable_v<std::optional<int>>);
static_assert(std::is_trivially_copyable_v<std::optional<double*>>);
static_assert(std::is_trivially_copy_constructible_v<std::optional<int>>);
static_assert(std::is_trivially_move_constructible_v<std::optional<int>>);
static_assert(std::is_trivially_copy_assignable_v<std::optional<int>>);
static_assert(std::is_trivially_move_assignable_v<std::optional<int>>);
static_assert(std::is_trivially_destructible_v<std::optional<int>>);

static_assert(!std::is_trivially_destructible_v<std::optional<NTDtor>>);
static_assert(!std::is_trivially_copy_assignable_v<std::optional<NTDtor>>);
static_assert(!std::is_trivially_copy_constructible_v<std::optional<NTCopy>>);
static_assert(!std::is_trivially_copy_assignable_v<std::optional<NTCopy>>);   // needs trivial copy ctor too
static_assert(std::is_trivially_copy_constructible_v<std::optional<NTCopyAssign>>);
static_assert(!std::is_trivially_copy_assignable_v<std::optional<NTCopyAssign>>);

static_assert(std::is_copy_constructible_v<std::optional<NoCopyAssign>>);
static_assert(!std::is_copy_assignable_v<std::optional<NoCopyAssign>>);
static_assert(!std::is_copy_constructible_v<std::optional<MoveOnly>>);
static_assert(!std::is_copy_assignable_v<std::optional<MoveOnly>>);
static_assert(std::is_move_assignable_v<std::optional<MoveOnly>>);
static_assert(std::is_trivially_move_assignable_v<std::optional<MoveOnly>>);
static_assert(!std::is_move_assignable_v<std::optional<NoMoveAssign>>);
static_assert(std::is_move_constructible_v<std::optional<NoMoveAssign>>);

// optional<T&>
static_assert(std::is_trivially_copyable_v<std::optional<int&>>);
static_assert(std::is_trivially_copyable_v<std::optional<NTDtor&>>);
static_assert(std::is_trivially_copyable_v<std::optional<const NTCopy&>>);
static_assert(std::is_nothrow_copy_constructible_v<std::optional<int&>>);
static_assert(std::is_nothrow_copy_assignable_v<std::optional<int&>>);
static_assert(std::is_nothrow_default_constructible_v<std::optional<int&>>);
