// EXPECT-ERROR-GCC: error: no match for 'operator=' \(operand types are 'std::variant<S, S>'
// EXPECT-ERROR-CLANG: error: no viable overloaded '='
// EXPECT-ERROR-CLANG: note: candidate template ignored: substitution failure[^\n]*call to 'fun' is ambiguous
// [variant.assign]/12.3 and Note 1: converting assignment is ill-formed when FUN overload
// resolution is ambiguous ("variant<string, string> v; v = "abc"; is ill-formed").
#include <variant>

struct S { S(const char*) {} S() = default; };

void f() {
  std::variant<S, S> v;
  v = "abc";
}
