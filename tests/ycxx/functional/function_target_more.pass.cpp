// [func.wrap.func.targ]/1: target_type() "Returns: If *this has a target of type T, typeid(T);
// otherwise, typeid(void)." /2: target<T>() "Returns: If target_type() == typeid(T) a pointer
// to the stored function target; otherwise a null pointer." typeid ignores top-level cv
// ([expr.typeid]/5), so target<const T>() finds a target of type T (as a const T*).
// [func.wrap.func.con]/13: the target has type FD = decay_t<F>; /12: no target when f is a null
// function / member pointer or an empty function specialization; /6: a moved-to function has
// the source's target; [func.wrap.func.mod] swap exchanges targets; /23 operator=(nullptr_t).
#include <functional>
#include <typeinfo>
#include <utility>
#include "check.hpp"

struct Fn {
  int v;
  int operator()(int x) const { return v + x; }
};
struct S {
  int m;
  int get(int) const { return m; }
};
int plain(int x) { return x; }

int main() {
  std::function<int(int)> f = Fn{1};
  // cv-qualified T: typeid(const Fn) == typeid(Fn)
  const Fn* cp = f.target<const Fn>();
  CHECK(cp != nullptr && cp == f.target<Fn>());
  CHECK(f.target<const volatile Fn>() != nullptr);

  // member pointers are stored as such
  std::function<int(const S&, int)> pm = &S::get;
  CHECK(pm.target_type() == typeid(int (S::*)(int) const));
  CHECK(*pm.target<int (S::*)(int) const>() == &S::get);
  std::function<int(const S&)> dm = &S::m;
  CHECK(dm.target_type() == typeid(int S::*));

  // a lambda's closure type
  auto lam = [](int x) { return -x; };
  std::function<int(int)> fl = lam;
  CHECK(fl.target_type() == typeid(decltype(lam)));
  CHECK(fl.target<decltype(lam)>() != nullptr);

  // a function of another signature is stored as a target of that type
  std::function<long(int)> other = plain;
  std::function<int(int)> wrapped = other;
  CHECK(wrapped.target_type() == typeid(std::function<long(int)>));
  CHECK((*wrapped.target<std::function<long(int)>>())(3) == 3L);
  // ...unless it is empty
  std::function<long(int)> empty_other;
  std::function<int(int)> nowrap = empty_other;
  CHECK(nowrap.target_type() == typeid(void));

  // null pointers give no target
  int (*nfp)(int) = nullptr;
  std::function<int(int)> fromnull = nfp;
  CHECK(fromnull.target_type() == typeid(void) && fromnull.target<int (*)(int)>() == nullptr);
  int (S::*nmp)(int) const = nullptr;
  std::function<int(const S&, int)> fromnullmp = nmp;
  CHECK(fromnullmp.target_type() == typeid(void));

  // move and swap carry the target type
  std::function<int(int)> moved = std::move(f);
  CHECK(moved.target_type() == typeid(Fn) && moved.target<Fn>()->v == 1);
  std::function<int(int)> p = plain;
  moved.swap(p);
  CHECK(moved.target_type() == typeid(int (*)(int)) && p.target_type() == typeid(Fn));
  // assignment replaces it
  p = std::ref(*p.target<Fn>());
  CHECK(p.target_type() == typeid(std::reference_wrapper<Fn>));
  p = nullptr;
  CHECK(p.target_type() == typeid(void) && p.target<Fn>() == nullptr);
  p = lam;
  CHECK(p.target_type() == typeid(decltype(lam)));
  return 0;
}
