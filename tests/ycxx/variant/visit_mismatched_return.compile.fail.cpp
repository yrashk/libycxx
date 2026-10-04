// [variant.visit]/5: "Mandates: For each valid pack m, e(m) is a valid expression.
// All such expressions are of the same type and value category."
#include <variant>

void f() {
  std::variant<int, double> v;
  std::visit([](auto x) { return x; }, v);  // int vs double
}
