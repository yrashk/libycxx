// FLAGS: -Werror=dangling-gsl -Werror=dangling
// COMPILERS: clang  (GCC's -Wdangling-reference does not depend on the namespace)
// Clang treats the standard containers and views as gsl::Owner / gsl::Pointer by name (when
// declared in std) and diagnoses a string_view bound to a temporary string. This probe passes when
// the diagnostic is produced, i.e. when the compile FAILS: run-known.sh reports "COMPILE FAILED"
// with the dangling-gsl error, and "run: ok" would mean the warning was lost.
#include <string>
#include <string_view>
#include <cstdio>
int main() {
  std::string_view sv = std::string("a temporary string, longer than the small buffer");
  std::puts(sv.empty() ? "FAIL" : "ok");
  return 0;
}
