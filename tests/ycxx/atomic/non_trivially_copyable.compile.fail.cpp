// [atomics.types.generic.general]/1: "The program is ill-formed if any of
// is_trivially_copyable_v<T>, ... is false."
#include <atomic>

struct NTC {
  int v;
  NTC() = default;
  NTC(const NTC& o) : v(o.v) {}
  NTC& operator=(const NTC&) = default;
};

std::atomic<NTC> a;
