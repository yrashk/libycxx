// EXPECT-ERROR: error: static assertion failed[^\n]*optional::and_then: F must return an optional
// [optional.monadic]/2: and_then: "Mandates: remove_cvref_t<U> is a specialization of optional."
#include <optional>

void f() {
  std::optional<int> o(1);
  (void)o.and_then([](int x) { return x; });
}
