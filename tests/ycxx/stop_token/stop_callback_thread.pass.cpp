// [stoptoken.concepts]/12: callbacks registered with the stop state are executed synchronously
// by the thread making the stop request; /11: "A call to request_stop that returns true
// synchronizes with a call to stop_requested on an associated stoppable_token ... that returns
// true." /3.3.3: destroying a stop_callback while its callback is executing on another thread
// blocks until it returns. Concurrent request_stop: exactly one returns true.
// FLAGS: -pthread
#include <stop_token>
#include <thread>
#include <atomic>
#include <vector>
#include "check.hpp"

int main() {
  // callback runs on the requesting thread
  std::stop_source src;
  std::thread::id ran_on;
  std::stop_callback cb(src.get_token(), [&] { ran_on = std::this_thread::get_id(); });
  std::thread::id requester;
  std::thread t([&] { requester = std::this_thread::get_id(); src.request_stop(); });
  t.join();
  CHECK(ran_on == requester);

  // exactly one concurrent request_stop returns true; the callback runs exactly once
  for (int round = 0; round < 20; ++round) {
    std::stop_source s;
    std::atomic<int> calls(0), wins(0);
    std::stop_callback c(s.get_token(), [&] { calls.fetch_add(1); });
    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i)
      ts.emplace_back([&] { if (s.request_stop()) wins.fetch_add(1); });
    for (auto& th : ts) th.join();
    CHECK(wins.load() == 1);
    CHECK(calls.load() == 1);
  }

  // destruction waits for a running callback on another thread
  std::stop_source s5;
  std::atomic<int> phase(0);
  std::atomic<bool> finished(false);
  auto* c5 = new std::stop_callback(s5.get_token(), [&] {
    phase = 1;
    phase.notify_one();
    for (int i = 0; i < 2000; ++i) std::this_thread::yield();
    finished = true;
  });
  std::thread stopper([&] { s5.request_stop(); });
  phase.wait(0);
  delete c5;  // blocks until the callback returns
  CHECK(finished.load());
  stopper.join();
  return 0;
}
