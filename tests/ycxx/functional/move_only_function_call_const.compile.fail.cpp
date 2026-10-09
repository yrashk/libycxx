// EXPECT-ERROR-GCC: error: no match for call[^\n]*std::move_only_function<
// EXPECT-ERROR-CLANG: error: no matching function for call to object[^\n]*std::move_only_function<
// [func.wrap.move.class]: "R operator()(ArgTypes...) cv ref noexcept(noex);" -- for
// move_only_function<int()> cv is empty, so the call operator is not const-qualified and a
// const move_only_function cannot be called.
#include <functional>
#include <utility>

int f(const std::move_only_function<int()>& m) { return m(); }
