// [rand.adapt.shuf]: constructors fill V[0..k-1] and then Y with successive e() (/6); each step
// computes j = floor(k * (Y - emin) / (emax - emin + 1)), sets Y to V[j] and V[j] to e(), and
// yields Y. table_size is k; min() and max() are those of the base engine.
#include <random>
#include <cstdint>
#include <type_traits>
#include "check.hpp"
#include "random_support.hpp"

using u128 = unsigned __int128;

template <class Engine, std::size_t k>
struct ref_shuf {
  using T = typename Engine::result_type;
  Engine e;
  T V[k];
  T Y;
  explicit ref_shuf(const Engine& b) : e(b) {
    for (std::size_t i = 0; i < k; ++i) V[i] = e();
    Y = e();
  }
  T operator()() {
    u128 span = u128(Engine::max()) - Engine::min() + 1;
    std::size_t j = std::size_t(u128(k) * (u128(Y) - Engine::min()) / span);
    Y = V[j];
    V[j] = e();
    return Y;
  }
};

template <class Engine, std::size_t k>
void compare(const Engine& base, int count) {
  std::shuffle_order_engine<Engine, k> a(base);
  ref_shuf<Engine, k> ref(base);
  Engine advanced = base;
  advanced.discard(k + 1);
  CHECK(a.base() == advanced);  // k + 1 invocations by the constructor
  for (int i = 0; i < count; ++i) CHECK(a() == ref());
  CHECK(a.base() == ref.e);
}

using small = std::linear_congruential_engine<std::uint32_t, 3u, 1u, 10u>;
using S = std::shuffle_order_engine<std::minstd_rand, 5>;
static_assert(S::table_size == 5);
static_assert(std::is_same_v<decltype(S::table_size), const std::size_t>);
static_assert(S::min() == std::minstd_rand::min() && S::max() == std::minstd_rand::max());
static_assert(std::is_same_v<decltype(std::declval<const S&>().base()), const std::minstd_rand&>);
static_assert(noexcept(std::declval<const S&>().base()));

int main() {
  compare<std::minstd_rand0, 256>(std::minstd_rand0(), 3000);  // knuth_b's parameters
  compare<std::minstd_rand, 5>(std::minstd_rand(8), 2000);
  compare<std::mt19937_64, 7>(std::mt19937_64(), 2000);  // emax - emin + 1 == 2^64
  compare<std::mt19937, 1>(std::mt19937(), 500);
  compare<small, 3>(small(1), 500);

  std::knuth_b kb;
  ref_shuf<std::minstd_rand0, 256> rk(std::minstd_rand0{});
  for (int i = 0; i < 100; ++i) CHECK(kb() == rk());

  S a;
  CHECK(a == S(std::minstd_rand()));
  S b(17);
  CHECK(b == S(std::minstd_rand(17)));
  rs::pattern_seq q1, q2;
  S c(q1);
  CHECK(c == S(std::minstd_rand(q2)));
  S copy(b);
  for (int i = 0; i < 50; ++i) CHECK(copy() == b());

  // seed(): e == E() and the same future sequence (V and Y are refilled).
  b.seed();
  CHECK(b == S());
  S fresh;
  for (int i = 0; i < 50; ++i) CHECK(b() == fresh());
  b.seed(99);
  CHECK(b == S(99));
  S fresh99(99);
  for (int i = 0; i < 50; ++i) CHECK(b() == fresh99());
  rs::pattern_seq q3, q4;
  b.seed(q3);
  S fq(q4);
  CHECK(b == fq);
  for (int i = 0; i < 50; ++i) CHECK(b() == fq());

  S u, v;
  u.discard(300);
  for (int i = 0; i < 300; ++i) v();
  CHECK(u == v);
  CHECK(u() == v());
}
