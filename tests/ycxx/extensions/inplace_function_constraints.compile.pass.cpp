#include <ycxx/functional.hpp>
#include <type_traits>

using W = ycxx::inplace_function<int(int), 32, 16>;
struct Good { int operator()(int x) { return x; } };
struct Large { char data[33]; int operator()(int); };
struct alignas(32) Overaligned { int operator()(int); };
struct Throwing { Throwing() = default; Throwing(Throwing&&) noexcept(false); Throwing(const Throwing&); int operator()(int); };
struct MoveOnly { MoveOnly() = default; MoveOnly(MoveOnly&&) noexcept; int operator()(int); };
struct BadSignature { void operator()(); };
struct Incomplete;
struct RefTemporary { int operator()(); };

static_assert(W::capacity == 32 && W::alignment == 16);
static_assert(std::is_nothrow_default_constructible_v<W>);
static_assert(std::is_nothrow_constructible_v<W, std::nullptr_t>);
static_assert(std::is_nothrow_constructible_v<W, Good>);
static_assert(std::is_copy_constructible_v<W> && std::is_copy_assignable_v<W>);
static_assert(std::is_nothrow_move_constructible_v<W> && std::is_nothrow_move_assignable_v<W>);
static_assert(std::is_nothrow_swappable_v<W>);
static_assert(!std::is_convertible_v<W, bool>);
static_assert(std::is_invocable_r_v<int, const W&, int>);
static_assert(!std::is_constructible_v<W, Large>);
static_assert(!std::is_constructible_v<W, Overaligned>);
static_assert(!std::is_constructible_v<W, Throwing>);
static_assert(!std::is_constructible_v<W, MoveOnly>);
static_assert(!std::is_constructible_v<W, BadSignature>);
static_assert(!std::is_constructible_v<ycxx::inplace_function<const int&()>, RefTemporary>);
static_assert(ycxx::uses_inline_storage_v<W, Good>);
static_assert(!ycxx::uses_inline_storage_v<W, Large>);
static_assert(!ycxx::uses_inline_storage_v<W, Overaligned>);
static_assert(!ycxx::uses_inline_storage_v<W, Throwing>);
static_assert(!ycxx::uses_inline_storage_v<W, void>);
static_assert(!ycxx::uses_inline_storage_v<W, Incomplete>);
static_assert(ycxx::uses_inline_storage_v<std::function<void()>, void()>);
static_assert(ycxx::uses_inline_storage_v<std::move_only_function<int(int) noexcept>, Good>);
static_assert(ycxx::uses_inline_storage_v<std::copyable_function<int(int) const>, Good>);
static_assert(!ycxx::uses_inline_storage_v<int, Good>);

// Capturing lambdas must obey the same constructor and assignment constraints.
using SmallClosure = decltype([value = 1](int x) { return value + x; });
using LargeClosure = decltype([value = Large{}](int x) mutable { return value(x); });
using OveralignedClosure = decltype([value = Overaligned{}](int x) mutable { return value(x); });
using ThrowingClosure = decltype([value = Throwing{}](int x) mutable { return value(x); });
using MoveOnlyClosure = decltype([value = MoveOnly{}](int x) mutable { return value(x); });
using BadSignatureClosure = decltype([value = 1] { return value; });
using DanglingResultClosure = decltype([value = 1] { return value; });

template <class T>
constexpr bool rejects = !std::is_constructible_v<W, T> && !std::is_constructible_v<W, T&> &&
                         !std::is_assignable_v<W&, T> && !std::is_assignable_v<W&, T&>;
static_assert(std::is_constructible_v<W, SmallClosure> && std::is_assignable_v<W&, SmallClosure>);
static_assert(rejects<LargeClosure>);
static_assert(rejects<OveralignedClosure>);
static_assert(rejects<ThrowingClosure>);
static_assert(rejects<MoveOnlyClosure>);
static_assert(rejects<BadSignatureClosure>);
static_assert(!std::is_constructible_v<ycxx::inplace_function<const int&()>, DanglingResultClosure>);
