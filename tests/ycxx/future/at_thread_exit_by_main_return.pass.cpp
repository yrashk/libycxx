// The thread-exit actions of the main thread run when main returns ([basic.start.main]/5: a
// return from main calls exit; [support.start.term]/9.1), after its thread_local objects are
// destroyed and before the static objects are destroyed and the atexit functions run
// ([futures.promise]/23, /26, [futures.task.members], [thread.condition.nonmember]/2-3; the
// checks are in at_thread_exit_program.hpp).
// FLAGS: -pthread
// REQUIRES: exceptions
#include "at_thread_exit_program.hpp"

int main() {
  ate::register_actions();
  return 0;
}
