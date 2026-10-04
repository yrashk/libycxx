// [func.wrap.func.con]/12: "Postconditions: !*this is true if any of the following hold:
// f is a null function pointer value. f is a null member pointer value.
// remove_cvref_t<F> is a specialization of the function class template, and !f is true."
// /6: function(function&& f): "If !f, *this has no target".
#include <functional>
#include <utility>
#include "check.hpp"

struct S {
  int v;
  int get() const { return v; }
};

int main() {
  int (*nullfp)(int) = nullptr;
  std::function<int(int)> a(nullfp);
  CHECK(!a);
  std::function<long(int)> a2 = nullfp;  // also via INVOKE<R> conversions
  CHECK(!a2);

  int (S::*nullpmf)() const = nullptr;
  std::function<int(const S&)> b(nullpmf);
  CHECK(!b);
  int S::*nullpmd = nullptr;
  std::function<int(const S&)> c(nullpmd);
  CHECK(!c);

  // an empty function of a different specialization
  std::function<int(int)> e;
  std::function<long(int)> d(e);
  CHECK(!d);
  std::function<void(int)> d2(std::move(e));
  CHECK(!d2);
  std::function<int(int)> e2(e);
  CHECK(!e2);

  // assignment uses the same constructor
  std::function<int(int)> g = [](int x) { return x; };
  g = nullfp;
  CHECK(!g);
  g = [](int x) { return x; };
  g = std::function<short(int)>();
  CHECK(!g);
  return 0;
}
