// EXPECT-ERROR-GCC: error: invalid application of 'sizeof' to incomplete type [^\n]*Incomplete
// EXPECT-ERROR-CLANG: error: invalid application of 'sizeof' to an incomplete type [^\n]*Incomplete
// [propagation]/13: exception_ptr_cast: "Mandates: E is a cv-unqualified complete object
// type."
#include <exception>

struct Incomplete;
void f(const std::exception_ptr& p) { (void)std::exception_ptr_cast<Incomplete>(p); }
