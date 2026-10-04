// [func.not.fn]/7: template<auto f> not_fn(): "Mandates: If is_pointer_v<F> ||
// is_member_pointer_v<F> is true, then f != nullptr is true."
#include <functional>

constexpr bool (*null_fn)(int) = nullptr;
auto g = std::not_fn<null_fn>();
