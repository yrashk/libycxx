// EXPECT-ERROR: error: static assertion failed[^\n]*thread: the callable is not invocable with the decayed arguments
// [thread.thread.constr]/5.3 and /6: the copies are passed as rvalues (auto(...)), so a
// function taking a non-const lvalue reference is not invocable with decay_t<FArgs>:
// "Mandates: is_invocable_v<decay_t<F>, decay_t<FArgs>...>" (use std::ref instead).
#include <thread>

void f(int&);
void g() {
  int x = 0;
  std::thread t(f, x);
}
