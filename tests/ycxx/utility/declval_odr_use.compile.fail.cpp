// EXPECT-ERROR: error: static assertion failed[^\n]*std::declval can only be used in unevaluated contexts
// [declval]/2: "Mandates: This function is not odr-used ([basic.def.odr])."
#include <utility>

int test() { return std::declval<int>(); }
