// [func.wrap.move.ctor]/8: "Postconditions: *this has no target object if any of the
// following hold: f is a null function pointer value, f is a null member pointer value, or
// remove_cvref_t<F> is a specialization of the move_only_function or copyable_function class
// template, and f has no target object."
#include <functional>
#include <utility>
#include "check.hpp"

struct S {
  int v;
  int get() const { return v; }
};

int main() {
  int (*nullfp)() = nullptr;
  std::move_only_function<int()> a(nullfp);
  CHECK(!a && a == nullptr);
  std::move_only_function<long() const noexcept> a2 = static_cast<int (*)() noexcept>(nullptr);
  CHECK(!a2);

  int (S::*nullpmf)() const = nullptr;
  std::move_only_function<int(const S&)> b(nullpmf);
  CHECK(!b);
  int S::*nullpmd = nullptr;
  std::move_only_function<int(const S&)> c(nullpmd);
  CHECK(!c);

  // empty wrapper of another move_only_function specialization
  std::move_only_function<int() const> e1;
  std::move_only_function<int()> d1(std::move(e1));
  CHECK(!d1);
  std::move_only_function<int() &&> e2;
  std::move_only_function<void() &&> d2(std::move(e2));
  CHECK(!d2);
  // empty copyable_function
  std::copyable_function<int() const> e3;
  std::move_only_function<int()> d3(e3);
  CHECK(!d3);
  std::move_only_function<int()> d4(std::move(e3));
  CHECK(!d4);
  // non-empty ones are wrapped
  std::move_only_function<int() const> full = [] { return 3; };
  std::move_only_function<long()> wrapped(std::move(full));
  CHECK(wrapped && wrapped() == 3);
  std::copyable_function<int()> cf = [] { return 4; };
  std::move_only_function<int()> fromcf(cf);
  CHECK(fromcf() == 4 && cf() == 4);

  // assignment goes through the same constructor
  std::move_only_function<int()> g = [] { return 1; };
  g = nullfp;
  CHECK(!g);
  g = [] { return 1; };
  g = std::move_only_function<int() const>();
  CHECK(!g);
  return 0;
}
