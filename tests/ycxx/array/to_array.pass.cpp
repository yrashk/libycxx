// [array.creation]: to_array(T(&)[N]) copies, to_array(T(&&)[N]) moves; result is
// array<remove_cv_t<T>, N>; constexpr.
#include <array>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct MoveOnly {
  int v;
  constexpr MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) : v(o.v) { o.v = -1; }
  MoveOnly(const MoveOnly&) = delete;
};

constexpr bool test() {
  int a[3] = {1, 2, 3};
  auto r = std::to_array(a);
  static_assert(std::is_same_v<decltype(r), std::array<int, 3>>);
  if (r[0] != 1 || r[2] != 3) return false;
  const int ca[2] = {4, 5};
  static_assert(std::is_same_v<decltype(std::to_array(ca)), std::array<int, 2>>);
  if (std::to_array(ca)[1] != 5) return false;
  auto s = std::to_array("ab");
  static_assert(std::is_same_v<decltype(s), std::array<char, 3>>);
  if (s[0] != 'a' || s[2] != '\0') return false;
  auto l = std::to_array<long>({1, 2});
  static_assert(std::is_same_v<decltype(l), std::array<long, 2>>);
  auto d = std::to_array({1.5, 2.5});
  static_assert(std::is_same_v<decltype(d), std::array<double, 2>>);
  MoveOnly m[2] = {MoveOnly(1), MoveOnly(2)};
  auto mm = std::to_array(std::move(m));
  if (mm[0].v != 1 || mm[1].v != 2 || m[0].v != -1) return false;
  auto pr = std::to_array({MoveOnly(3)});
  if (pr[0].v != 3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
