// [func.wrap.ref.class]: the constructors from F&& and from constant_wrapper (with or without a
// bound object) are constexpr, so a function_ref can be constant-initialized.
// [func.wrap.ref.ctor]/12: function_ref(constant_wrapper<c, F> f) "Initializes bound-entity with
// a pointer to an unspecified object or null pointer value" -- it refers to no user object, so
// it stays valid regardless of the lifetime of anything the caller created. /16, /20: the bound
// object forms refer to obj (by address / by the given pointer).
#include <functional>
#include <utility>
#include "check.hpp"

constexpr int twice(int x) { return 2 * x; }
struct S {
  int v;
  int get() const { return v; }
};
struct Adder {
  int k;
  int operator()(int x) const { return x + k; }
};
S global_s{5};
Adder global_adder{100};

constinit std::function_ref<int(int)> g1 = std::cw<twice>;
constinit std::function_ref<int()> g2(std::cw<&S::get>, global_s);
constinit std::function_ref<int()> g3(std::cw<&S::get>, &global_s);
constinit std::function_ref<int(int)> g4 = global_adder;
constinit std::function_ref<int(int)> g5 = std::cw<[](int x) { return x + 1; }>;

std::function_ref<int(int)> make() {
  // no object is referenced: returning it is fine
  return std::cw<twice>;
}
std::function_ref<int(int)> make_lambda() {
  return std::cw<[](int x) { return x * x; }>;
}

int main() {
  CHECK(g1(4) == 8);
  CHECK(g2() == 5 && g3() == 5);
  global_s.v = 6;
  CHECK(g2() == 6 && g3() == 6);
  CHECK(g4(1) == 101);
  global_adder.k = 200;
  CHECK(g4(1) == 201);
  CHECK(g5(1) == 2);
  CHECK(make()(21) == 42);
  CHECK(make_lambda()(5) == 25);
  // copies of a constant_wrapper-initialized function_ref are as good as the original
  auto copy = make();
  { auto inner = make_lambda(); copy = inner; }
  CHECK(copy(3) == 9);
  return 0;
}
