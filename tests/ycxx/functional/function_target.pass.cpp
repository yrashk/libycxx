// [func.wrap.func.targ]/1: target_type() "Returns: If *this has a target of type T,
// typeid(T); otherwise, typeid(void)." /2: target<T>() "Returns: If target_type() ==
// typeid(T) a pointer to the stored function target; otherwise a null pointer."
// [func.wrap.func.con]/8,13: the target has type FD = decay_t<F>.
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include "check.hpp"

int one() { return 1; }
struct Fn {
  int v;
  int operator()() const { return v; }
};

static_assert(noexcept(std::declval<const std::function<int()>&>().target_type()));
static_assert(noexcept(std::declval<std::function<int()>&>().target<Fn>()));
static_assert(std::is_same_v<decltype(std::declval<std::function<int()>&>().target<Fn>()), Fn*>);
static_assert(
    std::is_same_v<decltype(std::declval<const std::function<int()>&>().target<Fn>()), const Fn*>);

int main() {
  std::function<int()> empty;
  CHECK(empty.target_type() == typeid(void));
  CHECK(empty.target<Fn>() == nullptr);

  std::function<int()> f = Fn{7};
  CHECK(f.target_type() == typeid(Fn));
  Fn* p = f.target<Fn>();
  CHECK(p != nullptr && p->v == 7);
  p->v = 8;  // the pointer designates the stored target
  CHECK(f() == 8);
  CHECK(f.target<int (*)()>() == nullptr);
  const std::function<int()>& cf = f;
  CHECK(cf.target<Fn>() == p);

  // function names decay to function pointers
  std::function<int()> g = one;
  CHECK(g.target_type() == typeid(int (*)()));
  int (**pp)() = g.target<int (*)()>();
  CHECK(pp != nullptr && *pp == &one);
  CHECK(g.target<Fn>() == nullptr);

  // const-qualified argument decays: FD = Fn
  const Fn cfn{3};
  std::function<int()> h = cfn;
  CHECK(h.target_type() == typeid(Fn));
  CHECK(h.target<Fn>() != nullptr);

  // a copy has a distinct copy of the target
  std::function<int()> k = f;
  CHECK(k.target<Fn>() != f.target<Fn>());
  CHECK(k.target<Fn>()->v == 8);

  // reference_wrapper is stored as such
  Fn r{5};
  std::function<int()> w = std::ref(r);
  CHECK(w.target_type() == typeid(std::reference_wrapper<Fn>));
  r.v = 6;
  CHECK(w() == 6);
  return 0;
}
