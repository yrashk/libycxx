// [forward]/2-3: forward<T>(t) returns static_cast<T&&>(t); both overloads noexcept and
// constexpr. /10: move(t) returns static_cast<remove_reference_t<T>&&>(t).
// [utility.syn]: move_if_noexcept(T& x) noexcept returns conditional_t<
// !is_nothrow_move_constructible_v<T> && is_copy_constructible_v<T>, const T&, T&&>.
#include <utility>
#include <type_traits>
#include "check.hpp"

struct NothrowMove {
  NothrowMove() = default;
  NothrowMove(const NothrowMove&) = default;
  NothrowMove(NothrowMove&&) noexcept {}
};
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(const ThrowingMove&) = default;
  ThrowingMove(ThrowingMove&&) {}
};
struct ThrowingMoveOnly {
  ThrowingMoveOnly() = default;
  ThrowingMoveOnly(ThrowingMoveOnly&&) {}
};

int i = 0;
const int ci = 0;
static_assert(std::is_same_v<decltype(std::forward<int>(i)), int&&>);
static_assert(std::is_same_v<decltype(std::forward<int&>(i)), int&>);
static_assert(std::is_same_v<decltype(std::forward<const int&>(i)), const int&>);
static_assert(std::is_same_v<decltype(std::forward<const int>(ci)), const int&&>);
static_assert(std::is_same_v<decltype(std::forward<int>(1)), int&&>);
static_assert(std::is_same_v<decltype(std::forward<const int&>(1)), const int&>);
static_assert(noexcept(std::forward<int>(i)) && noexcept(std::forward<int>(1)));
static_assert(std::is_same_v<decltype(std::move(i)), int&&>);
static_assert(std::is_same_v<decltype(std::move(ci)), const int&&>);
static_assert(std::is_same_v<decltype(std::move(1)), int&&>);
static_assert(noexcept(std::move(i)));

NothrowMove nm;
ThrowingMove tm;
ThrowingMoveOnly tmo;
static_assert(std::is_same_v<decltype(std::move_if_noexcept(nm)), NothrowMove&&>);
static_assert(std::is_same_v<decltype(std::move_if_noexcept(tm)), const ThrowingMove&>);
static_assert(std::is_same_v<decltype(std::move_if_noexcept(tmo)), ThrowingMoveOnly&&>);
static_assert(std::is_same_v<decltype(std::move_if_noexcept(i)), int&&>);
static_assert(noexcept(std::move_if_noexcept(tm)));

constexpr int which(int&) { return 1; }
constexpr int which(int&&) { return 2; }
constexpr int which(const int&) { return 3; }
template <class T>
constexpr int relay(T&& t) {
  return which(std::forward<T>(t));
}

constexpr bool test() {
  int x = 1;
  const int cx = 2;
  if (relay(x) != 1 || relay(2) != 2 || relay(cx) != 3) return false;
  if (which(std::move(x)) != 2) return false;
  int& r = std::forward<int&>(x);
  if (&r != &x) return false;
  int&& rr = std::move(x);
  if (&rr != &x) return false;
  int&& mi = std::move_if_noexcept(x);
  if (&mi != &x) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
