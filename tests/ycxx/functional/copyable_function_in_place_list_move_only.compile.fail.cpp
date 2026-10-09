// EXPECT-ERROR: error: static assertion failed[^\n]*std::function/move_only_function/copyable_function: Mandates: VT is copy constructible
// [func.wrap.copy.ctor]/20: explicit copyable_function(in_place_type_t<T>,
// initializer_list<U>, Args&&...): "Mandates: VT is the same type as T, and
// is_copy_constructible_v<VT> is true."
#include <functional>
#include <initializer_list>
#include <utility>

struct MoveOnly {
  MoveOnly(std::initializer_list<int>) {}
  MoveOnly(MoveOnly&&) = default;
  MoveOnly(const MoveOnly&) = delete;
  int operator()() const { return 0; }
};

void test() { std::copyable_function<int() const> f(std::in_place_type<MoveOnly>, {1}); }
