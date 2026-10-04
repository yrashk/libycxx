// [atomics.types.int]/6: fetch_key are "atomic read-modify-write operations"; concurrent
// increments are never lost. [intro.races]/... : RMW operations read the last value in the
// modification order. Also a CAS loop (Example 1 of [atomics.types.operations]) and fetch_max.
// FLAGS: -pthread
#include <atomic>
#include <thread>
#include <vector>
#include "check.hpp"

int main() {
  constexpr int threads = 4, iters = 20000;
  std::atomic<long> counter(0);
  std::atomic<long> cas_counter(0);
  std::atomic<int> maxv(0);
  std::atomic<unsigned> bits(0);
  std::vector<std::thread> ts;
  for (int t = 0; t < threads; ++t)
    ts.emplace_back([&, t] {
      for (int i = 0; i < iters; ++i) {
        counter.fetch_add(1, std::memory_order::relaxed);
        long e = cas_counter.load();
        while (!cas_counter.compare_exchange_weak(e, e + 2)) {}
        maxv.fetch_max(t * iters + i);
      }
      bits.fetch_or(1u << t);
    });
  for (auto& th : ts) th.join();
  CHECK(counter.load() == long(threads) * iters);
  CHECK(cas_counter.load() == 2L * threads * iters);
  CHECK(maxv.load() == threads * iters - 1);
  CHECK(bits.load() == (1u << threads) - 1);
  return 0;
}
