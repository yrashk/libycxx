// EXPECT-ERROR: error: static assertion failed[^\n]*optional::transform: invalid result type
// [optional.monadic]/8: transform: "Mandates: U is a valid contained type for optional."
// ([optional.optional.general]/2: remove_cvref_t<X> must not be in_place_t or nullopt_t.)
#include <optional>

void f() {
  std::optional<int> o(1);
  (void)o.transform([](int) { return std::nullopt; });
}
