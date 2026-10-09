// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::atomic_ref[^\n]*atomic_ref\(_Tp&&\)
// EXPECT-ERROR-CLANG: error: call to deleted constructor of 'std::atomic_ref<int>'
// [atomics.ref.generic.general]: "explicit atomic_ref(T&&) = delete;"
#include <atomic>

void f() {
  std::atomic_ref<int> r(42);
  (void)r;
}
