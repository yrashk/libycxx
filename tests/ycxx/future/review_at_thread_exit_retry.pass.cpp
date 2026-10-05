// [futures.promise]/22-24 set_value_at_thread_exit: "Throws: future_error if its shared state
// already has a stored value or exception; any exception thrown by the constructor selected to
// copy an object of R". A copy that throws stores nothing, so a later call can still store the
// value, and the state becomes ready only when the thread exits (after its thread_local objects
// are destroyed); then exactly that value is retrieved.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <future>
#include <thread>
#include "check.hpp"

static int copies_to_fail = 1;
struct Value {
  int v;
  explicit Value(int x) : v(x) {}
  Value(const Value& o) : v(o.v) {
    if (copies_to_fail > 0) {
      --copies_to_fail;
      throw 7;
    }
  }
};

static std::future<Value>* observed;
struct Watch {
  ~Watch() {
    // The thread's thread_local objects are destroyed before the state becomes ready.
    CHECK(observed->wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
  }
};

int main() {
  for (int round = 0; round < 50; ++round) {
    copies_to_fail = 1;
    std::promise<Value> p;
    std::future<Value> f = p.get_future();
    observed = &f;
    std::thread t([&p] {
      thread_local Watch w;
      (void)w;
      const Value x(42);
      bool threw = false;
      try {
        p.set_value_at_thread_exit(x);
      } catch (int) {
        threw = true;
      }
      CHECK(threw);
      p.set_value_at_thread_exit(x);  // nothing was stored by the failed call
      bool second = false;
      try {
        p.set_value_at_thread_exit(x);
      } catch (const std::future_error& e) {
        second = e.code() == std::future_errc::promise_already_satisfied;
      }
      CHECK(second);
    });
    t.join();
    CHECK(f.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
    CHECK(f.get().v == 42);
  }
  return 0;
}
