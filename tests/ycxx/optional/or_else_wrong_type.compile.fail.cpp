// EXPECT-ERROR: error: static assertion failed[^\n]*optional::or_else: F must return optional
// [optional.monadic]/14: or_else: "Mandates: is_same_v<remove_cvref_t<invoke_result_t<F>>,
// optional> is true."
#include <optional>

void f() {
  std::optional<int> o;
  (void)o.or_else([] { return std::optional<long>(1); });
}
