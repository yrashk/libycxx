// [func.wrap.ref.ctor]/15: function_ref(constant_wrapper<c, F> f, U&& obj): "Mandates: If
// is_pointer_v<F> || is_member_pointer_v<F> is true, then f.value != nullptr is true."
#include <functional>
#include <utility>

struct S {
  int get() const { return 0; }
};
constexpr int (S::*null_mf)() const = nullptr;

void test(S& s) { std::function_ref<int()> r(std::cw<null_mf>, s); }
