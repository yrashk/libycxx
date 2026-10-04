// [func.wrap.move.class]: "R operator()(ArgTypes...) cv ref noexcept(noex);" -- for
// move_only_function<int() &&> the call operator is &&-qualified, so an lvalue cannot call it.
#include <functional>
#include <utility>

int f(std::move_only_function<int() &&>& m) { return m(); }
