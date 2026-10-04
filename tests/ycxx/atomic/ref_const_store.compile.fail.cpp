// [atomics.ref.ops]/11: store: "Constraints: is_const_v<T> is false."
#include <atomic>

void f(const int& c) {
  std::atomic_ref<const int> r(c);
  r.store(1);
}
