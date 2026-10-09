// EXPECT-ERROR-GCC: error: no match for call[^\n]*std::copyable_function<
// EXPECT-ERROR-CLANG: error: no matching function for call to object[^\n]*std::copyable_function<
// [func.wrap.copy.class]: "R operator()(ArgTypes...) cv ref noexcept(noex);" -- for
// copyable_function<int() const&&> the call operator is const&&-qualified, so an lvalue cannot
// call it.
#include <functional>
#include <utility>

int f(std::copyable_function<int() const&&>& c) { return c(); }
