// [atomics.ref.generic.general]/2: "The program is ill-formed if is_trivially_copyable_v<T>
// is false."
#include <atomic>

struct NTC {
  int v;
  NTC() = default;
  NTC(const NTC& o) : v(o.v) {}
};

void f(NTC& n) {
  std::atomic_ref<NTC> r(n);
  (void)r;
}
