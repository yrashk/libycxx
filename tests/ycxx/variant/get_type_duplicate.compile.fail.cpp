// EXPECT-ERROR: error: static assertion failed[^\n]*std::get<T>: T must occur exactly once
// [variant.get]/8: get<T>: "Mandates: The type T occurs exactly once in Types."
#include <variant>

void f() {
  std::variant<int, int> v;
  (void)std::get<int>(v);
}
