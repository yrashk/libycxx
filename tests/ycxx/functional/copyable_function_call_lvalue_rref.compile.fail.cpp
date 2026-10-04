// [func.wrap.copy.class]: "R operator()(ArgTypes...) cv ref noexcept(noex);" -- for
// copyable_function<int() const&&> the call operator is const&&-qualified, so an lvalue cannot
// call it.
#include <functional>
#include <utility>

int f(std::copyable_function<int() const&&>& c) { return c(); }
