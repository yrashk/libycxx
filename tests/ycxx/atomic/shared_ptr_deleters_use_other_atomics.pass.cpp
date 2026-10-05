// atomic<shared_ptr<T>> operations that release the last reference to an object whose deleter
// uses other atomic<shared_ptr<T>> objects (load, store, exchange, compare_exchange).
//   [util.smartptr.atomic.shared]/4 store: "Atomically replaces the value pointed to by this with
//     the value of desired as if by p.swap(desired)" (the old value ends up in the parameter
//     desired and is destroyed with it); /11 exchange; /14 compare_exchange: "If p is equivalent
//     to expected, assigns desired to p ..., otherwise assigns p to expected" (the old value of
//     expected is released); /5-6 operator=. Each object is a distinct atomic object, so using
//     another one from the deleter is an ordinary call on an object no operation is active on;
//     nothing in the draft lets such a call block. An implementation that runs the deleter while
//     holding an internal lock that another object's operations also take (for instance a lock
//     from a table shared by many objects) deadlocks; the watchdog fails the test then.
// 600 objects (more than any small table of locks); every object's first value is a node whose
// deleter loads two other objects, stores into a third and makes a failing and a succeeding
// compare_exchange on a fourth.
// FLAGS: -latomic -pthread
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <memory>
#include "check.hpp"
#include "watchdog.hpp"

constexpr int K = 600;

struct Node {
  int value;
};

static std::atomic<std::shared_ptr<Node>>* objs = nullptr;
static int deleter_runs = 0;
static int deleter_bad = 0;

struct UsesOthers {
  int k;
  void operator()(Node* n) const {
    ++deleter_runs;
    (void)objs[(k + 1) % K].load();
    (void)objs[(k + 2) % K].load();
    // Only forwards (no wrap-around): a chain of deleters releasing each other never comes back
    // to an object whose operation is still under way.
    if (k + 3 < K) objs[k + 3].store(std::make_shared<Node>(Node{-1}));
    std::shared_ptr<Node> wrong = std::make_shared<Node>(Node{-2});
    std::shared_ptr<Node> exp = wrong;
    const int c = (k + 4) % K;
    if (objs[c].compare_exchange_strong(exp, std::make_shared<Node>(Node{-3}))) ++deleter_bad;
    if (exp == wrong) ++deleter_bad;  // a failed compare_exchange stores the current value in expected
    if (!objs[c].compare_exchange_strong(exp, exp)) ++deleter_bad;
    delete n;
  }
};

static std::shared_ptr<Node> node_using_others(int k) { return std::shared_ptr<Node>(new Node{k}, UsesOthers{k}); }

int main() {
  watchdog(5);
  objs = new std::atomic<std::shared_ptr<Node>>[K];
  auto refill = [] {
    for (int k = 0; k < K; ++k) objs[k].store(node_using_others(k));
  };
  // (refill releases the previous values too; a released node's deleter stores only into a
  // later object, which refill then overwrites: every object ends up holding such a node.)

  refill();
  deleter_runs = 0;
  for (int k = 0; k < K; ++k) objs[k].store(std::make_shared<Node>(Node{k}));  // store releases
  CHECK(deleter_runs > 0);

  refill();
  deleter_runs = 0;
  for (int k = 0; k < K; ++k) objs[k] = nullptr;  // operator=(nullptr_t)
  CHECK(deleter_runs > 0);

  refill();
  deleter_runs = 0;
  for (int k = 0; k < K; ++k) (void)objs[k].exchange(std::make_shared<Node>(Node{k}));
  CHECK(deleter_runs > 0);

  refill();
  deleter_runs = 0;
  for (int k = 0; k < K; ++k) {
    // A failing compare_exchange whose expected holds the last reference to a node whose
    // deleter uses other objects: assigning p to expected releases it.
    std::shared_ptr<Node> exp = std::shared_ptr<Node>(new Node{-k}, UsesOthers{k});
    (void)objs[k].compare_exchange_strong(exp, std::make_shared<Node>(Node{0}));
  }
  CHECK(deleter_runs >= K);  // (every expected's node)
  CHECK(deleter_bad == 0);

  for (int k = 0; k < K; ++k) objs[k].store(nullptr);
  delete[] objs;
  return 0;
}
