// [saferecl.rcu]: rcu_default_domain() "Returns: A reference to a static-duration object of
// type rcu_domain. A reference to the same object is returned every time"; rcu_domain meets
// Cpp17Lockable, try_lock is "Equivalent to lock()" and returns true, lock/unlock nest
// ([saferecl.rcu.general]/3); rcu_obj_base<T, D>::retire(d, dom) "Evaluates deleter =
// std::move(d) and schedules the evaluation of the expression deleter(addressof(x))";
// rcu_retire(p, d, dom) schedules d1(p) with d1 initialised from std::move(d); /6 "Each
// scheduled evaluation is evaluated at most once"; rcu_barrier: "For any evaluation that
// happens before the call to rcu_barrier and that schedules an evaluation E in dom, blocks
// until E has been evaluated"; rcu_synchronize blocks until every region of RCU protection
// that began before it has ended; /5: a region that started before an object was retired ends
// before the object's deleter runs. Readers on several threads check that the node they reached
// inside a region has not been reclaimed. [saferecl.rcu.base]/6: rcu_obj_base<T, D> is
// trivially copyable when D is.
// FLAGS: -pthread
#include <rcu>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>
#include "check.hpp"

constexpr int alive = 0x5eed, dead = 0xdead;

struct Node;
struct MarkDead {  // reclaims a pool node by marking it (the storage is static)
  int* calls = nullptr;
  void operator()(Node* p) const noexcept;
};
struct Node : std::rcu_obj_base<Node, MarkDead> {
  std::atomic<int> magic{alive};
  std::atomic<int> reclaimed{0};
  int value = 0;
};
void MarkDead::operator()(Node* p) const noexcept {
  p->magic.store(dead);
  p->reclaimed.fetch_add(1);
  if (calls) ++*calls;
}

struct Plain : std::rcu_obj_base<Plain> {
  static inline std::atomic<int> destroyed{0};
  ~Plain() { ++destroyed; }
};

static_assert(noexcept(std::rcu_default_domain()));
static_assert(noexcept(std::rcu_default_domain().lock()) && noexcept(std::rcu_default_domain().unlock()));
static_assert(noexcept(std::rcu_default_domain().try_lock()));
static_assert(noexcept(std::rcu_synchronize()) && noexcept(std::rcu_barrier()));
static_assert(noexcept(std::declval<Node&>().retire()));
static_assert(!std::is_copy_constructible_v<std::rcu_domain> && !std::is_copy_assignable_v<std::rcu_domain>);
static_assert(std::is_trivially_copyable_v<std::rcu_obj_base<Plain>>);
static_assert(!std::is_constructible_v<std::rcu_obj_base<Plain>>);  // protected constructor

static Node pool[20000];

int main() {
  std::rcu_domain& dom = std::rcu_default_domain();
  CHECK(&dom == &std::rcu_default_domain());
  CHECK(dom.try_lock());
  dom.lock();
  dom.unlock();
  dom.unlock();
  {
    std::scoped_lock<std::rcu_domain> lk(dom);
    std::scoped_lock<std::rcu_domain> lk2(dom);
  }

  // retire with the default deleter, through the base and through rcu_retire
  Plain* a = new Plain;
  a->retire();
  std::rcu_retire(new Plain);
  int custom_calls = 0;
  int* raw = new int(5);
  std::rcu_retire(raw, [&custom_calls](int* p) {
    ++custom_calls;
    delete p;
  });
  std::rcu_barrier();
  CHECK(Plain::destroyed == 2);
  CHECK(custom_calls == 1);

  // the deleter given to retire is the one used, moved into the object
  int calls = 0;
  pool[0].retire(MarkDead{&calls});
  pool[1].retire(MarkDead{&calls}, dom);
  std::rcu_barrier(dom);
  CHECK(calls == 2 && pool[0].magic == dead && pool[1].magic == dead);
  CHECK(pool[0].reclaimed == 1 && pool[1].reclaimed == 1);

  // readers inside regions never see a reclaimed node; the writer replaces and retires
  std::atomic<Node*> current{&pool[2]};
  std::atomic<bool> stop{false};
  std::atomic<long> bad{0}, reads{0};
  std::vector<std::thread> readers;
  for (int t = 0; t < 4; ++t) {
    readers.emplace_back([&] {
      while (!stop.load()) {
        std::scoped_lock<std::rcu_domain> lk(std::rcu_default_domain());
        Node* n = current.load(std::memory_order_acquire);
        for (int spin = 0; spin < 50; ++spin)
          if (n->magic.load() != alive) ++bad;
        ++reads;
      }
    });
  }
  int next = 3;
  for (; next < 10000; ++next) {
    pool[next].value = next;
    Node* old = current.exchange(&pool[next], std::memory_order_acq_rel);
    if (next % 2) old->retire(MarkDead{});
    else std::rcu_retire(old, MarkDead{});
  }
  // rcu_synchronize: after it returns no reader still uses the replaced node, so it may be
  // reclaimed by hand
  for (; next < 10400; ++next) {
    Node* old = current.exchange(&pool[next], std::memory_order_acq_rel);
    std::rcu_synchronize();
    old->magic.store(dead);
  }
  stop = true;
  for (auto& th : readers) th.join();
  std::rcu_barrier();
  CHECK(bad == 0);
  CHECK(reads > 0);
  for (int i = 2; i < 10000 - 1; ++i) CHECK(pool[i].reclaimed == 1);  // every retired node, once
  for (int i = 10000; i < next; ++i) CHECK(pool[i].reclaimed == 0);
  return 0;
}
