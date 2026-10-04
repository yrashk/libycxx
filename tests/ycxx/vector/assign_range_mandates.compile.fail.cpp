// [sequence.reqmts]/61: a.assign_range(rg): "Mandates: assignable_from<T&,
// ranges::range_reference_t<R>> is modeled." Here the range's elements are convertible to T
// (so R is a container-compatible-range<T> and the call is not rejected by a constraint) but
// T& is not assignable from them, so the program is ill-formed.
#include <vector>

struct Src {
  int v;
};
struct T {
  int v = 0;
  T() = default;
  T(const Src& s) : v(s.v) {}
  T& operator=(const T&) = default;
  T& operator=(const Src&) = delete;
};

void f() {
  Src src[2] = {{1}, {2}};
  std::vector<T> v;
  v.assign_range(src);
}
