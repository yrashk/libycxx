// [rand.req.eng] Table 127 for every engine and adaptor of [rand.eng], [rand.adapt], [rand.predef]:
// E() gives the same state as other default-constructed engines; E(x) compares equal to x;
// e.seed(), e.seed(s), e.seed(q) give e == E(), E(s), E(q); e.discard(z) is equivalent to z calls;
// == is an equivalence relation on future sequences and != is its negation; E meets
// Cpp17CopyConstructible and Cpp17CopyAssignable ([rand.req.eng]/5); E is a uniform random bit
// generator ([rand.req.urng]): min() <= e() <= max().
#include <random>
#include <concepts>
#include <cstdint>
#include <type_traits>
#include "check.hpp"
#include "random_support.hpp"

template <class E>
void check_engine() {
  using T = typename E::result_type;
  static_assert(std::uniform_random_bit_generator<E>);
  static_assert(std::is_same_v<std::invoke_result_t<E&>, T>);
  static_assert(std::is_unsigned_v<T>);
  static_assert(E::min() < E::max());
  static_assert(std::is_copy_constructible_v<E> && std::is_copy_assignable_v<E>);
  static_assert(std::is_default_constructible_v<E>);
  static_assert(requires(E e, const E x, const E y, T s, std::seed_seq& q, unsigned long long z) {
    { e.seed() } -> std::same_as<void>;
    { e.seed(s) } -> std::same_as<void>;
    { e.seed(q) } -> std::same_as<void>;
    { e() } -> std::same_as<T>;
    { e.discard(z) } -> std::same_as<void>;
    { x == y } -> std::same_as<bool>;
    { x != y } -> std::same_as<bool>;
  });

  E a, b;
  CHECK(a == b && !(a != b));
  for (int i = 0; i < 100; ++i) {
    T v = a();
    CHECK(E::min() <= v && v <= E::max());
    CHECK(v == b());
  }
  CHECK(a == b);
  a();
  CHECK(a != b && !(a == b) && b != a);
  b();
  CHECK(a == b);

  // Copy construction and assignment.
  E c(a);
  CHECK(c == a);
  E d(T(7));
  CHECK(d != a);
  d = a;
  CHECK(d == a);
  for (int i = 0; i < 20; ++i) CHECK(c() == d());

  // Seeding.
  E s1(T(12345)), s2(T(12345)), s3(T(54321));
  CHECK(s1 == s2);
  CHECK(s1 != s3);
  for (int i = 0; i < 20; ++i) CHECK(s1() == s2());
  a.seed();
  CHECK(a == E());
  a.seed(T(12345));
  CHECK(a == E(T(12345)));
  std::seed_seq q1{3, 1, 4, 1, 5}, q2{3, 1, 4, 1, 5};
  E e1(q1);
  a.seed(q2);
  CHECK(a == e1);
  rs::pattern_seq p1, p2;
  E f1(p1);
  E f2(T(1));
  f2.seed(p2);
  CHECK(f1 == f2);
  CHECK(p1.calls == 1 && p2.calls == 1);  // E(q) depends on one call to q.generate

  // discard(z) is equivalent to z calls.
  for (unsigned long long z : {0ull, 1ull, 2ull, 3ull, 17ull, 1000ull}) {
    E u(T(99)), v(T(99));
    u.discard(z);
    for (unsigned long long i = 0; i < z; ++i) v();
    CHECK(u == v);
    CHECK(u() == v());
  }

  // Equivalence relation: reflexive and symmetric (transitivity follows from the sequences).
  const E x(T(5)), y(T(5));
  CHECK(x == x && x == y && y == x);
}

using lcg_small = std::linear_congruential_engine<std::uint32_t, 3u, 1u, 10u>;

int main() {
  check_engine<std::minstd_rand0>();
  check_engine<std::minstd_rand>();
  check_engine<std::mt19937>();
  check_engine<std::mt19937_64>();
  check_engine<std::ranlux24_base>();
  check_engine<std::ranlux48_base>();
  check_engine<std::ranlux24>();
  check_engine<std::ranlux48>();
  check_engine<std::knuth_b>();
  check_engine<std::philox4x32>();
  check_engine<std::philox4x64>();
  check_engine<std::default_random_engine>();
  check_engine<std::linear_congruential_engine<std::uint64_t, 6364136223846793005ull, 1442695040888963407ull, 0>>();
  check_engine<std::subtract_with_carry_engine<std::uint32_t, 11, 3, 7>>();
  check_engine<std::discard_block_engine<std::mt19937, 7, 3>>();
  check_engine<std::independent_bits_engine<std::minstd_rand, 48, std::uint64_t>>();
  check_engine<std::independent_bits_engine<lcg_small, 5, unsigned short>>();
  check_engine<std::shuffle_order_engine<std::mt19937_64, 3>>();
  check_engine<std::philox_engine<std::uint32_t, 32, 2, 10, 0xD256D193, 0x9E3779B9>>();
  // Adaptors of adaptors.
  check_engine<std::shuffle_order_engine<std::discard_block_engine<std::minstd_rand, 3, 2>, 4>>();
  check_engine<std::independent_bits_engine<std::knuth_b, 64, std::uint64_t>>();
}
