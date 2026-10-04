// [variant.get]/1: holds_alternative<T>: "Mandates: The type T occurs exactly once in Types."
#include <variant>

void f() {
  std::variant<int, int> v;
  (void)std::holds_alternative<int>(v);
}
