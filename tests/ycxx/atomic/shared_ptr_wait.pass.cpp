// [util.smartptr.atomic.shared]/21: wait(old): "Repeatedly performs the following steps, in
// order: Evaluates load(order) and compares it to old. If the two are not equivalent, returns.
// Blocks until it is unblocked by an atomic notifying operation or is unblocked spuriously."
// /22: "Two shared_ptr objects are equivalent if they store the same pointer and either share
// ownership or are both empty." So wait returns at once for the same pointer with different
// ownership, and for an empty versus a non-empty null pointer. /23, /25: notify_one / notify_all.
// The same for atomic<weak_ptr<T>> ([util.smartptr.atomic.weak]).
// FLAGS: -pthread
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

int main() {
  watchdog(20);
  auto s = std::make_shared<int>(1);
  std::atomic<std::shared_ptr<int>> a(s);
  a.wait(nullptr);  // different pointer: returns
  a.wait(std::make_shared<int>(1), std::memory_order::acquire);
  auto other = std::make_shared<long>(0);
  a.wait(std::shared_ptr<int>(other, s.get()));  // same pointer, other owner: not equivalent
  a.wait(std::shared_ptr<int>(), std::memory_order::relaxed);

  std::atomic<std::shared_ptr<int>> z;  // empty
  z.wait(std::shared_ptr<int>(other, static_cast<int*>(nullptr)));  // non-empty null: not equivalent

  // a hand-off in both directions
  auto t = std::make_shared<int>(2);
  std::thread th([&] {
    a.wait(s);
    CHECK(a.load() == t);
    a.store(s);
    a.notify_one();
  });
  a.store(t);
  a.notify_one();
  a.wait(t);
  CHECK(a.load() == s);
  th.join();

  // notify_all wakes every waiter
  std::atomic<int> woke(0);
  std::vector<std::thread> ws;
  for (int i = 0; i < 3; ++i)
    ws.emplace_back([&] {
      a.wait(s);
      woke.fetch_add(1);
    });
  a.store(nullptr);
  a.notify_all();
  for (auto& w : ws) w.join();
  CHECK(woke.load() == 3);

  // atomic<weak_ptr>
  std::weak_ptr<int> ws_(s), wt(t);
  std::atomic<std::weak_ptr<int>> aw(ws_);
  aw.wait(wt);
  aw.wait(std::weak_ptr<int>());
  std::thread th2([&] {
    aw.wait(ws_);
    CHECK(aw.load().lock() == t);
  });
  aw.store(wt);
  aw.notify_all();
  th2.join();
  return 0;
}
