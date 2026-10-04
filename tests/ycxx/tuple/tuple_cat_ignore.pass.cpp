// [tuple.creation]/7-9: tuple_cat accepts any tuple-like arguments (tuple and pair here);
// CTypes are the tuple_element_t of each argument, elements are get<k>(std::forward<Ti>(tpi)).
// [tuple.syn]: ignore is an inline constexpr object whose operator= is a constexpr const
// member taking const auto& and returning const ignore-type&, noexcept.
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct MoveOnly {
  int v;
  constexpr MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) : v(o.v) { o.v = -1; }
  MoveOnly(const MoveOnly&) = delete;
};
struct NonCopyable { NonCopyable() = default; NonCopyable(const NonCopyable&) = delete; };

static_assert(std::is_same_v<decltype(std::tuple_cat()), std::tuple<>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::tuple<int>{}, std::pair<long, char>{})),
                             std::tuple<int, long, char>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::pair<int&, const long&>(std::declval<int&>(), 0L),
                                                     std::tuple<>{}, std::tuple<double&&>(1.0))),
                             std::tuple<int&, const long&, double&&>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::declval<const std::pair<int, int>&>(),
                                                     std::declval<std::pair<int, int>&>())),
                             std::tuple<int, int, int, int>>);

constexpr bool test_cat() {
  std::pair<int, long> p(1, 2);
  std::tuple<char, double> t('a', 3.5);
  auto r = std::tuple_cat(t, p, std::tuple<>{}, std::make_pair(true, 4u));
  static_assert(std::is_same_v<decltype(r), std::tuple<char, double, int, long, bool, unsigned>>);
  if (std::get<0>(r) != 'a' || std::get<1>(r) != 3.5 || std::get<2>(r) != 1 || std::get<3>(r) != 2) return false;
  if (std::get<4>(r) != true || std::get<5>(r) != 4u) return false;
  // Moves from rvalue arguments.
  std::pair<MoveOnly, int> mp(MoveOnly(5), 6);
  auto m = std::tuple_cat(std::move(mp), std::tuple<MoveOnly>(MoveOnly(7)));
  if (std::get<0>(m).v != 5 || mp.first.v != -1 || std::get<2>(m).v != 7) return false;
  // Reference elements are preserved.
  int x = 0;
  std::pair<int&, int> rp(x, 0);
  auto rr = std::tuple_cat(rp, std::tie(x));
  std::get<0>(rr) = 8;
  std::get<2>(rr) += 1;
  if (x != 9) return false;
  return true;
}

constexpr bool test_ignore() {
  std::ignore = 5;
  std::ignore = NonCopyable{};
  std::ignore = std::ignore;
  const auto& r = (std::ignore = 1.5);
  if (&r != &std::ignore) return false;
  int a = 0;
  std::tie(std::ignore, a, std::ignore) = std::make_tuple(1, 2, 3);
  if (a != 2) return false;
  std::tie(a, std::ignore) = std::make_pair(4, 5);
  if (a != 4) return false;
  return true;
}

static_assert(std::is_const_v<decltype(std::ignore)>);
static_assert(noexcept(std::ignore = 1));
static_assert(std::is_same_v<decltype(std::ignore = 1), std::remove_reference_t<decltype(std::ignore)>&>);
constexpr auto copy_of_ignore = std::ignore;        // usable in constant expressions
static_assert((copy_of_ignore = 3, true));
static_assert(test_cat());
static_assert(test_ignore());

int main() {
  CHECK(test_cat());
  CHECK(test_ignore());
  return 0;
}
