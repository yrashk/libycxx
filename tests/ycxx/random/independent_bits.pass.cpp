// [rand.adapt.ibits]/2-4: with R = e.max() - e.min() + 1, m = floor(log2 R), n = ceil(w/m) if
// R - y0 <= floor(y0/n) else 1 + ceil(w/m), w0 = floor(w/n), n0 = n - w mod n,
// y0 = 2^w0 * floor(R / 2^w0), y1 = 2^(w0+1) * floor(R / 2^(w0+1)), the value is
//   S = 0; for k < n0: do u = e() - e.min(); while (u >= y0); S = 2^w0 * S + u mod 2^w0;
//          for k >= n0: do u = e() - e.min(); while (u >= y1); S = 2^(w0+1) * S + u mod 2^(w0+1).
// min() is 0, max() is 2^w - 1.
#include <random>
#include <cstdint>
#include <type_traits>
#include "check.hpp"
#include "random_support.hpp"

using u128 = unsigned __int128;

template <class Engine, std::size_t w>
struct ref_ibits {
  Engine e;
  u128 R = u128(Engine::max()) - Engine::min() + 1;
  std::size_t n, n0, w0;
  u128 y0, y1;
  explicit ref_ibits(const Engine& b) : e(b) {
    std::size_t m = 0;
    while ((u128(1) << (m + 1)) <= R) ++m;
    auto setup = [&](std::size_t nn) {
      n = nn;
      w0 = w / n;
      n0 = n - w % n;
      y0 = (u128(1) << w0) * (R >> w0);
      y1 = (u128(1) << (w0 + 1)) * (R >> (w0 + 1));
    };
    std::size_t c = (w + m - 1) / m;
    setup(c);
    if (!(R - y0 <= y0 / n)) setup(c + 1);
  }
  u128 operator()() {
    u128 S = 0, u;
    for (std::size_t k = 0; k != n0; ++k) {
      do u = u128(e()) - Engine::min(); while (u >= y0);
      S = (S << w0) + (u & ((u128(1) << w0) - 1));
    }
    for (std::size_t k = n0; k != n; ++k) {
      do u = u128(e()) - Engine::min(); while (u >= y1);
      S = (S << (w0 + 1)) + (u & ((u128(1) << (w0 + 1)) - 1));
    }
    return S;
  }
};

template <class Engine, std::size_t w, class UInt>
void compare(const Engine& base, int count) {
  using IB = std::independent_bits_engine<Engine, w, UInt>;
  static_assert(IB::min() == 0);
  static_assert(IB::max() == UInt((u128(1) << w) - 1));
  IB a(base);
  ref_ibits<Engine, w> ref(base);
  for (int i = 0; i < count; ++i) {
    UInt v = a();
    u128 r = ref();
    CHECK(r < (u128(1) << w));
    CHECK(v == UInt(r));
  }
  CHECK(a.base() == ref.e);  // same number of base invocations, including rejections
}

using small = std::linear_congruential_engine<std::uint32_t, 3u, 1u, 10u>;  // values 0..9, R = 10
using odd = std::linear_congruential_engine<std::uint32_t, 16807u, 0u, 2147483647u>;  // 1..2^31-2

int main() {
  compare<std::minstd_rand, 64, std::uint64_t>(std::minstd_rand(), 1000);
  compare<std::minstd_rand, 32, std::uint32_t>(std::minstd_rand(5), 1000);
  compare<std::minstd_rand0, 10, unsigned short>(std::minstd_rand0(), 1000);
  compare<std::mt19937, 64, std::uint64_t>(std::mt19937(), 1000);
  compare<std::mt19937, 1, std::uint32_t>(std::mt19937(), 1000);
  compare<std::mt19937, 33, std::uint64_t>(std::mt19937(), 1000);
  compare<std::mt19937_64, 64, std::uint64_t>(std::mt19937_64(), 1000);
  compare<std::mt19937_64, 63, std::uint64_t>(std::mt19937_64(), 1000);
  compare<std::mt19937_64, 16, std::uint16_t>(std::mt19937_64(), 1000);
  compare<std::ranlux24, 48, std::uint64_t>(std::ranlux24(), 300);
  compare<small, 7, std::uint32_t>(small(4), 1000);
  compare<small, 32, std::uint32_t>(small(4), 1000);
  compare<small, 1, unsigned short>(small(4), 1000);
  compare<odd, 61, std::uint64_t>(odd(3), 1000);

  // Full-width passthrough: w equal to the base engine's width and R a power of two.
  std::independent_bits_engine<std::mt19937, 32, std::uint32_t> pass;
  std::mt19937 m;
  for (int i = 0; i < 100; ++i) CHECK(pass() == m());

  // Constructors, seeding, equality, discard.
  using IB = std::independent_bits_engine<std::minstd_rand, 40, std::uint64_t>;
  static_assert(std::is_same_v<IB::result_type, std::uint64_t>);
  static_assert(std::is_same_v<decltype(std::declval<const IB&>().base()), const std::minstd_rand&>);
  static_assert(noexcept(std::declval<const IB&>().base()));
  IB a;
  CHECK(a.base() == std::minstd_rand());
  IB b(17);
  CHECK(b.base() == std::minstd_rand(17));
  rs::pattern_seq q1, q2;
  IB c(q1);
  CHECK(c.base() == std::minstd_rand(q2));
  IB d(std::minstd_rand(17));
  CHECK(d == b);
  b();
  CHECK(d != b);
  d();
  CHECK(d == b);
  b.seed();
  CHECK(b == IB());
  b.seed(3);
  CHECK(b == IB(3));
  IB e, f;
  e.discard(25);
  for (int i = 0; i < 25; ++i) f();
  CHECK(e == f);
}
