// [func.bind.bind]/4: "A program that attempts to invoke a volatile-qualified g is
// ill-formed."
#include <functional>

int f() { return 0; }

void test() {
  volatile auto g = std::bind(f);
  g();
}
