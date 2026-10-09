// EXPECT-ERROR-GCC: error: no matching function [^\n]*std::variant<S, S>::variant\(
// EXPECT-ERROR-CLANG: error: no matching constructor for initialization of 'std::variant<S, S>'
// [variant.ctor]/15.5 and Note 2: the converting constructor requires FUN(std::forward<T>(t))
// to be well-formed; with two equally viable alternatives it is ambiguous and the
// construction is ill-formed.
#include <variant>

struct S { S(const char*) {} };

void f() {
  std::variant<S, S> v("abc");
  (void)v;
}
