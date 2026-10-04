// [basic.stc.thread]: a variable with thread storage duration has a distinct object per
// thread, and [basic.start.term]/1: "The destructors for all initialized objects with thread
// storage duration within that thread strongly happen before the destructors of any object
// with static storage duration are initiated"; they are destroyed as the thread exits, in
// reverse order of construction completion. [stmt.dcl]/3-4: a block thread_local is
// initialized the first time control passes through its declaration in that thread, and
// destroyed iff it was constructed.
// FLAGS: -pthread
#include <pthread.h>
#include "check.hpp"

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static int ctors = 0, dtors = 0;
static int order_log[64];
static int nlog = 0;

struct PerThread {
  int id;
  explicit PerThread(int i) : id(i) {
    pthread_mutex_lock(&mu);
    ++ctors;
    pthread_mutex_unlock(&mu);
  }
  ~PerThread() {
    pthread_mutex_lock(&mu);
    ++dtors;
    order_log[nlog++] = id;
    pthread_mutex_unlock(&mu);
  }
};

static PerThread& first(int base) {
  thread_local PerThread a(base + 1);
  return a;
}
static PerThread& second(int base) {
  thread_local PerThread b(base + 2);
  return b;
}
[[maybe_unused]] static PerThread& unused() {
  thread_local PerThread c(999);
  return c;
}

static void* worker(void* arg) {
  int base = *static_cast<int*>(arg);
  PerThread& a = first(base);
  PerThread& b = second(base);
  CHECK(&first(base) == &a);  // initialized once per thread
  CHECK(a.id == base + 1 && b.id == base + 2);
  return nullptr;
}

int main() {
  int arg1 = 10, arg2 = 20;
  pthread_t t1, t2;
  CHECK(pthread_create(&t1, nullptr, worker, &arg1) == 0);
  CHECK(pthread_join(t1, nullptr) == 0);
  // the thread's objects were destroyed by the time join returns, in reverse order
  CHECK(ctors == 2 && dtors == 2);
  CHECK(nlog == 2 && order_log[0] == 12 && order_log[1] == 11);

  CHECK(pthread_create(&t2, nullptr, worker, &arg2) == 0);
  CHECK(pthread_join(t2, nullptr) == 0);
  CHECK(ctors == 4 && dtors == 4);
  CHECK(order_log[2] == 22 && order_log[3] == 21);

  // the main thread has its own instance, distinct from the other threads'
  PerThread& mine = first(0);
  CHECK(mine.id == 1);
  CHECK(ctors == 5 && dtors == 4);
  return 0;
}
