// [span.sub]: first<Count>() -> span<element_type, Count>; last<Count>() -> span<element_type,
// Count>; subspan<Offset, Count>() whose extent is "Count != dynamic_extent ? Count :
// (Extent != dynamic_extent ? Extent - Offset : dynamic_extent)"; the runtime first(count),
// last(count), subspan(offset, count) always return span<element_type, dynamic_extent>.
#include <span>
#include <type_traits>
#include "check.hpp"

constexpr auto dyn = std::dynamic_extent;

constexpr bool test() {
  int a[6] = {0, 1, 2, 3, 4, 5};
  std::span<int, 6> s(a);
  std::span<int> d(a);

  auto f = s.first<2>();
  static_assert(std::is_same_v<decltype(f), std::span<int, 2>>);
  if (f.data() != a || f.size() != 2) return false;
  auto fd = d.first<2>();
  static_assert(std::is_same_v<decltype(fd), std::span<int, 2>>);
  if (fd.data() != a) return false;

  auto l = s.last<2>();
  static_assert(std::is_same_v<decltype(l), std::span<int, 2>>);
  if (l.data() != a + 4 || l.size() != 2) return false;
  auto ld = d.last<3>();
  static_assert(std::is_same_v<decltype(ld), std::span<int, 3>>);
  if (ld.data() != a + 3) return false;

  auto ss1 = s.subspan<1>();
  static_assert(std::is_same_v<decltype(ss1), std::span<int, 5>>);
  if (ss1.data() != a + 1 || ss1.size() != 5) return false;
  auto ss2 = s.subspan<1, 2>();
  static_assert(std::is_same_v<decltype(ss2), std::span<int, 2>>);
  if (ss2.data() != a + 1 || ss2.size() != 2) return false;
  auto ss3 = d.subspan<2>();
  static_assert(std::is_same_v<decltype(ss3), std::span<int, dyn>>);
  if (ss3.data() != a + 2 || ss3.size() != 4) return false;
  auto ss4 = d.subspan<2, 3>();
  static_assert(std::is_same_v<decltype(ss4), std::span<int, 3>>);
  if (ss4.data() != a + 2) return false;
  auto ss5 = s.subspan<6>();
  static_assert(std::is_same_v<decltype(ss5), std::span<int, 0>>);
  if (ss5.size() != 0 || ss5.data() != a + 6) return false;
  auto ss6 = s.subspan<0, dyn>();
  static_assert(std::is_same_v<decltype(ss6), std::span<int, 6>>);

  auto rf = s.first(3);
  static_assert(std::is_same_v<decltype(rf), std::span<int, dyn>>);
  if (rf.data() != a || rf.size() != 3) return false;
  auto rl = s.last(1);
  static_assert(std::is_same_v<decltype(rl), std::span<int, dyn>>);
  if (rl.data() != a + 5 || rl.size() != 1) return false;
  auto rs = s.subspan(2);
  static_assert(std::is_same_v<decltype(rs), std::span<int, dyn>>);
  if (rs.data() != a + 2 || rs.size() != 4) return false;
  auto rs2 = d.subspan(2, 1);
  if (rs2.data() != a + 2 || rs2.size() != 1) return false;
  auto rs3 = d.subspan(6);
  if (rs3.size() != 0) return false;
  auto rs4 = d.subspan(1, dyn);
  if (rs4.size() != 5) return false;
  if (d.first(0).size() != 0 || d.last(0).size() != 0 || d.last(6).data() != a) return false;

  std::span<const int, 6> cs(a);
  auto cf = cs.first<1>();
  static_assert(std::is_same_v<decltype(cf), std::span<const int, 1>>);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
