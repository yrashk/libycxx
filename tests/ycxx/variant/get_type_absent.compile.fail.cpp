// [variant.get]/8: get<T>: "Mandates: The type T occurs exactly once in Types."
// (T does not occur at all.)
#include <variant>

void f() {
  std::variant<int, long> v;
  (void)std::get<char>(v);
}
