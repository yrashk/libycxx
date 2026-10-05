// [func.wrap.func.inv]/2: operator() "Throws: bad_function_call if !*this". [func.wrap.badcall]:
// bad_function_call publicly derives from exception and overrides what(), which returns an
// implementation-defined NTBS. [exception]/2: "Each standard library class T that derives from
// class exception has ... a publicly accessible copy constructor and a publicly accessible copy
// assignment operator that do not exit with an exception. These member functions meet the
// following postcondition: If two objects lhs and rhs both have dynamic type T and lhs is a
// copy of rhs, then strcmp(lhs.what(), rhs.what()) is equal to 0."
// [func.wrap.func.con]/12: no target from a null function pointer, a null member pointer, or an
// empty function of any signature -- calling any of these throws bad_function_call, whatever
// R and the argument types are.
// REQUIRES: exceptions
#include <functional>
#include <cstring>
#include <exception>
#include <utility>
#include "check.hpp"

struct S {
  int m;
  int get() const { return m; }
};

template <class F, class... A>
bool throws_bad_call(const F& f, A&&... a) {
  try {
    f(std::forward<A>(a)...);
  } catch (const std::bad_function_call& e) {
    const char* w = e.what();
    CHECK(w != nullptr);
    std::bad_function_call copy(e);
    CHECK(std::strcmp(copy.what(), w) == 0);
    std::bad_function_call assigned;
    assigned = e;
    CHECK(std::strcmp(assigned.what(), w) == 0);
    const std::exception& base = e;
    CHECK(std::strcmp(base.what(), w) == 0);  // virtual dispatch
    return true;
  }
  return false;
}

int main() {
  CHECK(throws_bad_call(std::function<void()>()));
  int z = 0;
  CHECK(throws_bad_call(std::function<int&(int&)>(), z));
  CHECK(throws_bad_call(std::function<void(int, double)>(nullptr), 1, 2.0));
  void (*nfp)() = nullptr;
  CHECK(throws_bad_call(std::function<void()>(nfp)));
  int (S::*nmf)() const = nullptr;
  CHECK(throws_bad_call(std::function<int(const S&)>(nmf), S{1}));
  int S::*nmd = nullptr;
  CHECK(throws_bad_call(std::function<int(const S&)>(nmd), S{1}));
  std::function<long()> empty_long;
  CHECK(throws_bad_call(std::function<int()>(empty_long)));

  // a non-empty function wrapping an empty one of another signature: no target per
  // [func.wrap.func.con]/12.3, so the outer call throws
  std::function<void()> outer = std::function<int()>();
  CHECK(!outer);
  CHECK(throws_bad_call(outer));

  // bad_function_call is caught by a handler for std::exception, and the arguments' side
  // effects happen before the throw
  int evaluated = 0;
  std::function<void(int)> e;
  try {
    e(++evaluated);
    CHECK(false);
  } catch (const std::exception&) {
  }
  CHECK(evaluated == 1);
  return 0;
}
