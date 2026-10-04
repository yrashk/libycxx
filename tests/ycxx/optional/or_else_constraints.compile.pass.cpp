// [optional.monadic]/13: or_else(F&&) const&: "Constraints: F models invocable and T models
// copy_constructible." /16: or_else(F&&) &&: "Constraints: F models invocable and T models
// move_constructible." So a const lvalue optional of a move-only type has no usable or_else,
// an rvalue one does, and a callable that needs arguments is rejected.
#include <optional>
#include <utility>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly(const MoveOnly&) = delete;
};
struct Immovable {
  Immovable() = default;
  Immovable(Immovable&&) = delete;
};

template <class O, class F>
concept has_or_else = requires(O&& o, F&& f) { std::forward<O>(o).or_else(std::forward<F>(f)); };

using MakeMO = decltype([] { return std::optional<MoveOnly>(); });
using MakeInt = decltype([] { return std::optional<int>(); });
using NeedsArg = decltype([](int) { return std::optional<int>(); });

static_assert(has_or_else<std::optional<int>&, MakeInt>);
static_assert(has_or_else<const std::optional<int>&, MakeInt>);
static_assert(has_or_else<std::optional<int>, MakeInt>);
static_assert(!has_or_else<std::optional<int>&, NeedsArg>);
static_assert(!has_or_else<const std::optional<MoveOnly>&, MakeMO>);
static_assert(!has_or_else<std::optional<MoveOnly>&, MakeMO>);  // lvalue binds to const&
static_assert(has_or_else<std::optional<MoveOnly>, MakeMO>);
static_assert(!has_or_else<std::optional<Immovable>, decltype([] { return std::optional<Immovable>(); })>);
