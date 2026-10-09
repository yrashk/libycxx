// EXPECT-ERROR: error: static assertion failed[^\n]*std::exception_ptr_cast: Mandates: E is a cv\-unqualified complete object type
// [propagation]/13: exception_ptr_cast: "Mandates: E is a cv-unqualified complete object
// type." (volatile-qualified here)
#include <exception>

void f(const std::exception_ptr& p) { (void)std::exception_ptr_cast<volatile int>(p); }
