// EXPECT-ERROR: error: static assertion failed[^\n]*std::not_fn<f>: f must not be a null pointer
// [func.not.fn]/7: template<auto f> not_fn(): "Mandates: If is_pointer_v<F> ||
// is_member_pointer_v<F> is true, then f != nullptr is true." (null pointer to member)
#include <functional>

struct S {
  bool b;
};
constexpr bool S::*null_md = nullptr;
auto g = std::not_fn<null_md>();
