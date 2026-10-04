// [util.smartptr.atomic.general]/2: "All changes to an atomic smart pointer in
// [util.smartptr.atomic], and all associated use_count increments, are guaranteed to be
// performed atomically." /3 Example 1: the lock-free list whose push_front is
// "while (!head.compare_exchange_weak(p->next, p)) {}". [util.smartptr.atomic.shared]/17: a
// failed compare-exchange updates expected with the existing value (and its use_count).
// Concurrent pushes and pops (pop: compare_exchange_weak(p, p->next)) neither lose nor
// duplicate nodes; concurrent store/load never yield a destroyed or torn object; every object is
// destroyed exactly once.
// FLAGS: -pthread
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include "check.hpp"

static std::atomic<int> live(0);

struct Node {
  int value;
  std::shared_ptr<Node> next;
  explicit Node(int v) : value(v) { live.fetch_add(1); }
  ~Node() { live.fetch_sub(1); }
};

struct Payload {
  static constexpr unsigned magic = 0x5eedf00d;
  unsigned m = magic;
  int a, b;
  Payload(int x) : a(x), b(-x) { live.fetch_add(1); }
  ~Payload() { m = 0; live.fetch_sub(1); }
};

int main() {
  constexpr int threads = 4, per = 500;
  {
    std::atomic<std::shared_ptr<Node>> head;
    std::vector<std::thread> ts;
    std::vector<std::vector<int>> popped(threads);
    for (int t = 0; t < threads; ++t)
      ts.emplace_back([&, t] {
        for (int i = 0; i < per; ++i) {
          auto p = std::make_shared<Node>(t * per + i);
          p->next = head;  // load()
          while (!head.compare_exchange_weak(p->next, p)) {}
          if (i % 2) {  // pop one
            auto q = head.load();
            while (q && !head.compare_exchange_weak(q, q->next)) {}
            if (q) popped[t].push_back(q->value);
          }
        }
      });
    for (auto& th : ts) th.join();
    std::vector<int> seen(threads * per, 0);
    for (auto& v : popped)
      for (int x : v) ++seen[x];
    int remaining = 0;
    for (auto p = head.load(); p; p = p->next) { ++seen[p->value]; ++remaining; }
    for (int c : seen) CHECK(c == 1);  // every node exactly once: popped or still in the list
    CHECK(remaining == threads * per / 2);
    // dismantle iteratively (avoid a deep recursive destruction of the chain)
    for (auto p = head.exchange(nullptr); p;) p = std::move(p->next);
  }
  CHECK(live.load() == 0);

  {
    std::atomic<std::shared_ptr<Payload>> a(std::make_shared<Payload>(0));
    std::atomic<bool> stop(false);
    std::vector<std::thread> ts;
    for (int t = 0; t < 2; ++t)
      ts.emplace_back([&, t] {
        for (int i = 1; i <= 2000; ++i) {
          if (t == 0) a.store(std::make_shared<Payload>(i));
          else a.exchange(std::make_shared<Payload>(-i));
        }
      });
    std::vector<std::thread> rs;
    for (int r = 0; r < 2; ++r)
      rs.emplace_back([&] {
        while (!stop.load()) {
          auto p = a.load();
          CHECK(p && p->m == Payload::magic && p->a == -p->b);
          std::shared_ptr<Payload> e = p;
          if (a.compare_exchange_strong(e, p)) CHECK(e == p);  // a no-op exchange
          else CHECK(e && e->m == Payload::magic);
        }
      });
    for (auto& th : ts) th.join();
    stop.store(true);
    for (auto& th : rs) th.join();
    CHECK(live.load() == 1);
  }
  CHECK(live.load() == 0);
  return 0;
}
