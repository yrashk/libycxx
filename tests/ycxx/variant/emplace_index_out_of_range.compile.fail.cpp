// [variant.mod]/5: emplace<I>: "Mandates: I < sizeof...(Types)."
#include <variant>

void f() {
  std::variant<int, long> v;
  v.emplace<2>(1);
}
