// [stmt.dcl]/3: concurrent executions entering the declaration "shall wait for completion of
// the initialization"; if the initialization exits by an exception it is not complete "so it
// will be tried again the next time control enters the declaration". One thread's failed
// attempt must therefore let a waiting thread run the initializer itself.
// FLAGS: -pthread
#include <pthread.h>
#include <sched.h>
#include "check.hpp"

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static int attempts = 0;
static volatile int go = 0;

static int init_value() {
  pthread_mutex_lock(&mu);
  int n = ++attempts;
  pthread_mutex_unlock(&mu);
  for (int i = 0; i < 1000; ++i) sched_yield();
  if (n == 1) throw n;  // the first attempt fails
  return 100 + n;
}

static int get() {
  static int v = init_value();
  return v;
}

struct Result {
  int value = 0;
  bool threw = false;
};

static void* worker(void* p) {
  Result* r = static_cast<Result*>(p);
  while (!go) sched_yield();
  try {
    r->value = get();
  } catch (int) {
    r->threw = true;
  }
  return nullptr;
}

int main() {
  constexpr int N = 6;
  pthread_t th[N];
  Result res[N];
  for (int i = 0; i < N; ++i) CHECK(pthread_create(&th[i], nullptr, worker, &res[i]) == 0);
  go = 1;
  for (int i = 0; i < N; ++i) CHECK(pthread_join(th[i], nullptr) == 0);
  int threw = 0;
  for (int i = 0; i < N; ++i) {
    if (res[i].threw)
      ++threw;
    else
      CHECK(res[i].value == 102);
  }
  CHECK(threw == 1);
  CHECK(attempts == 2);
  CHECK(get() == 102);
  return 0;
}
