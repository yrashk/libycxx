// [variant.visit]/11: member visit is "Equivalent to: return std::visit(std::forward<Visitor>(vis), (V)self);"
// so the Mandates of [variant.visit]/5 (same type and value category) apply.
#include <variant>

void f() {
  std::variant<int, double> v;
  v.visit([](auto x) { return x; });
}
