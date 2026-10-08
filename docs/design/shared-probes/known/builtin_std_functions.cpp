// FLAGS: -O0
// The compilers treat std::move, std::forward, std::addressof, std::as_const, std::move_if_noexcept
// (and Clang also std::forward_like, std::to_underlying ...) as builtins when declared in std:
// at -O0 no call is emitted. Checked by nm of the -O0 object (run-known.sh): the probe fails when
// an out-of-line std::move or std::forward is referenced or defined.
#include <utility>
#include <memory>
#include <cstdio>
template <class T> T&& fwd(T& t) { return std::forward<T>(t); }
int main() {
  int a = 1;
  int&& b = std::move(a);
  int* p = std::addressof(a);
  const int& c = std::as_const(a);
  int d = fwd(a);
  bool r = &b == p && &c == p && d == 1;
  std::puts(r ? "ok" : "FAIL");
  return r ? 0 : 1;
}
