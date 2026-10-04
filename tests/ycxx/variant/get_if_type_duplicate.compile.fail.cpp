// [variant.get]/12: get_if<T>: "Mandates: The type T occurs exactly once in Types."
#include <variant>

void f() {
  std::variant<int, long, int> v;
  (void)std::get_if<int>(&v);
}
