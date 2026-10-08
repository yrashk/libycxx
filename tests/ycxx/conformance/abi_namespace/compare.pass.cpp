// The inline ABI namespace std::__y1 (DECISIONS §20.4-20.5): a construct the compilers handle
// specially still works with libycxx's declarations.
// The compiler names the comparison category types: built-in <=> and defaulted operator<=>.
#include <compare>
#include <cstdio>
struct S {
  int a; double b;
  auto operator<=>(const S&) const = default;
  bool operator==(const S&) const = default;
};
int main() {
  bool r = (1 <=> 2) < 0 && (1.0 <=> 2.0) == std::partial_ordering::less && is_lt(S{1, 2.0} <=> S{1, 3.0}) &&
           (static_cast<void*>(nullptr) <=> static_cast<void*>(nullptr)) == 0 && S{1, 2.0} == S{1, 2.0};
  std::puts(r ? "ok" : "FAIL");
  return r ? 0 : 1;
}
