// EXPECT-ERROR: error: static assertion failed[^\n]*std::bind_front<f>: f must not be a null pointer
// [func.bind.partial]/7.3: template<auto f, class... Args> bind_front(args...): "Mandates:
// ... if is_pointer_v<F> || is_member_pointer_v<F> is true, then f != nullptr is true."
#include <functional>

struct S { int g(int) const; };
constexpr int (S::*null_mf)(int) const = nullptr;
auto g = std::bind_front<null_mf>(S{});
