// [atomics.fences]: atomic_thread_fence / atomic_signal_fence (extern "C", constexpr, noexcept);
// /2: "A release fence A synchronizes with an acquire fence B if there exist atomic operations
// X and Y ... A is sequenced before X, X modifies M, Y is sequenced before B, and Y reads the
// value written by X". A message-passing check with relaxed operations and fences.
// FLAGS: -pthread
#include <atomic>
#include <thread>
#include "check.hpp"

static_assert(noexcept(std::atomic_thread_fence(std::memory_order::seq_cst)));
static_assert(noexcept(std::atomic_signal_fence(std::memory_order::seq_cst)));

int main() {
  std::atomic_thread_fence(std::memory_order::relaxed);
  std::atomic_thread_fence(std::memory_order::acquire);
  std::atomic_thread_fence(std::memory_order::release);
  std::atomic_thread_fence(std::memory_order::acq_rel);
  std::atomic_thread_fence(std::memory_order::seq_cst);
  std::atomic_signal_fence(std::memory_order::seq_cst);
  std::atomic_signal_fence(std::memory_order::acq_rel);

  for (int round = 0; round < 50; ++round) {
    int payload = 0;
    std::atomic<bool> flag(false);
    std::thread producer([&] {
      payload = round + 1;
      std::atomic_thread_fence(std::memory_order::release);
      flag.store(true, std::memory_order::relaxed);
    });
    while (!flag.load(std::memory_order::relaxed)) {}
    std::atomic_thread_fence(std::memory_order::acquire);
    CHECK(payload == round + 1);
    producer.join();
  }
  return 0;
}
