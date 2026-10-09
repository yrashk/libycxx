// EXPECT-ERROR: error: static assertion failed[^\n]*std::function/move_only_function/copyable_function: Mandates: VT is copy constructible
// [func.wrap.copy.ctor]/8: template<class F> copyable_function(F&& f): "Mandates:
// is_constructible_v<VT, F> is true, and is_copy_constructible_v<VT> is true."
#include <functional>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly(const MoveOnly&) = delete;
  int operator()() const { return 0; }
};

void test() { std::copyable_function<int() const> f(MoveOnly{}); }
