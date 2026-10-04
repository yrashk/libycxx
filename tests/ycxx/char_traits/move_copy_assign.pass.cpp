// [tab:char.traits.req]:
//   move(s,p,n): "for each i in [0, n), performs X::assign(s[i],p[i]). Copies correctly even
//     where the ranges [p, p+n) and [s, s+n) overlap. Returns: s."
//   copy(s,p,n): "Preconditions: The ranges [p, p+n) and [s, s+n) do not overlap. Returns: s.
//     for each i in [0, n), performs X::assign(s[i],p[i])."
//   assign(s,n,c): "for each i in [0, n), performs X::assign(s[i],c). Returns: s."
// constexpr in all five specializations, including the overlapping move.
#include <string_view>
#include <cstddef>
#include "check.hpp"

template <class C>
constexpr bool test() {
  using T = std::char_traits<C>;
  C buf[8] = {C('0'), C('1'), C('2'), C('3'), C('4'), C('5'), C('6'), C('7')};
  // overlapping, destination after source
  if (T::move(buf + 2, buf, 5) != buf + 2) return false;
  const C r1[8] = {C('0'), C('1'), C('0'), C('1'), C('2'), C('3'), C('4'), C('7')};
  for (int i = 0; i < 8; ++i)
    if (buf[i] != r1[i]) return false;
  // overlapping, destination before source
  if (T::move(buf, buf + 3, 5) != buf) return false;
  const C r2[8] = {C('1'), C('2'), C('3'), C('4'), C('7'), C('3'), C('4'), C('7')};
  for (int i = 0; i < 8; ++i)
    if (buf[i] != r2[i]) return false;
  // identical ranges and n == 0
  if (T::move(buf, buf, 8) != buf || buf[4] != C('7')) return false;
  if (T::move(buf + 1, buf, 0) != buf + 1 || buf[1] != C('2')) return false;

  C dst[4] = {};
  const C src[4] = {C('w'), C('x'), C('y'), C('z')};
  if (T::copy(dst, src, 3) != dst) return false;
  if (dst[0] != C('w') || dst[2] != C('y') || dst[3] != C(0)) return false;
  if (T::copy(dst, src, 0) != dst) return false;

  if (T::assign(dst, 2, C('q')) != dst) return false;
  if (dst[0] != C('q') || dst[1] != C('q') || dst[2] != C('y')) return false;
  if (T::assign(dst + 3, 0, C('q')) != dst + 3 || dst[3] != C(0)) return false;
  return true;
}

static_assert(test<char>());
static_assert(test<wchar_t>());
static_assert(test<char8_t>());
static_assert(test<char16_t>());
static_assert(test<char32_t>());

int main() {
  CHECK(test<char>());
  CHECK(test<wchar_t>());
  CHECK(test<char8_t>());
  CHECK(test<char16_t>());
  CHECK(test<char32_t>());
  // a longer overlapping move at run time
  char big[300];
  for (int i = 0; i < 300; ++i) big[i] = static_cast<char>(i % 97);
  std::char_traits<char>::move(big + 1, big, 299);
  bool ok = big[0] == 0;
  for (int i = 1; i < 300; ++i) ok = ok && big[i] == static_cast<char>((i - 1) % 97);
  CHECK(ok);
  return 0;
}
