// [atomics.ref.generic.general]: "explicit atomic_ref(T&&) = delete;"
#include <atomic>

void f() {
  std::atomic_ref<int> r(42);
  (void)r;
}
