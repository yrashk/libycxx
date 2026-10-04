// [func.wrap.copy.ctor]/14: explicit copyable_function(in_place_type_t<T>, Args&&...):
// "Mandates: VT is the same type as T, and is_copy_constructible_v<VT> is true."
#include <functional>
#include <utility>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly(const MoveOnly&) = delete;
  int operator()() const { return 0; }
};

void test() { std::copyable_function<int() const> f(std::in_place_type<MoveOnly>); }
