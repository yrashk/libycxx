// [propagation]/13: exception_ptr_cast: "Mandates: E is a cv-unqualified complete object
// type. E is not an array type. E is not a pointer or pointer-to-member type."
#include <exception>

void f(const std::exception_ptr& p) { (void)std::exception_ptr_cast<int[2]>(p); }
