// [stmt.dcl]/3, Note 2: "A conforming implementation cannot introduce any deadlock around
// execution of the initializer." The initializer of one block static may initialize other
// block statics (in other functions); and an initializer may run concurrently with another
// thread initializing a different static.
// FLAGS: -pthread
#include <atomic>
#include <pthread.h>
#include <sched.h>
#include "check.hpp"

static int order[4];
static int n = 0;

static int& inner() {
  static int i = (order[n++] = 1, 7);
  return i;
}
static int& outer() {
  static int o = (order[n++] = 2, inner() * 2);
  return o;
}

static std::atomic<int> a_started{0}, b_done{0};  // signals between the threads

static int init_a() {
  a_started = 1;
  while (!b_done) sched_yield();  // waits for another thread's different static
  return 1;
}
static int get_a() {
  static int a = init_a();
  return a;
}
static int get_b() {
  static int b = 2;
  static int b2 = b + 1;  // dynamic? constant: still fine
  return b2;
}
static int dyn_b_value = 5;
static int get_b_dynamic() {
  static int b = dyn_b_value + get_b();
  return b;
}

static void* thread_a(void*) {
  CHECK(get_a() == 1);
  return nullptr;
}

int main() {
  CHECK(outer() == 14);
  CHECK(n == 2 && order[0] == 2 && order[1] == 1);
  CHECK(outer() == 14 && inner() == 7 && n == 2);

  pthread_t t;
  CHECK(pthread_create(&t, nullptr, thread_a, nullptr) == 0);
  while (!a_started) sched_yield();
  CHECK(get_b_dynamic() == 8);  // must not block on a's in-progress initialization
  b_done = 1;
  CHECK(pthread_join(t, nullptr) == 0);
  CHECK(get_a() == 1);
  return 0;
}
