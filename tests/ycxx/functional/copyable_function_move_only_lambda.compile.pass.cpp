// [func.wrap.copy.ctor]/7-8: the converting constructor's Constraints are only the
// type-identity checks and is-callable-from<VT>; copyability is a Mandates, so a move-only
// callable still satisfies is_constructible (the program is ill-formed only if the
// constructor is actually used). move_only_function accepts it ([func.wrap.move.ctor]/5).
// copyable_function converts to move_only_function of a compatible signature.
#include <functional>
#include <type_traits>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly(const MoveOnly&) = delete;
  int operator()() const { return 0; }
};

static_assert(std::is_constructible_v<std::move_only_function<int() const>, MoveOnly>);
static_assert(std::is_constructible_v<std::copyable_function<int() const>, MoveOnly>);
static_assert(std::is_constructible_v<std::move_only_function<int()>, std::copyable_function<int() const>>);
static_assert(std::is_constructible_v<std::move_only_function<int() &&>, std::copyable_function<int()>>);
static_assert(!std::is_constructible_v<std::move_only_function<int() const>, std::copyable_function<int()>>);
