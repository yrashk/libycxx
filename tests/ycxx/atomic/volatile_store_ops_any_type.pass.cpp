// [atomics.types.int]/1, [atomics.types.float]/1, [atomics.types.pointer]/1: the volatile
// store_add ... store_fminimum_num members are declared for every specialization;
// [depr.atomics.volatile]/1 deprecates them only where is_always_lock_free is false (a
// non-lock-free long double here), it does not remove them.
// FLAGS: -Wno-deprecated-declarations
#include <atomic>

template<class A, class V>
concept fp_stores = requires(volatile A& a, V v) {
  a.store_add(v); a.store_sub(v); a.store_max(v); a.store_min(v);
  a.store_fmaximum(v); a.store_fminimum(v); a.store_fmaximum_num(v); a.store_fminimum_num(v);
  a.store_add(v, std::memory_order::relaxed);
};
template<class A, class V>
concept int_stores = requires(volatile A& a, V v) {
  a.store_add(v); a.store_sub(v); a.store_and(v); a.store_or(v); a.store_xor(v);
  a.store_max(v); a.store_min(v);
};
template<class A>
concept ptr_stores = requires(volatile A& a) {
  a.store_add(1); a.store_sub(1); a.store_max(nullptr); a.store_min(nullptr);
};

static_assert(fp_stores<std::atomic<float>, float>);
static_assert(fp_stores<std::atomic<double>, double>);
static_assert(fp_stores<std::atomic<long double>, long double>);
static_assert(int_stores<std::atomic<long long>, long long>);
static_assert(int_stores<std::atomic<char>, char>);
static_assert(ptr_stores<std::atomic<int*>>);

int main() {
  volatile std::atomic<long double> a{1.0L};
  a.store_add(2.0L);
  a.store_fminimum_num(0.5L, std::memory_order::relaxed);
  return a.load() == 0.5L ? 0 : 1;
}
