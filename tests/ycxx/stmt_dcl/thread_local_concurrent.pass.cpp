// [basic.stc.thread]/1-2: "All variables declared with the thread_local keyword have thread
// storage duration. The storage for these entities lasts for the duration of the thread in
// which they are created. There is a distinct object or reference per thread". Dynamic
// initialization and destruction happen per thread; threads running concurrently each see
// their own object.
// FLAGS: -pthread
#include <pthread.h>
#include <sched.h>
#include "check.hpp"

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static int live = 0, max_live = 0, dtors = 0;
static volatile int arrived = 0;

struct Counter {
  int hits = 0;
  Counter() {
    pthread_mutex_lock(&mu);
    if (++live > max_live) max_live = live;
    pthread_mutex_unlock(&mu);
  }
  ~Counter() {
    pthread_mutex_lock(&mu);
    --live;
    ++dtors;
    pthread_mutex_unlock(&mu);
  }
};

thread_local Counter ns_counter;  // namespace-scope thread_local with dynamic init

static Counter& block_counter() {
  thread_local Counter c;
  return c;
}

constexpr int N = 4;

static void* worker(void*) {
  for (int i = 0; i < 100; ++i) ++block_counter().hits;
  ++ns_counter.hits;
  pthread_mutex_lock(&mu);
  ++arrived;
  pthread_mutex_unlock(&mu);
  while (arrived < N) sched_yield();  // all threads alive at once
  CHECK(block_counter().hits == 100);
  CHECK(ns_counter.hits == 1);
  return nullptr;
}

int main() {
  pthread_t th[N];
  for (int i = 0; i < N; ++i) CHECK(pthread_create(&th[i], nullptr, worker, nullptr) == 0);
  for (int i = 0; i < N; ++i) CHECK(pthread_join(th[i], nullptr) == 0);
  CHECK(max_live >= N);  // N concurrent block-scope objects at least
  CHECK(live == 0);
  CHECK(dtors == 2 * N);
  return 0;
}
