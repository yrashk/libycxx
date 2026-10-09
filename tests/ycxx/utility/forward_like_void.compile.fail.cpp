// EXPECT-ERROR-GCC: error: no matching function for call to 'forward_like<void>\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'forward_like'
// [forward]/5: forward_like "Mandates: T is a referenceable type ([defns.referenceable])."
// void is not referenceable, so the program is ill-formed.
#include <utility>

void f() {
  int i = 0;
  (void)std::forward_like<void>(i);
}
