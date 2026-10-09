// EXPECT-ERROR: error: static assertion failed[^\n]*std::bind_back<f>: f must not be a null pointer
// [func.bind.partial]/7.3: template<auto f, class... Args> bind_back(args...): "Mandates: ...
// if is_pointer_v<F> || is_member_pointer_v<F> is true, then f != nullptr is true." (null
// pointer to data member)
#include <functional>

struct S {
  int v;
};
constexpr int S::*null_md = nullptr;
auto g = std::bind_back<null_md>();
