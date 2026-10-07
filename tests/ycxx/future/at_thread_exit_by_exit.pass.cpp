// The thread-exit actions of the main thread run when it calls exit() ([support.start.term]/9.1:
// "First, objects with thread storage duration and associated with the current thread are
// destroyed. Next, objects with static storage duration are destroyed and functions registered by
// calling atexit are called."), after its thread_local objects are destroyed and before the
// static objects and the atexit functions ([futures.promise]/23, /26, [futures.task.members],
// [thread.condition.nonmember]/2-3; the checks are in at_thread_exit_program.hpp).
// FLAGS: -pthread
// REQUIRES: exceptions
#include <cstdlib>
#include "at_thread_exit_program.hpp"

[[noreturn]] void leave() {
  ate::register_actions();
  std::exit(0); // automatic objects of main and leave() are not destroyed
}

int main() { leave(); }
