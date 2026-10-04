// [func.wrap.ref.class]: "R operator()(ArgTypes...) const noexcept(noex);" -- the parameters
// are exactly ArgTypes..., so function_ref<void(int&)> cannot be called with an rvalue.
#include <functional>
#include <utility>

void g(int&);
void f(std::function_ref<void(int&)> r) { r(42); }
