// EXPECT-ERROR-GCC: error: no match for call[^\n]*volatile[^\n]*__binder
// EXPECT-ERROR-CLANG: error: no matching function for call to object[^\n]*volatile[^\n]*__binder
// [func.bind.bind]/4: "A program that attempts to invoke a volatile-qualified g is
// ill-formed."
#include <functional>

int f() { return 0; }

void test() {
  volatile auto g = std::bind(f);
  g();
}
