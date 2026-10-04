// Built with -ffreestanding -fno-exceptions -fno-rtti (__STDC_HOSTED__ is 0); see
// modes/freestanding_items for the rules ([compliance]/2-4, [freestanding.item]/4-6). The
// freestanding items of the remaining Table 27 headers:
//   [charconv.syn] to_chars/from_chars for integers (freestanding; the floating-point overloads
//     are freestanding-deleted), to_chars_result, from_chars_result
//   [cmath.syn] constexpr int abs(int j); long, long long (freestanding)
//   [execution.syn] is_execution_policy, is_execution_policy_v (freestanding)
//   [inplace.vector.syn] inplace_vector (partially freestanding): try_push_back,
//     try_emplace_back return optional<reference> ([inplace.vector.modifiers])
//   [string.syn] char_traits and its specializations (freestanding)
//   [system.error.syn] enum class errc (freestanding), errc::x == the corresponding E macro
//   [rand.synopsis] uniform_random_bit_generator, the predefined engines minstd_rand0,
//     minstd_rand, mt19937, mt19937_64, ranlux24_base, ranlux48_base, ranlux24, ranlux48,
//     philox4x32, philox4x64 (freestanding), uniform_int_distribution (partially freestanding);
//     [rand.predef]: the 10000th invocation of a default-constructed engine yields the value given
#include <cerrno>
#include <charconv>
#include <cmath>
#include <concepts>
#include <execution>
#include <inplace_vector>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include "check.hpp"

// FLAGS: -ffreestanding -fno-exceptions -fno-rtti

static_assert(__STDC_HOSTED__ == 0);

static_assert(std::abs(-4) == 4 && std::abs(-4L) == 4L && std::abs(-4LL) == 4LL);
static_assert(std::is_execution_policy_v<std::execution::sequenced_policy>);
static_assert(std::is_execution_policy<std::execution::parallel_policy>::value);
static_assert(!std::is_execution_policy_v<int>);
static_assert(std::char_traits<char>::length("abc") == 3);
static_assert(std::char_traits<char16_t>::compare(u"ab", u"ac", 2) < 0);
static_assert(std::char_traits<char32_t>::find(U"xyz", 3, U'z') != nullptr);
static_assert(std::char_traits<wchar_t>::eq_int_type(std::char_traits<wchar_t>::eof(), std::char_traits<wchar_t>::eof()));
static_assert(std::char_traits<char8_t>::to_int_type(u8'a') == 'a');
static_assert(static_cast<int>(std::errc::invalid_argument) == EINVAL);
static_assert(static_cast<int>(std::errc::result_out_of_range) == ERANGE);
static_assert(std::uniform_random_bit_generator<std::mt19937>);

constexpr bool constexpr_items() {
  std::inplace_vector<int, 2> iv;
  auto r1 = iv.try_push_back(1);
  if (!r1 || &*r1 != &iv[0]) return false;
  if (!iv.try_emplace_back(2) || iv.try_push_back(3)) return false;
  if (iv.size() != 2 || iv[1] != 2) return false;
  char out[8]{};
  auto r = std::to_chars(out, out + 8, -255, 16);
  if (r.ec != std::errc() || std::string_view(out, r.ptr) != "-ff") return false;
  auto small = std::to_chars(out, out + 2, 12345);
  if (small.ec != std::errc::value_too_large || small.ptr != out + 2) return false;
  int back = 0;
  auto fr = std::from_chars(out, out + 3, back, 16);
  if (fr.ec != std::errc() || back != -255) return false;
  unsigned char uc = 0;
  const char big[] = "300";
  if (std::from_chars(big, big + 3, uc).ec != std::errc::result_out_of_range) return false;
  return true;
}
static_assert(constexpr_items());

template <class E>
typename E::result_type ten_thousandth() {
  E e;
  e.discard(9999);
  return e();
}

extern "C" int main() {  // see modes/freestanding_items
  CHECK(constexpr_items());
  CHECK(ten_thousandth<std::minstd_rand0>() == 1043618065u);
  CHECK(ten_thousandth<std::minstd_rand>() == 399268537u);
  CHECK(ten_thousandth<std::mt19937>() == 4123659995u);
  CHECK(ten_thousandth<std::mt19937_64>() == 9981545732273789042ull);
  CHECK(ten_thousandth<std::ranlux24_base>() == 7937952u);
  CHECK(ten_thousandth<std::ranlux48_base>() == 61839128582725ull);
  CHECK(ten_thousandth<std::ranlux24>() == 9901578u);
  CHECK(ten_thousandth<std::ranlux48>() == 249142670248501ull);
  CHECK(ten_thousandth<std::philox4x32>() == 1955073260u);
  CHECK(ten_thousandth<std::philox4x64>() == 3409172418970261260ull);
  std::mt19937 g;
  std::uniform_int_distribution<long> d(-3, 3);
  bool seen[7] = {};
  for (int i = 0; i < 1000; ++i) {
    long v = d(g);
    CHECK(v >= -3 && v <= 3);
    seen[v + 3] = true;
  }
  for (bool b : seen) CHECK(b);
  CHECK(d.param() == std::uniform_int_distribution<long>(-3, 3).param());
  return 0;
}
