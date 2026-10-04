// [stmt.dcl]/3: "Dynamic initialization of a block variable with static storage duration ...
// is performed the first time control passes through its declaration ... If control enters
// the declaration concurrently while the variable is being initialized, the concurrent
// execution shall wait for completion of the initialization." So the initializer runs once,
// and every thread observes the fully initialized object.
// FLAGS: -pthread
#include <pthread.h>
#include <sched.h>
#include "check.hpp"

static int init_runs = 0;  // only touched by the (single) initializer
static volatile int go = 0;

struct Slow {
  int value;
  Slow() {
    ++init_runs;
    for (int i = 0; i < 2000; ++i) sched_yield();  // widen the race window
    value = 42;
  }
};

static int get() {
  static Slow s;
  return s.value;
}

static void* worker(void* out) {
  while (!go) sched_yield();
  *static_cast<int*>(out) = get();
  return nullptr;
}

int main() {
  constexpr int N = 8;
  pthread_t th[N];
  int results[N] = {};
  for (int i = 0; i < N; ++i) CHECK(pthread_create(&th[i], nullptr, worker, &results[i]) == 0);
  go = 1;
  for (int i = 0; i < N; ++i) CHECK(pthread_join(th[i], nullptr) == 0);
  CHECK(init_runs == 1);
  for (int i = 0; i < N; ++i) CHECK(results[i] == 42);
  CHECK(get() == 42 && init_runs == 1);
  return 0;
}
