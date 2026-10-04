// [simd.bit]: byteswap, bit_floor, bit_ceil, has_single_bit (returning mask_type), rotl, rotr,
// popcount, countl_zero, countl_one, countr_zero, countr_one, bit_width apply the <bit> function
// element-wise; the counting functions return rebind_t<make_signed_t<T>, V>; the names are also
// declared in namespace std ([simd.syn]).
#include <simd>
#include <bit>
#include <cstdint>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;

constexpr bool test() {
  using U = simd::vec<std::uint32_t, 4>;
  U u([](int i) { return std::uint32_t(1) << (i * 3) | (i == 3 ? 1u : 0u); });  // 1 8 64 513
  CHECK(simd::byteswap(U(0x11223344u))[0] == 0x44332211u);
  CHECK(simd::has_single_bit(u)[1] && !simd::has_single_bit(u)[3]);
  CHECK(simd::bit_floor(u)[3] == 512u && simd::bit_ceil(u)[3] == 1024u);
  CHECK(simd::rotl(U(0x80000001u), 1)[0] == 3u && simd::rotr(U(3u), 1)[0] == 0x80000001u);
  auto pc = simd::popcount(u);
  static_assert(std::is_same_v<decltype(pc), simd::vec<std::int32_t, 4>>);
  CHECK(pc[0] == 1 && pc[3] == 2);
  CHECK(simd::countl_zero(u)[0] == 31 && simd::countr_zero(u)[2] == 6 && simd::bit_width(u)[3] == 10);
  CHECK(simd::countl_one(U(0xF0000000u))[0] == 4 && simd::countr_one(U(7u))[0] == 3);
  CHECK(std::popcount(u)[1] == 1);  // also in namespace std
  return true;
}
static_assert(test());

int main() {
  test();
  return 0;
}
