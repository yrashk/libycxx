// [exec.run.loop.general]/1-3: a run_loop maintains a thread-safe first-in-first-out queue;
// concurrent invocations of its members other than run and its destructor (here: starting
// schedule operations, which push-back, and finish) do not introduce data races, and push-back,
// pop-front and finish execute atomically. [exec.run.loop.members]/3: push-back synchronizes with
// the pop-front that obtains the item; /6: run executes the items on the calling thread until
// finish() was called and the queue is empty; /10: finish synchronizes with the pop-front that
// returns nullptr.
// So, with producers on other threads and run() on this one: every operation completes exactly
// once, on the running thread; each producer's operations complete in the order it started
// them; data a producer wrote before starting an operation is visible when it completes.
// Run under ThreadSanitizer as well.
// FLAGS: -pthread
#include <execution>
#include <memory>
#include <optional>
#include <utility>
#include <thread>
#include <vector>
#include "check.hpp"

namespace ex = std::execution;

constexpr int producers = 4;
constexpr int per_producer = 500;

struct slot {
  int payload = 0;               // written by the producer before start
  int seen = -1;                 // read back by the receiver
  int order = -1;                // completion order within its producer
  std::thread::id ran_on;
};

struct rcvr {
  using receiver_concept = ex::receiver_tag;
  slot* s;
  int* counter; // per producer, touched only on the running thread
  void set_value() && noexcept {
    s->seen = s->payload;
    s->order = (*counter)++;
    s->ran_on = std::this_thread::get_id();
  }
  void set_stopped() && noexcept {}
};

using sched_t = decltype(std::declval<ex::run_loop&>().get_scheduler());
using op_t = decltype(ex::connect(ex::schedule(std::declval<sched_t>()), std::declval<rcvr>()));

// Operation states cannot move: built in place through a conversion (guaranteed elision).
struct make_op {
  sched_t sch;
  rcvr r;
  operator op_t() const { return ex::connect(ex::schedule(sch), r); }
};

int main() {
  ex::run_loop loop;
  auto sch = loop.get_scheduler();
  std::vector<std::vector<slot>> slots(producers, std::vector<slot>(per_producer));
  std::vector<int> counters(producers, 0);
  // operation states must stay put: one array per producer
  std::vector<std::unique_ptr<std::optional<op_t>[]>> ops;
  for (int p = 0; p < producers; ++p)
    ops.emplace_back(new std::optional<op_t>[per_producer]);

  std::vector<std::thread> threads;
  for (int p = 0; p < producers; ++p)
    threads.emplace_back([&, p] {
      for (int i = 0; i < per_producer; ++i) {
        slots[p][i].payload = p * 100000 + i;
        auto& op = ops[p][i].emplace(make_op{sch, rcvr{&slots[p][i], &counters[p]}});
        ex::start(op);
      }
    });
  std::thread finisher([&] {
    for (auto& t : threads)
      t.join();
    loop.finish();
  });
  loop.run();
  finisher.join();

  const auto me = std::this_thread::get_id();
  for (int p = 0; p < producers; ++p) {
    CHECK(counters[p] == per_producer);
    for (int i = 0; i < per_producer; ++i) {
      CHECK(slots[p][i].seen == p * 100000 + i);
      CHECK(slots[p][i].order == i); // FIFO per producer
      CHECK(slots[p][i].ran_on == me);
    }
  }
}
