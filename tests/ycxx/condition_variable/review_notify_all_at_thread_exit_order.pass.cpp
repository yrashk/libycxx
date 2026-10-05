// [thread.condition.nonmember]/2: the notification scheduled by notify_all_at_thread_exit "is
// equivalent to: cond.notify_all(); lk.unlock();" -- the condition variable is notified while
// the lock is still held. So a waiter that acquires the lock after the thread's exit and sees its
// condition satisfied may destroy the condition variable at once: no access to it can follow
// the unlock. (Notifying after the unlock would touch a destroyed object; ThreadSanitizer
// reports that as a race between the destruction and notify_all.)
// FLAGS: -pthread
#include <condition_variable>
#include <mutex>
#include <new>
#include <thread>
#include "check.hpp"

int main() {
  for (int round = 0; round < 200; ++round) {
    std::mutex m;
    alignas(std::condition_variable) unsigned char storage[sizeof(std::condition_variable)];
    auto* cv = ::new (static_cast<void*>(storage)) std::condition_variable;
    bool done = false;
    std::thread t([&] {
      std::unique_lock lk(m);
      done = true;
      std::notify_all_at_thread_exit(*cv, std::move(lk));
    });
    {
      std::unique_lock lk(m);
      cv->wait(lk, [&] { return done; });
      // The lock was released by the exiting thread, after its notification: the condition
      // variable is no longer needed by anyone.
      cv->~condition_variable();
      for (unsigned char& b : storage)
        b = 0xA5;
    }
    t.join();
    // Nothing wrote to the storage after the destruction.
    for (unsigned char b : storage)
      CHECK(b == 0xA5);
  }
  return 0;
}
