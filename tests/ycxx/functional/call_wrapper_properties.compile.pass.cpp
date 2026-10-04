// [func.require]/5: "A postfix call performed on a perfect forwarding call wrapper is
// expression-equivalent to an expression e determined from its call pattern" -- so validity
// and noexcept follow the call pattern, and a deleted overload of the target for a given
// cv/ref is not bypassed. /4: state entities are delivered as cv T& or cv T&&.
// /6: a simple call wrapper (mem_fn) is copy-constructible/assignable with non-throwing
// copy/move/assignment. /7: copy/move constructors behave as memberwise copy/move of the state
// entities. /8: "Argument forwarding call wrappers returned by a given standard library
// function template have the same type if the types of their corresponding state entities
// are the same."
// Applies to not_fn ([func.not.fn]), bind_front/bind_back ([func.bind.partial]) and mem_fn
// ([func.memfn]).
#include <functional>
#include <type_traits>
#include <utility>

struct MaybeThrow {
  bool operator()(int) const { return true; }
};
struct NoThrow {
  bool operator()(int) const noexcept { return true; }
};
struct DeletedConst {  // only the non-const lvalue form is usable
  bool operator()() & { return true; }
  bool operator()() const& = delete;
  bool operator()() && = delete;
  bool operator()() const&& = delete;
};
struct IntOnly {
  bool operator()(int) const { return true; }
};
struct S {
  int v;
  int f() noexcept { return v; }
  int g() { return v; }
  int h() noexcept { return v + 1; }
};
struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) noexcept = default;
  MoveOnly(const MoveOnly&) = delete;
  bool operator()(int) const { return true; }
};
struct ThrowingCopy {
  ThrowingCopy() = default;
  ThrowingCopy(const ThrowingCopy&) noexcept(false) {}
  bool operator()(int) const { return true; }
};

// noexcept follows the call pattern
static_assert(!std::is_nothrow_invocable_v<decltype(std::not_fn(MaybeThrow{})), int>);
static_assert(std::is_nothrow_invocable_v<decltype(std::not_fn(NoThrow{})), int>);
static_assert(!std::is_nothrow_invocable_v<decltype(std::bind_front(MaybeThrow{}, 1))>);
static_assert(std::is_nothrow_invocable_v<decltype(std::bind_front(NoThrow{}, 1))>);
static_assert(std::is_nothrow_invocable_v<decltype(std::bind_back(NoThrow{}, 1))>);
static_assert(!std::is_nothrow_invocable_v<decltype(std::bind_back(MaybeThrow{}, 1))>);
static_assert(std::is_nothrow_invocable_v<decltype(std::mem_fn(&S::f)), S&>);
static_assert(!std::is_nothrow_invocable_v<decltype(std::mem_fn(&S::g)), S&>);
static_assert(std::is_nothrow_invocable_v<decltype(std::not_fn<NoThrow{}>()), int>);
static_assert(!std::is_nothrow_invocable_v<decltype(std::not_fn<MaybeThrow{}>()), int>);
static_assert(std::is_nothrow_invocable_v<decltype(std::bind_front<NoThrow{}>(1))>);

// validity follows the call pattern
static_assert(!std::is_invocable_v<decltype(std::not_fn(IntOnly{})), int*>);
static_assert(!std::is_invocable_v<decltype(std::bind_front(IntOnly{}, 1)), int>);
static_assert(!std::is_invocable_v<decltype(std::bind_back(IntOnly{}, nullptr))>);
static_assert(!std::is_invocable_v<decltype(std::mem_fn(&S::f)), int>);

// deleted overloads of the target are not bypassed
using NF = decltype(std::not_fn(DeletedConst{}));
static_assert(std::is_invocable_v<NF&>);
static_assert(!std::is_invocable_v<const NF&>);
static_assert(!std::is_invocable_v<NF&&>);
static_assert(!std::is_invocable_v<const NF&&>);
using BF = decltype(std::bind_front(DeletedConst{}));
static_assert(std::is_invocable_v<BF&>);
static_assert(!std::is_invocable_v<const BF&>);
static_assert(!std::is_invocable_v<BF&&>);
using BB = decltype(std::bind_back(DeletedConst{}));
static_assert(std::is_invocable_v<BB&>);
static_assert(!std::is_invocable_v<const BB&>);
static_assert(!std::is_invocable_v<BB&&>);

// copy/move follow the state entities
using NFM = decltype(std::not_fn(MoveOnly{}));
static_assert(!std::is_copy_constructible_v<NFM>);
static_assert(std::is_nothrow_move_constructible_v<NFM>);
using BFM = decltype(std::bind_front(IntOnly{}, MoveOnly{}));
static_assert(!std::is_copy_constructible_v<BFM>);
static_assert(std::is_move_constructible_v<BFM>);
using BBT = decltype(std::bind_back(IntOnly{}, ThrowingCopy{}));
static_assert(std::is_copy_constructible_v<BBT>);
static_assert(!std::is_nothrow_copy_constructible_v<BBT>);
using BFI = decltype(std::bind_front(IntOnly{}, 1));
static_assert(std::is_nothrow_copy_constructible_v<BFI>);

// mem_fn is a simple call wrapper
using MF = decltype(std::mem_fn(&S::f));
static_assert(std::is_nothrow_copy_constructible_v<MF>);
static_assert(std::is_nothrow_move_constructible_v<MF>);
static_assert(std::is_nothrow_copy_assignable_v<MF>);
static_assert(std::is_nothrow_move_assignable_v<MF>);
constexpr auto mf1 = std::mem_fn(&S::f);
constexpr auto mf2 = mf1;  // constexpr copy
static_assert(sizeof(mf2) > 0);

// same state entity types -> same wrapper type
static_assert(std::is_same_v<decltype(std::mem_fn(&S::f)), decltype(std::mem_fn(&S::h))>);
static_assert(std::is_same_v<decltype(std::not_fn(IntOnly{})), decltype(std::not_fn(std::declval<IntOnly&>()))>);
static_assert(std::is_same_v<decltype(std::bind_front(IntOnly{}, 1)), decltype(std::bind_front(IntOnly{}, 2))>);
static_assert(std::is_same_v<decltype(std::bind_back(IntOnly{}, 1)), decltype(std::bind_back(std::declval<const IntOnly&>(), std::declval<int&>()))>);
