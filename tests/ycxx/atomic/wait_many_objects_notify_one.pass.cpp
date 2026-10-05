// notify_one on each of many atomic objects, each with one blocked waiter: every waiter is
// unblocked, whatever internal structure the implementation shares between objects.
//   [atomics.wait]/4: a waiting operation on M that "has blocked after observing the result of
//     X", where X precedes Y in M's modification order and Y happens before the notifying call,
//     "is eligible to be unblocked" by that call.
//   [atomics.types.operations]/34 notify_one: "Unblocks the execution of at least one atomic
//     waiting operation that is eligible to be unblocked by this call, if any such atomic waiting
//     operations exist." So notify_one on M unblocks M's waiter (the only one, here); waking a
//     thread that waits on another object instead does not meet that.
//   The same for atomic_ref ([atomics.ref.ops]/33), atomic_flag ([atomics.flag]/18) and for a
//   type that is not lock-free (a 64-byte struct), and the non-member atomic_notify_one.
// 400 waiters on 400 distinct objects (more objects than an implementation is likely to have
// distinct internal wait slots), all blocked before the notifications; then, in a scrambled
// order, each object's value is changed and notify_one is called on it once. The watchdog fails
// the test if a waiter is never unblocked.
// FLAGS: -latomic -pthread
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

struct Big {
  long v[8];
};

struct Objects {
  std::atomic<int> i{0};
  alignas(std::atomic_ref<long>::required_alignment) long r = 0;
  std::atomic_flag f;
  std::atomic<Big> big{Big{}};
  std::atomic<unsigned char> c{0};
};

constexpr int N = 400;

int main() {
  watchdog(30);
  std::unique_ptr<Objects[]> objs(new Objects[N]);
  std::atomic<int> started{0}, finished{0};
  std::vector<std::thread> ts;
  for (int k = 0; k < N; ++k)
    ts.emplace_back([&, k] {
      Objects& o = objs[static_cast<std::size_t>(k)];
      started.fetch_add(1);
      switch (k % 5) {
        case 0: o.i.wait(0); break;
        case 1: std::atomic_ref<long>(o.r).wait(0); break;
        case 2: o.f.wait(false); break;
        case 3: o.big.wait(Big{}); break;
        default: std::atomic_wait(&o.c, static_cast<unsigned char>(0)); break;
      }
      finished.fetch_add(1);
    });
  while (started.load() < N) std::this_thread::yield();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));  // let them block
  CHECK(finished.load() == 0);
  for (int n = 0; n < N; ++n) {
    const int k = (n * 157) % N;  // 157 is coprime with 400: every object once
    Objects& o = objs[static_cast<std::size_t>(k)];
    switch (k % 5) {
      case 0: o.i.store(1); o.i.notify_one(); break;
      case 1: { std::atomic_ref<long> r(o.r); r.store(1); r.notify_one(); break; }
      case 2: o.f.test_and_set(); o.f.notify_one(); break;
      case 3: { Big b{}; b.v[7] = 1; o.big.store(b); o.big.notify_one(); break; }
      default: o.c.store(1); std::atomic_notify_one(&o.c); break;
    }
  }
  for (auto& t : ts) t.join();
  CHECK(finished.load() == N);
  return 0;
}
