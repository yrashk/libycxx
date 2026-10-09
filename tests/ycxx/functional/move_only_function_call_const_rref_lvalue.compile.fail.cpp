// EXPECT-ERROR-GCC: error: no match for call[^\n]*std::move_only_function<
// EXPECT-ERROR-CLANG: error: no matching function for call to object[^\n]*std::move_only_function<
// [func.wrap.move.class]: "R operator()(ArgTypes...) cv ref noexcept(noex);" -- for
// move_only_function<int() const &&> the call operator is const&&-qualified, so even a const
// lvalue cannot call it.
#include <functional>
#include <utility>

int f(const std::move_only_function<int() const&&>& m) { return m(); }
