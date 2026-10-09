// EXPECT-ERROR-GCC: error: invalid application of 'sizeof' to incomplete type [^\n]*Incomplete
// EXPECT-ERROR-CLANG: error: invalid application of 'sizeof' to an incomplete type [^\n]*Incomplete
// [numeric.ops.midpoint]/4-5: midpoint(T* a, T* b): "Constraints: T is an object type."
// "Mandates: T is a complete type." An incomplete object type satisfies the constraint, so
// the call is selected and ill-formed.
#include <numeric>

struct Incomplete;

int main() {
  Incomplete* p = nullptr;
  (void)std::midpoint(p, p);
  return 0;
}
