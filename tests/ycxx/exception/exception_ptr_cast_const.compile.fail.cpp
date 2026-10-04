// [propagation]: exception_ptr_cast: "Mandates: E is a cv-unqualified complete object type."
#include <exception>

void f(const std::exception_ptr& p) { (void)std::exception_ptr_cast<const int>(p); }
