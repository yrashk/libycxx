// EXPECT-ERROR: error: static assertion failed[^\n]*variant_alternative index out of range
// [variant.get]/6: get<I>: "Mandates: I < sizeof...(Types)."
#include <variant>

void f() {
  std::variant<int, long> v;
  (void)std::get<2>(v);
}
