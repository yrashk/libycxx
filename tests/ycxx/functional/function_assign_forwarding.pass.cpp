// [func.wrap.func.con]/27: template<class F> operator=(F&& f) "Effects: As if by:
// function(std::forward<F>(f)).swap(*this);" and /13 the target is "direct-non-list-initialized
// with std::forward<F>(f)": an lvalue argument is copied, an rvalue argument is moved.
// /19: operator=(const function& f) "As if by function(f).swap(*this);" (so self-assignment
// keeps the target). /21: operator=(function&& f) "Replaces the target of *this with the target
// of f." /29: operator=(reference_wrapper<F>) noexcept stores the reference_wrapper. All return
// *this (/20, /22, /25, /28, /30).
#include <functional>
#include <typeinfo>
#include <utility>
#include "check.hpp"

struct Tracked {
  static inline int copies = 0, moves = 0;
  int v;
  explicit Tracked(int x) : v(x) {}
  Tracked(const Tracked& o) : v(o.v) { ++copies; }
  Tracked(Tracked&& o) noexcept : v(o.v) {
    o.v = -1;
    ++moves;
  }
  int operator()() const { return v; }
};

int main() {
  std::function<int()> f;
  Tracked t(5);
  Tracked::copies = Tracked::moves = 0;
  std::function<int()>& r1 = (f = t);  // lvalue: copied
  CHECK(&r1 == &f);
  CHECK(Tracked::copies >= 1 && t.v == 5 && f() == 5);

  Tracked u(6);
  Tracked::copies = 0;
  std::function<int()>& r2 = (f = std::move(u));  // rvalue: moved, never copied
  CHECK(&r2 == &f);
  CHECK(Tracked::copies == 0 && u.v == -1 && f() == 6);

  const Tracked c(7);
  Tracked::copies = 0;
  f = c;  // const lvalue: FD is Tracked, copied
  CHECK(Tracked::copies >= 1 && f() == 7 && f.target_type() == typeid(Tracked));

  // self copy-assignment keeps the target
  std::function<int()>& self = f;
  f = self;
  CHECK(f && f() == 7);

  // copy assignment yields an independent target
  std::function<int()> g;
  std::function<int()>& r3 = (g = f);
  CHECK(&r3 == &g && g() == 7 && g.target<Tracked>() != f.target<Tracked>());

  // move assignment takes the target over
  std::function<int()> h = [] { return 1; };
  std::function<int()>& r4 = (h = std::move(g));
  CHECK(&r4 == &h && h() == 7 && h.target_type() == typeid(Tracked));
  // move assignment from an empty function empties the destination
  std::function<int()> empty;
  h = std::move(empty);
  CHECK(!h);

  // reference_wrapper: the referenced object is used, nothing is copied
  Tracked w(8);
  Tracked::copies = 0;
  std::function<int()>& r5 = (h = std::ref(w));
  CHECK(&r5 == &h && Tracked::copies == 0);
  w.v = 9;
  CHECK(h() == 9);
  std::function<int()>& r6 = (h = std::cref(w));
  CHECK(&r6 == &h && h.target_type() == typeid(std::reference_wrapper<const Tracked>));

  // nullptr
  std::function<int()>& r7 = (h = nullptr);
  CHECK(&r7 == &h && !h);
  return 0;
}
