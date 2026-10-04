// [thread.barrier.class]/12-13: arrive(update) "Constructs an object of type arrival_token that
// is associated with the phase synchronization point for the current phase. Then, decrements
// the expected count by update." /20: wait(arrival) blocks until that phase completes ("If
// arrival is associated with the synchronization point for a previous phase, the call returns
// immediately"). /25: arrive_and_drop() "Decrements the initial expected count for all
// subsequent phases by one. Then decrements the expected count for the current phase by one."
// /6: the default completion function has no effects.
// FLAGS: -pthread
#include <barrier>
#include <thread>
#include <atomic>
#include "check.hpp"

int main() {
  // a single participant: arrive completes the phase; wait returns
  std::barrier<> solo(1);
  auto tok = solo.arrive();
  solo.wait(std::move(tok));
  solo.arrive_and_wait();

  // arrive with update > 1
  std::atomic<int> phase_count(0);
  auto count = [&]() noexcept { phase_count.fetch_add(1); };
  std::barrier b(3, count);
  std::thread t([&] { b.arrive_and_wait(); });
  auto token = b.arrive(2);
  b.wait(std::move(token));
  t.join();
  CHECK(phase_count.load() == 1);

  // token from a completed phase: returns immediately
  auto t2 = b.arrive(3);  // completes phase 2 at once
  b.wait(std::move(t2));
  CHECK(phase_count.load() == 2);

  // arrive_and_drop: 3 participants, one drops out; later phases need only 2
  std::barrier d(3, count);
  std::thread dropper([&] { d.arrive_and_drop(); });
  std::thread other([&] {
    d.arrive_and_wait();  // phase 1: 3 arrivals (drop counts)
    d.arrive_and_wait();  // phase 2: 2 participants
  });
  d.arrive_and_wait();
  d.arrive_and_wait();
  dropper.join();
  other.join();
  CHECK(phase_count.load() == 4);

  // the default completion function: phases still cycle
  std::barrier<> e(2);
  std::thread p([&] { for (int i = 0; i < 100; ++i) e.arrive_and_wait(); });
  for (int i = 0; i < 100; ++i) e.arrive_and_wait();
  p.join();
  return 0;
}
