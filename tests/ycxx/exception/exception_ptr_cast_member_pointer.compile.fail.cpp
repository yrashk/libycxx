// EXPECT-ERROR: error: static assertion failed[^\n]*std::exception_ptr_cast: Mandates: E is not a pointer or pointer\-to\-member type
// [propagation]/13: exception_ptr_cast: "Mandates: ... E is not a pointer or pointer-to-member
// type." [Note 5: "When E is a pointer or pointer-to-member type, a handler of type const E&
// can match without binding to the exception object itself."]
#include <exception>

struct S { int m; };
void f(const std::exception_ptr& p) { (void)std::exception_ptr_cast<int S::*>(p); }
