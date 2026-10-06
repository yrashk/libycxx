// [atomics.types.operations]/31 (atomic<T>::wait) and [atomics.ref.ops]/29 (atomic_ref<T>::wait):
// wait(old) "Evaluates load(order) and compares its value representation for equality against
// that of old. If they compare unequal, returns. Blocks until it is unblocked by an atomic
// notifying operation or is unblocked spuriously." So it keeps waiting while the value
// representations are equal even where == says otherwise (a NaN with the same payload), and
// padding bits, not part of the value representation, are ignored (cf. Note 7 of
// [atomics.types.operations] for compare_exchange). Each wait below would return at once if it
// compared with == or with the padding, before the other thread changed the value; the test
// checks that it returned only after that change.
// FLAGS: -latomic -pthread
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <chrono>
#include <cstring>
#include <limits>
#include <new>
#include <thread>
#include "check.hpp"

struct padded {
  char c = 0x42;
  // padding
  unsigned u = 0xC0DEFEFE;
  friend bool operator==(const padded& a, const padded& b) { return a.c == b.c && a.u == b.u; }
};
static_assert(sizeof(padded) > sizeof(char) + sizeof(unsigned));

padded with_padding(char c, unsigned u, unsigned char garbage) {
  alignas(padded) unsigned char buf[sizeof(padded)];
  std::memset(buf, garbage, sizeof buf);
  padded* p = ::new (buf) padded;
  p->c = c;
  p->u = u;
  padded r;
  std::memcpy(&r, p, sizeof r);
  return r;
}

// waits on w with old, while another thread changes the value to next after a delay
template <class W, class T>
void check_blocks(W& w, T old, T next) {
  std::atomic<bool> changed{false};
  std::thread t([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    changed.store(true);
    w.store(next);
    w.notify_all();
  });
  w.wait(old);
  CHECK(changed.load());
  t.join();
}

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  {
    std::atomic<double> a(nan);
    check_blocks(a, nan, 1.0);
  }
  {
    alignas(std::atomic_ref<double>::required_alignment) double x = nan;
    std::atomic_ref<double> r(x);
    check_blocks(r, nan, 2.0);
  }
  {
    std::atomic<padded> a(with_padding(1, 2, 0x00));
    check_blocks(a, with_padding(1, 2, 0xA5), with_padding(3, 4, 0x00));
  }
  {
    alignas(std::atomic_ref<padded>::required_alignment) padded p = with_padding(5, 6, 0x5A);
    std::atomic_ref<padded> r(p);
    check_blocks(r, with_padding(5, 6, 0xFF), with_padding(7, 8, 0x00));
  }
  {
    // the converse: == would say equal (+0.0 == -0.0), the representations differ: no blocking
    std::atomic<double> z(0.0);
    z.wait(-0.0);
    alignas(std::atomic_ref<float>::required_alignment) float f = -0.0f;
    std::atomic_ref<float>(f).wait(0.0f);
  }
  return 0;
}
