// EXPECT-ERROR-GCC: error: no matching function for call to 'invoke\(OnlyInt, double&\)'
// EXPECT-ERROR-CLANG: error: no matching function for call to 'invoke'
// [variant.visit]/5: "Mandates: For each valid pack m, e(m) is a valid expression."
#include <variant>

struct OnlyInt { void operator()(int) const {} void operator()(double) const = delete; };

void f() {
  std::variant<int, double> v;
  std::visit(OnlyInt{}, v);
}
