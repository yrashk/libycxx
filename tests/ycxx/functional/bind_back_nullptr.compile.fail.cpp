// EXPECT-ERROR: error: static assertion failed[^\n]*std::bind_back<f>: f must not be a null pointer
// [func.bind.partial]/7.3: template<auto f, class... Args> bind_back(args...): "Mandates:
// ... if is_pointer_v<F> || is_member_pointer_v<F> is true, then f != nullptr is true."
#include <functional>

constexpr int (*null_fn)(int, int) = nullptr;
auto g = std::bind_back<null_fn>(1);
