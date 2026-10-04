// [func.wrap.move.class]: "R operator()(ArgTypes...) cv ref noexcept(noex);" -- for
// move_only_function<int() &> the call operator is &-qualified and not const, so a const lvalue
// cannot call it.
#include <functional>
#include <utility>

int f(const std::move_only_function<int() &>& m) { return m(); }
