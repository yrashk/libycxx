// [atomics.ref.generic.general]/4: "Atomic operations applied to an object through a
// referencing atomic_ref are atomic with respect to atomic operations applied through any other
// atomic_ref referencing the same object." Distinct members of one struct are distinct memory
// locations ([intro.memory]/3), so concurrent atomic_ref operations on neighbouring members
// (including char and short members sharing a word with others) must not disturb one another:
// every member ends with exactly the expected value. [atomics.ref.float]/[atomics.types.float]:
// fetch_add/fetch_sub on floating-point objects are atomic read-modify-write operations; with
// exactly representable increments the sum is exact whatever the interleaving. The same for
// atomic<float>/atomic<double>. [atomics.ref.int] fetch_max ([atomics.types.int]).
// FLAGS: -latomic -pthread
#include <atomic>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

struct S {
  int a;
  char c1;
  char c2;
  short s;
  long b;
  float f;
  double d;
  unsigned char flags;
};

int main() {
  watchdog(20);
  constexpr int threads = 6, iters = 3000;
  S s{};
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < threads; ++t)
      ts.emplace_back([&s] {
        std::atomic_ref ra(s.a);
        std::atomic_ref rc1(s.c1);
        std::atomic_ref rc2(s.c2);
        std::atomic_ref rs(s.s);
        std::atomic_ref rb(s.b);
        std::atomic_ref rf(s.f);
        std::atomic_ref rd(s.d);
        std::atomic_ref rfl(s.flags);
        for (int i = 0; i < iters; ++i) {
          ++ra;
          rc1.fetch_add(1);  // wraps; checked modulo 256
          rc2.fetch_xor(1);
          rs.fetch_add(1);
          rb.fetch_sub(2);
          rf.fetch_add(1.0f);
          rd += 0.5;
          rfl.fetch_or(static_cast<unsigned char>(1u << (i % 8)));
        }
      });
  }
  CHECK(s.a == threads * iters);
  CHECK(static_cast<unsigned char>(s.c1) == static_cast<unsigned char>(threads * iters % 256));
  CHECK(s.c2 == 0);  // an even number of xors
  CHECK(s.s == threads * iters);
  CHECK(s.b == -2L * threads * iters);
  CHECK(s.f == static_cast<float>(threads * iters));
  CHECK(s.d == 0.5 * threads * iters);
  CHECK(s.flags == 0xFF);

  int m = 0;
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < 4; ++t)
      ts.emplace_back([&m, t] {
        std::atomic_ref r(m);
        for (int i = 0; i < 1000; ++i) r.fetch_max(t * 1000 + i);
      });
  }
  CHECK(m == 3999);

  std::atomic<float> af{0.0f};
  std::atomic<double> ad{0.0};
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < 8; ++t)
      ts.emplace_back([&] {
        for (int i = 0; i < 2000; ++i) {
          af.fetch_add(0.5f);
          af.fetch_sub(0.25f);
          ad.fetch_sub(0.25);
        }
      });
  }
  CHECK(af.load() == 8 * 2000 * 0.25f);
  CHECK(ad.load() == -8 * 2000 * 0.25);
  return 0;
}
