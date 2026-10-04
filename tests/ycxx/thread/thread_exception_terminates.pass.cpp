// [thread.thread.constr]/6: "If the invocation of invoke terminates with an uncaught exception,
// terminate is invoked ([except.terminate])."
// FLAGS: -pthread
#include <thread>
#include <exception>
#include <cstdlib>

int main() {
  std::set_terminate([] { std::_Exit(0); });
  std::thread t([] { throw 1; });
  t.join();
  return 1;  // not reached
}
