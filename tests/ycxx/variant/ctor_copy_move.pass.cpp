// [variant.ctor]/7-13: copy constructor (deleted unless all copy-constructible; trivial if all
// trivially copy-constructible; noexcept = AND of nothrow copy) and move constructor
// (Constraints all move-constructible; trivial if all trivially move-constructible;
// noexcept = AND of nothrow move). [variant.dtor]/2: trivial destructor if all trivial.
#include <variant>
#include <type_traits>
#include "check.hpp"

struct MoveOnly {
  int v;
  constexpr MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) noexcept : v(o.v) { o.v = -1; }
  MoveOnly(const MoveOnly&) = delete;
};
struct NonTrivialCopy {
  int v = 0;
  constexpr NonTrivialCopy() = default;
  constexpr NonTrivialCopy(const NonTrivialCopy& o) : v(o.v + 1) {}
};
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(const ThrowingMove&) = default;
  ThrowingMove(ThrowingMove&&) noexcept(false) {}
};
struct NoMove {
  NoMove() = default;
  NoMove(const NoMove&) = delete;
  NoMove(NoMove&&) = delete;
};
struct NonTrivialDtor { ~NonTrivialDtor() {} };

// copy
static_assert(std::is_copy_constructible_v<std::variant<int, double>>);
static_assert(!std::is_copy_constructible_v<std::variant<int, MoveOnly>>);
static_assert(std::is_trivially_copy_constructible_v<std::variant<int, double>>);
static_assert(!std::is_trivially_copy_constructible_v<std::variant<int, NonTrivialCopy>>);
static_assert(std::is_nothrow_copy_constructible_v<std::variant<int, double>>);
// move
static_assert(std::is_move_constructible_v<std::variant<int, MoveOnly>>);
static_assert(!std::is_move_constructible_v<std::variant<int, NoMove>>);
static_assert(std::is_trivially_move_constructible_v<std::variant<int, double>>);
static_assert(!std::is_trivially_move_constructible_v<std::variant<int, MoveOnly>>);
static_assert(std::is_nothrow_move_constructible_v<std::variant<int, MoveOnly>>);
static_assert(!std::is_nothrow_move_constructible_v<std::variant<int, ThrowingMove>>);
// destructor
static_assert(std::is_trivially_destructible_v<std::variant<int, double>>);
static_assert(!std::is_trivially_destructible_v<std::variant<int, NonTrivialDtor>>);
// a variant of trivially copyable types is trivially copyable
static_assert(std::is_trivially_copyable_v<std::variant<int, double, char>>);

constexpr bool test() {
  std::variant<int, NonTrivialCopy> a{std::in_place_index<1>};
  std::variant<int, NonTrivialCopy> b(a);
  if (b.index() != 1 || std::get<1>(b).v != 1) return false;
  std::variant<int, MoveOnly> m{std::in_place_index<1>, 5};
  std::variant<int, MoveOnly> n(std::move(m));
  if (n.index() != 1 || std::get<1>(n).v != 5) return false;
  if (std::get<1>(m).v != -1) return false;  // moved-from, still holds alternative 1
  std::variant<int, double> c{3.5};
  std::variant<int, double> d = c;
  if (std::get<1>(d) != 3.5) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
