// [func.wrap.move.ctor]/18: explicit move_only_function(in_place_type_t<T>,
// initializer_list<U>, Args&&...): "Mandates: VT is the same type as T." (VT = decay_t<T>; here T is const-qualified.)
#include <functional>
#include <initializer_list>
#include <utility>

struct Fn {
  Fn(std::initializer_list<int>) {}
  int operator()() const { return 0; }
};

void test() { std::move_only_function<int() const> f(std::in_place_type<const Fn>, {1, 2}); }
