// Death tests of hardened preconditions (tests/ycxx/precondition). [structure.specifications]/3.5:
// in a hardened implementation, a hardened precondition is checked "prior to any other observable
// side effects of the function" by a contract assertion "evaluated with a terminating semantic".
//
// A test sets up its objects, calls about_to_violate(), then makes the one call whose hardened
// precondition does not hold. Its `// EXPECT-TERMINATE: about to violate` requires the program to
// end abnormally after that line was printed, so a crash during the setup does not count; the
// line after the violating call, never_reached(), makes a normal end a failure in the output too.
#pragma once
#include <cstdio>
#include <cstdlib>

inline void about_to_violate(const char* what) {
  std::fprintf(stderr, "about to violate: %s\n", what);
}

// Keeps a result (a reference or a value) observable, so that the call is not discarded.
template <class T>
void keep(const T& x) {
  [[maybe_unused]] static const void* volatile sink;
  sink = __builtin_addressof(x);
}

[[noreturn]] inline void never_reached() {
  std::fputs("the violated precondition did not terminate the program\n", stderr);
  std::exit(0);
}
