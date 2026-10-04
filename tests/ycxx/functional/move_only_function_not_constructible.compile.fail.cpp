// [func.wrap.move.ctor]/6: template<class F> move_only_function(F&& f): "Mandates:
// is_constructible_v<VT, F> is true." (VT = decay_t<F>; here F = const MoveOnly&, whose
// copy constructor is deleted, while is-callable-from<VT> holds.)
#include <functional>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly(const MoveOnly&) = delete;
  int operator()() const { return 0; }
};

void test() {
  const MoveOnly m;
  std::move_only_function<int() const> f(m);
}
