// [variant.assign]/12.3 and Note 1: converting assignment is ill-formed when FUN overload
// resolution is ambiguous ("variant<string, string> v; v = "abc"; is ill-formed").
#include <variant>

struct S { S(const char*) {} S() = default; };

void f() {
  std::variant<S, S> v;
  v = "abc";
}
