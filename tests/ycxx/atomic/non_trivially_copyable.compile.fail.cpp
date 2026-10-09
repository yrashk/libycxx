// EXPECT-ERROR: error: static assertion failed[^\n]*atomic<T> needs a trivially copyable, copy and move constructible and assignable T
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
