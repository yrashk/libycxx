// [func.wrap.func.inv]/2: function::operator() "Returns: INVOKE<R>(f,
// std::forward<ArgTypes>(args)...)". [func.wrap.move.inv]/3, [func.wrap.copy.inv]/3,
// [func.wrap.ref.inv]/2: "Equivalent to: return INVOKE<R>(...);" [func.require]/2: for
// non-void R, INVOKE<R> is INVOKE(...) "implicitly converted to R". When the target returns
// a prvalue of type R the conversion is the identity, so ([dcl.init.general]/16.6.1) the
// result object is initialized directly: no copy or move of the return value at any level,
// however the wrappers are nested ([func.wrap.general]/2). Also holds for a non-movable R.
#include <functional>
#include "check.hpp"

struct Ret {
  static inline int copies = 0;
  static inline int moves = 0;
  int v;
  explicit Ret(int x) : v(x) {}
  Ret(const Ret& o) : v(o.v) { ++copies; }
  Ret(Ret&& o) noexcept : v(o.v) { ++moves; }
};

auto make = [](int x) { return Ret(x); };

template <class F>
void check(F& f, int v) {
  Ret::copies = Ret::moves = 0;
  Ret r = f(v);
  CHECK(r.v == v);
  CHECK(Ret::copies == 0);
  CHECK(Ret::moves == 0);
}

int main() {
  using Sig = Ret(int);
  std::function<Sig> fn{make};
  check(fn, 1);
  std::move_only_function<Sig> mo{make};
  check(mo, 2);
  std::copyable_function<Sig> cf{make};
  check(cf, 3);
  std::function_ref<Sig> fr{make};
  check(fr, 4);

  std::move_only_function<Sig> nested1{std::copyable_function<Sig>{make}};
  check(nested1, 5);
  std::function<Sig> nested2{std::copyable_function<Sig>{std::function<Sig>{make}}};
  check(nested2, 6);
  std::function_ref<Sig> nested3{nested1};
  check(nested3, 7);
  std::copyable_function<Sig> nested4{std::function_ref<Sig>{cf}};
  check(nested4, 8);
  return 0;
}
