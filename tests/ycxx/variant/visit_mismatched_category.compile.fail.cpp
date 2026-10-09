// EXPECT-ERROR: error: static assertion failed[^\n]*std::visit: the visitor must return the same type and value category for all alternatives
// [variant.visit]/5: "All such expressions are of the same type and value category."
// int& vs int (lvalue vs prvalue).
#include <variant>

struct F {
  int x = 0;
  int& operator()(int) { return x; }
  int operator()(double) { return 0; }
};

void f() {
  std::variant<int, double> v;
  F fn;
  std::visit(fn, v);
}
