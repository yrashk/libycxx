// [optional.observe]/15: value_or: "Mandates: is_copy_constructible_v<T> &&
// is_convertible_v<U&&, T> is true."
#include <optional>

struct E { explicit E(int) {} };

void f() {
  std::optional<E> o;
  (void)o.value_or(1);  // int is not implicitly convertible to E
}
