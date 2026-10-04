// [func.bind.bind]/4: bind returns an argument forwarding call wrapper. [func.require]/7: "The
// copy/move constructor of an argument forwarding call wrapper has the same apparent semantics
// as if memberwise copy/move of its state entities were performed" (Note 2: same exception
// specification as the implicit definition). [func.bind.bind]/6 Note 1: copyable when all of FD
// and TDi are. [func.require]/8: same state entity types give the same type.
#include <functional>
#include <type_traits>
#include <utility>

using namespace std::placeholders;

int f(int, int);
struct Any {  // satisfies [func.bind.bind]/3 for any bound arguments
  int operator()(auto&&...) const { return 0; }
};
struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) noexcept = default;
  MoveOnly(const MoveOnly&) = delete;
  int operator()(...) const { return 0; }
};
struct ThrowingCopy {
  ThrowingCopy() = default;
  ThrowingCopy(const ThrowingCopy&) noexcept(false) {}
  ThrowingCopy(ThrowingCopy&&) noexcept {}
  int operator()(...) const { return 0; }
};
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(const ThrowingMove&) noexcept(false) {}
  int operator()(...) const { return 0; }
};

using B = decltype(std::bind(f, 1, _1));
static_assert(std::is_copy_constructible_v<B>);
static_assert(std::is_nothrow_copy_constructible_v<B>);
static_assert(std::is_nothrow_move_constructible_v<B>);

using BM = decltype(std::bind(Any{}, MoveOnly{}, _1));
static_assert(!std::is_copy_constructible_v<BM>);
static_assert(std::is_nothrow_move_constructible_v<BM>);
using BMT = decltype(std::bind(MoveOnly{}, 1));
static_assert(!std::is_copy_constructible_v<BMT>);
static_assert(std::is_move_constructible_v<BMT>);
using BMR = decltype(std::bind<int>(MoveOnly{}, 1));
static_assert(!std::is_copy_constructible_v<BMR>);
static_assert(std::is_move_constructible_v<BMR>);

using BT = decltype(std::bind(Any{}, ThrowingCopy{}));
static_assert(std::is_copy_constructible_v<BT>);
static_assert(!std::is_nothrow_copy_constructible_v<BT>);
static_assert(std::is_nothrow_move_constructible_v<BT>);
using BTM = decltype(std::bind(ThrowingMove{}));
static_assert(!std::is_nothrow_move_constructible_v<BTM>);

// the wrapper type depends only on the decayed state entity types
static_assert(std::is_same_v<B, decltype(std::bind(&f, std::declval<const int&>(), std::declval<const decltype(_1)&>()))>);
static_assert(std::is_same_v<decltype(std::bind(f, 1, 2)), decltype(std::bind(f, std::declval<int&>(), std::declval<int&&>()))>);
static_assert(std::is_same_v<decltype(std::bind<long>(f, 1, 2)), decltype(std::bind<long>(&f, 3, 4))>);

constexpr int sub(int a, int b) { return a - b; }
constexpr bool test() {
  auto b = std::bind(sub, 10, _1);
  auto copy = b;
  auto moved = std::move(b);
  return copy(3) == 7 && moved(4) == 6;
}
static_assert(test());
