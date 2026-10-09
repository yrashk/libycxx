// EXPECT-ERROR: error: static assertion failed[^\n]*std::function_ref: Mandates: the call does not produce a constant_wrapper
// [func.wrap.ref.ctor]/11.2: function_ref(constant_wrapper<c, F> f): "Mandates: ... if
// ArgTypes is not an empty pack and all types in remove_cvref_t<ArgTypes>... satisfy
// constexpr-param then constant_wrapper<INVOKE(f.value, remove_cvref_t<ArgTypes>::value...)>
// is not a valid type."
#include <functional>
#include <utility>

constexpr int twice(int x) { return 2 * x; }

void test() { std::function_ref<int(decltype(std::cw<3>))> r(std::cw<twice>); }
