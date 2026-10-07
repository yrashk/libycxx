// A thread other than main ends the program with exit(): its thread_local objects are destroyed
// ([basic.start.term]/2: "as a result of that thread calling std::exit") and then its thread-exit
// actions run, before the static objects and the atexit functions ([futures.promise]/23, /26,
// [futures.task.members], [thread.condition.nonmember]/2-3; the checks are in
// at_thread_exit_program.hpp). The main thread stays blocked, its own state untouched.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <cstdlib>
#include <thread>
#include "at_thread_exit_program.hpp"

int main() {
  std::thread t([] {
    ate::register_actions();
    std::exit(0);
  });
  t.join(); // never returns: the program ends while main waits here
  return 1;
}
