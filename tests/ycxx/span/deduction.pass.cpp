// [span.overview], [span.deduct], [span.syn]: deduction guides, including (C++26)
// span(It, EndOrSize) -> span<remove_reference_t<iter_reference_t<It>>,
// maybe-static-ext<EndOrSize>> where maybe-static-ext<T> is T::value for an
// integral-constant-like T and dynamic_extent otherwise.
#include <span>
#include <array>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

struct Vec {
  int buf[3] = {};
  int* begin() { return buf; }
  int* end() { return buf + 3; }
  const int* begin() const { return buf; }
  const int* end() const { return buf + 3; }
};

int main() {
  int a[4] = {};
  const int ca[2] = {};
  std::array<long, 3> arr{};
  const std::array<long, 3>& carr = arr;
  Vec v;
  const Vec& cv = v;

  std::span s1(a);
  static_assert(std::is_same_v<decltype(s1), std::span<int, 4>>);
  std::span s2(ca);
  static_assert(std::is_same_v<decltype(s2), std::span<const int, 2>>);
  std::span s3(arr);
  static_assert(std::is_same_v<decltype(s3), std::span<long, 3>>);
  std::span s4(carr);
  static_assert(std::is_same_v<decltype(s4), std::span<const long, 3>>);
  std::span s5(v);
  static_assert(std::is_same_v<decltype(s5), std::span<int>>);
  std::span s6(cv);
  static_assert(std::is_same_v<decltype(s6), std::span<const int>>);
  std::span s7(a, a + 2);
  static_assert(std::is_same_v<decltype(s7), std::span<int>>);
  std::span s8(a, 3);
  static_assert(std::is_same_v<decltype(s8), std::span<int>>);
  std::span s9(arr.begin(), std::size_t(2));
  static_assert(std::is_same_v<decltype(s9), std::span<long>>);
  std::span s10(a, std::integral_constant<std::size_t, 3>{});
  static_assert(std::is_same_v<decltype(s10), std::span<int, 3>>);
  std::span s11(ca, std::integral_constant<int, 2>{});
  static_assert(std::is_same_v<decltype(s11), std::span<const int, 2>>);
  // bool constants are not integral-constant-like
  std::span s12(a, std::true_type{});
  static_assert(std::is_same_v<decltype(s12), std::span<int>>);
  std::span s13(s1);
  static_assert(std::is_same_v<decltype(s13), std::span<int, 4>>);

  CHECK(s1.size() == 4 && s7.size() == 2 && s8.size() == 3 && s9.size() == 2);
  CHECK(s10.size() == 3 && s11.size() == 2 && s12.size() == 1);
  CHECK(s5.data() == v.buf && s6.size() == 3);
  return 0;
}
