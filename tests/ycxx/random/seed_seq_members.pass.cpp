// [rand.util.seedseq]: result_type is uint_least32_t; seed_seq() noexcept leaves v empty;
// the iterator constructor stores each value mod 2^32 (/6); the initializer_list<T> constructor is
// the same (/3); size() (noexcept) is the number of 32-bit units param() copies (/11); param(dest)
// copies v (/15); seed_seq is neither copy constructible nor copy assignable.
#include <random>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include "check.hpp"
#include "test_iterators.hpp"

static_assert(std::is_same_v<std::seed_seq::result_type, std::uint_least32_t>);
static_assert(std::is_nothrow_default_constructible_v<std::seed_seq>);
static_assert(!std::is_copy_constructible_v<std::seed_seq>);
static_assert(!std::is_copy_assignable_v<std::seed_seq>);
static_assert(!std::is_move_constructible_v<std::seed_seq>);
static_assert(noexcept(std::declval<const std::seed_seq&>().size()));
static_assert(std::is_same_v<decltype(std::declval<const std::seed_seq&>().size()), std::size_t>);

int main() {
  std::seed_seq a;
  CHECK(a.size() == 0);
  std::uint32_t sentinel[1] = {42};
  a.param(sentinel);
  CHECK(sentinel[0] == 42);

  std::seed_seq b{-1LL, 0x100000005LL, 7LL};
  CHECK(b.size() == 3);
  std::uint32_t pb[3] = {};
  b.param(pb);
  CHECK(pb[0] == 0xffffffffu && pb[1] == 5u && pb[2] == 7u);

  const signed char sc[4] = {-1, -128, 0, 127};
  std::seed_seq c(sc, sc + 4);
  CHECK(c.size() == 4);
  std::uint64_t pc[4] = {};
  c.param(pc);
  CHECK(pc[0] == 0xffffffffu && pc[1] == 0xffffff80u && pc[2] == 0 && pc[3] == 127);

  // Single-pass input iterators are enough ([rand.util.seedseq]/5).
  const unsigned long long big[3] = {0xffffffffffffffffull, 0x123456789ull, 3};
  std::seed_seq d(InputIter<const unsigned long long>(big), InputIter<const unsigned long long>(big + 3));
  std::uint32_t pd[3] = {};
  d.param(pd);
  CHECK(d.size() == 3 && pd[0] == 0xffffffffu && pd[1] == 0x23456789u && pd[2] == 3);

  // Equal input gives an equal sequence; param round-trips.
  std::seed_seq e(pb, pb + 3);
  std::uint32_t g1[20], g2[20];
  b.generate(g1, g1 + 20);
  e.generate(g2, g2 + 20);
  for (int i = 0; i < 20; ++i) CHECK(g1[i] == g2[i]);

  // initializer_list of an unsigned type and of a narrow unsigned type.
  std::seed_seq f{1u, 2u};
  CHECK(f.size() == 2);
  std::seed_seq g{std::uint16_t(65535)};
  std::uint32_t pg[1];
  g.param(pg);
  CHECK(pg[0] == 65535u);
}
