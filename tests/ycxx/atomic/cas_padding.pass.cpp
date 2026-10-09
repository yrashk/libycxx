// [atomics.types.operations]/23 and Note 7: compare-and-exchange "compares the value
// representation"; "padding bits that never participate in the object's value representation
// are ignored. As a consequence, the following code is guaranteed to avoid spurious failure:"
// struct padded { char clank = 0x42; /* Padding here. */ unsigned biff = 0xC0DEFEFE; };
// The padding of expected (and of the stored value) is given garbage on purpose.
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <cstring>
#include <new>
#include "check.hpp"

struct padded {
  char clank = 0x42;
  // Padding here.
  unsigned biff = 0xC0DEFEFE;
};
static_assert(sizeof(padded) > sizeof(char) + sizeof(unsigned), "the test needs padding");

static padded make(char c, unsigned b, unsigned char garbage) {
  alignas(padded) unsigned char buf[sizeof(padded)];
  std::memset(buf, garbage, sizeof buf);
  padded* p = ::new (buf) padded;  // default-init would leave padding as is; members set below
  p->clank = c;
  p->biff = b;
  padded r;
  std::memcpy(&r, p, sizeof r);  // copies the garbage padding bytes too
  return r;
}

int main() {
  std::atomic<padded> pad = {};
  padded expected = make(0x42, 0xC0DEFEFE, 0xAA), desired{0, 0};
  CHECK(pad.compare_exchange_strong(expected, desired));
  CHECK(pad.load().clank == 0 && pad.load().biff == 0);

  padded e2 = make(0, 0, 0x55);
  CHECK(pad.compare_exchange_strong(e2, make(1, 2, 0x33)));
  padded e3 = make(1, 2, 0xFF);
  CHECK(pad.compare_exchange_strong(e3, padded{}));

  // Weak CAS may fail spuriously with no specified retry bound. Its failure still reports
  // the stored value. One attempt covers that path; strong CAS completes the padding check.
  padded e4 = make(0x42, 0xC0DEFEFE, 0x11);
  const padded next = make(7, 7, 0x99);
  bool exchanged = pad.compare_exchange_weak(e4, next);
  CHECK(e4.clank == 0x42 && e4.biff == 0xC0DEFEFE);
  padded observed = pad.load();
  CHECK(observed.clank == (exchanged ? 7 : 0x42));
  CHECK(observed.biff == (exchanged ? 7u : 0xC0DEFEFEu));
  if (!exchanged) CHECK(pad.compare_exchange_strong(e4, next));
  CHECK(pad.load().clank == 7 && pad.load().biff == 7);

  // a real mismatch still fails and reports the current value
  padded e5 = make(7, 8, 0);
  CHECK(!pad.compare_exchange_strong(e5, padded{}));
  CHECK(e5.clank == 7 && e5.biff == 7);
  return 0;
}
