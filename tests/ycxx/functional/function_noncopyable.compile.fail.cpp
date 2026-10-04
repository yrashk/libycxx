// [func.wrap.func.con]/10: template<class F> function(F&& f): "Mandates:
// is_copy_constructible_v<FD> is true".
#include <functional>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly(const MoveOnly&) = delete;
  int operator()() const { return 0; }
};

void test() { std::function<int()> f(MoveOnly{}); }
