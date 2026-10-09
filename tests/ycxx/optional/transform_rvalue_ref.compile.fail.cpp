// EXPECT-ERROR: error: static assertion failed[^\n]*optional::transform: invalid result type
// [optional.monadic]/8: transform: "Mandates: U is a valid contained type for optional."
// An rvalue reference result is not a valid contained type (only lvalue references are).
#include <optional>

int g = 0;
void f() {
  std::optional<int> o(1);
  (void)o.transform([](int) -> int&& { return static_cast<int&&>(g); });
}
