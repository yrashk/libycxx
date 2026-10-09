// EXPECT-ERROR-GCC: error: static assertion failed[^\n]*optional::value_or: Mandates not met
// EXPECT-ERROR-GCC: error: could not convert 'int' to 'E'
// EXPECT-ERROR-CLANG: error: static assertion failed[^\n]*is_convertible_v<int &&, E>[^\n]*optional::value_or
// [optional.observe]/15: value_or: "Mandates: is_copy_constructible_v<T> &&
// is_convertible_v<U&&, T> is true."
#include <optional>

struct E { explicit E(int) {} };

void f() {
  std::optional<E> o;
  (void)o.value_or(1);  // int is not implicitly convertible to E
}
