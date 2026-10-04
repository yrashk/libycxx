// [func.wrap.copy.class]: "R operator()(ArgTypes...) cv ref noexcept(noex);" -- for
// copyable_function<int(int)> cv is empty, so a const copyable_function cannot be called.
#include <functional>
#include <utility>

int f(const std::copyable_function<int(int)>& c) { return c(1); }
