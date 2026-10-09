// EXPECT-ERROR: error: static assertion failed[^\n]*thread: the callable is not invocable with the decayed arguments
// [thread.thread.constr]/5.3: "Mandates: ... is_invocable_v<decay_t<F>, decay_t<FArgs>...>"
#include <thread>

void f(int*);
void g() {
  std::thread t(f, 1.5);
}
