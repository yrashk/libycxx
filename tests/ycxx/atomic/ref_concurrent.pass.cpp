// [atomics.ref.generic.general]/4: "Atomic operations applied to an object through a
// referencing atomic_ref are atomic with respect to atomic operations applied through any other
// atomic_ref referencing the same object." Each thread creates its own atomic_ref; also a
// type that is not lock-free.
// FLAGS: -latomic -pthread
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <thread>
#include <vector>
#include "check.hpp"

struct Wide { long long a, b, c; };

int main() {
  constexpr int threads = 4, iters = 10000;
  alignas(std::atomic_ref<long>::required_alignment) long counter = 0;
  alignas(std::atomic_ref<Wide>::required_alignment) Wide w{0, 0, 0};
  std::vector<std::thread> ts;
  for (int t = 0; t < threads; ++t)
    ts.emplace_back([&] {
      for (int i = 0; i < iters; ++i) {
        std::atomic_ref<long>(counter).fetch_add(1);
        std::atomic_ref<Wide> rw(w);
        Wide e = rw.load();
        while (!rw.compare_exchange_weak(e, Wide{e.a + 1, e.b + 2, e.c + 3})) {}
      }
    });
  for (auto& th : ts) th.join();
  CHECK(counter == long(threads) * iters);
  CHECK(w.a == threads * iters && w.b == 2 * threads * iters && w.c == 3 * threads * iters);

  // wait / notify through different atomic_refs
  alignas(std::atomic_ref<int>::required_alignment) int flag = 0;
  std::thread t([&] {
    std::atomic_ref<int>(flag).wait(0);
    std::atomic_ref<int> r(flag);
    r.store(2);
    r.notify_one();
  });
  std::atomic_ref<int> r(flag);
  r.store(1);
  r.notify_one();
  for (int v = r.load(); v != 2; v = r.load()) r.wait(v);
  t.join();
  return 0;
}
