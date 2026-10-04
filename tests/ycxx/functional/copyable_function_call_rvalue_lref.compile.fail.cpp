// [func.wrap.copy.class]: "R operator()(ArgTypes...) cv ref noexcept(noex);" -- for
// copyable_function<int() &> the call operator is &-qualified, so an rvalue cannot call it.
#include <functional>
#include <utility>

int f(std::copyable_function<int() &> c) { return std::move(c)(); }
