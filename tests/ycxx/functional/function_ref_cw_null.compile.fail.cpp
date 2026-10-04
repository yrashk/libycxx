// [func.wrap.ref.ctor]/11: function_ref(constant_wrapper<c, F> f): "Mandates: If
// is_pointer_v<F> || is_member_pointer_v<F> is true, then f.value != nullptr is true".
#include <functional>
#include <utility>

constexpr int (*null_fp)(int) = nullptr;

void test() { std::function_ref<int(int)> r(std::cw<null_fp>); }
