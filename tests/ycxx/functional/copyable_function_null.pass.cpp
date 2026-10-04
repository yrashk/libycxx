// [func.wrap.copy.ctor]/10: "Postconditions: *this has no target object if any of the
// following hold: f is a null function pointer value, or f is a null member pointer value, or
// remove_cvref_t<F> is a specialization of the copyable_function class template, and f has
// no target object."
#include <functional>
#include <utility>
#include "check.hpp"

struct S {
  int v;
  int get() const { return v; }
};

int main() {
  int (*nullfp)(int) = nullptr;
  std::copyable_function<int(int)> a(nullfp);
  CHECK(!a);
  int (S::*nullpmf)() const = nullptr;
  std::copyable_function<int(const S&)> b(nullpmf);
  CHECK(!b);
  int S::*nullpmd = nullptr;
  std::copyable_function<int(S&)> c(nullpmd);
  CHECK(!c);
  // an empty copyable_function of another specialization
  std::copyable_function<int(int) const> e;
  std::copyable_function<long(int)> d(e);
  CHECK(!d);
  std::copyable_function<void(int) &&> d2(std::move(e));
  CHECK(!d2);
  std::copyable_function<int(int) const noexcept> en;
  std::copyable_function<int(int)> d3(en);
  CHECK(!d3);
  // non-empty source of another specialization is wrapped
  std::copyable_function<int(int) const> full = [](int x) { return x + 1; };
  std::copyable_function<long(int)> w(full);
  CHECK(w && w(1) == 2);
  // assignment uses the same constructor
  w = nullfp;
  CHECK(!w);
  w = full;
  w = std::copyable_function<int(int) const>();
  CHECK(!w);
  return 0;
}
