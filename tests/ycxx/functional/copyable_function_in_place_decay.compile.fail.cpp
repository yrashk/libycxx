// [func.wrap.copy.ctor]/14: explicit copyable_function(in_place_type_t<T>, Args&&...):
// "Mandates: VT is the same type as T" (VT = decay_t<T>; here T is const-qualified).
#include <functional>
#include <utility>

struct Fn {
  int operator()() const { return 0; }
};

void test() { std::copyable_function<int() const> f(std::in_place_type<const Fn>); }
