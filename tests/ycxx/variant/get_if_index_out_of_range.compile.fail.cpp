// [variant.get]/10: get_if<I>: "Mandates: I < sizeof...(Types)."
#include <variant>

void f() {
  std::variant<int, long> v;
  (void)std::get_if<2>(&v);
}
