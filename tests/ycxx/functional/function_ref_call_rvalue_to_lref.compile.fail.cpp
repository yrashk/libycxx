// EXPECT-ERROR-GCC: error: no match for call[^\n]*std::function_ref<
// EXPECT-ERROR-CLANG: error: no matching function for call to object[^\n]*std::function_ref<
// [func.wrap.ref.class]: "R operator()(ArgTypes...) const noexcept(noex);" -- the parameters
// are exactly ArgTypes..., so function_ref<void(int&)> cannot be called with an rvalue.
#include <functional>
#include <utility>

void g(int&);
void f(std::function_ref<void(int&)> r) { r(42); }
