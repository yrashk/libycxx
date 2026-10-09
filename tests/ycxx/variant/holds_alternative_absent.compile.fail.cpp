// EXPECT-ERROR: error: static assertion failed[^\n]*holds_alternative: T must occur exactly once
// [variant.get]/1: holds_alternative<T>: "Mandates: The type T occurs exactly once in Types."
#include <variant>

void f() {
  std::variant<int, long> v;
  (void)std::holds_alternative<double>(v);
}
