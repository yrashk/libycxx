// [thread.condition.nonmember]: notify_all_at_thread_exit(cond, lk): "Transfers ownership of
// the lock associated with lk into internal storage and schedules cond to be notified when the
// current thread exits, after all objects with thread storage duration associated with the
// current thread have been destroyed. This notification is equivalent to: lk.unlock();
// cond.notify_all();"
// FLAGS: -pthread
#include <condition_variable>
#include <mutex>
#include <thread>
#include "check.hpp"

static std::mutex m;
static std::condition_variable cv;
static bool ready = false;
static int tls_destroyed = 0;

struct TlsProbe {
  ~TlsProbe() { tls_destroyed = 1; }
};
thread_local TlsProbe probe;

int main() {
  std::unique_lock<std::mutex> l(m);
  std::thread t([] {
    (void)&probe;  // construct the thread_local
    std::unique_lock<std::mutex> lk(m);
    ready = true;
    std::notify_all_at_thread_exit(cv, std::move(lk));
    // m stays locked until the thread has exited
  });
  cv.wait(l, [] { return ready; });
  CHECK(tls_destroyed == 1);  // notified only after thread_local destruction
  l.unlock();
  t.join();
  return 0;
}
