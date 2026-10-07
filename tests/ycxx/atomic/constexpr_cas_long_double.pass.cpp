// [atomics.types.float]/1 and [atomics.ref.float]/1: compare_exchange_weak/strong are constexpr
// for every floating-point type (P3309); [atomics.types.operations]/23: they compare value
// representations. long double's padding bytes (x87: 6 of 16) are indeterminate in constant
// evaluation and must not take part (Clang rejected the comparison).
#include <atomic>

constexpr bool cas_atomic() {
  std::atomic<long double> a{0.5L};
  long double expected = 0.5L;
  if (!a.compare_exchange_strong(expected, 2.0L)) return false;
  expected = 3.0L;
  if (a.compare_exchange_strong(expected, 4.0L) || expected != 2.0L) return false;
  while (!a.compare_exchange_weak(expected, 1.5L)) {}
  return a.load() == 1.5L;
}

constexpr bool cas_ref() {
  long double v = 0.25L;
  std::atomic_ref<long double> r(v);
  long double expected = 0.25L;
  if (!r.compare_exchange_strong(expected, 8.0L, std::memory_order::acq_rel, std::memory_order::acquire))
    return false;
  expected = 1.0L;
  return !r.compare_exchange_strong(expected, 9.0L) && expected == 8.0L && v == 8.0L;
}

static_assert(cas_atomic());
static_assert(cas_ref());

int main() { return cas_atomic() && cas_ref() ? 0 : 1; }
