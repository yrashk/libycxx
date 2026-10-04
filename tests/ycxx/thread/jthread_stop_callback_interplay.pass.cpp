// [stoptoken.concepts]/12 (request_stop of a stoppable-source): "If the request was made, the
// stop state's registered callback invocations shall be synchronously executed" -- by the
// thread that calls request_stop, before it returns; it returns true only for the call that
// made the request. /3.2.1.3.2: if stop has already been requested at registration, the
// callback "shall be immediately evaluated on the thread executing scb's constructor"
// ([stopcallback.cons]/2). [thread.jthread.cons]: the jthread's
// callable receives get_stop_token(); ~jthread: "If joinable() is true, calls request_stop()
// and then join()". [thread.jthread.stop]: get_stop_source/get_stop_token share the jthread's
// stop state. All hand-offs are explicit, so no timing assumptions are made.
// FLAGS: -pthread
#include <atomic>
#include <stop_token>
#include <thread>
#include "check.hpp"
#include "watchdog.hpp"

int main() {
  watchdog(20);
  {
    std::atomic<bool> registered = false, ran = false;
    std::atomic<std::thread::id> ran_on{};
    std::jthread j([&](std::stop_token st) {
      std::stop_callback cb(st, [&] {
        ran_on = std::this_thread::get_id();
        ran = true;
      });
      registered = true;
      while (!ran.load()) std::this_thread::yield();  // keep cb alive until it has run
    });
    while (!registered.load()) std::this_thread::yield();
    CHECK(j.get_stop_token().stop_possible() && !j.get_stop_token().stop_requested());
    CHECK(j.request_stop());
    CHECK(ran.load() && ran_on.load() == std::this_thread::get_id());  // synchronously, here
    CHECK(!j.request_stop() && !j.get_stop_source().request_stop());
    j.join();
    CHECK(j.get_stop_token().stop_requested());
  }
  {
    std::stop_source ss;
    ss.request_stop();
    std::thread::id id{};
    std::stop_callback late(ss.get_token(), [&] { id = std::this_thread::get_id(); });
    CHECK(id == std::this_thread::get_id());
  }
  {
    std::atomic<bool> registered = false, ran = false;
    std::thread::id dtor_thread = std::this_thread::get_id();
    std::atomic<bool> on_dtor_thread = false;
    {
      std::jthread k([&](std::stop_token st) {
        std::stop_callback cb(st, [&] {
          on_dtor_thread = std::this_thread::get_id() == dtor_thread;
          ran = true;
        });
        registered = true;
        while (!ran.load()) std::this_thread::yield();
      });
      while (!registered.load()) std::this_thread::yield();
    }  // ~jthread: request_stop() (runs cb here), then join()
    CHECK(ran.load() && on_dtor_thread.load());
  }
  {
    std::jthread done([](std::stop_token) {});
    done.join();
    CHECK(!done.joinable() && done.request_stop());  // the stop state outlives the thread
  }
  return 0;
}
