// [saferecl.hp]: hazard_pointer() is empty (/holder.ctor/1); make_hazard_pointer() returns a
// non-empty one; move construction/assignment transfer ownership and leave the source empty
// (self-move-assignment has no effect); swap exchanges ownership without changing
// associations; protect(src) is "T* ptr = src.load(memory_order::relaxed); while
// (!try_protect(ptr, src)) {} return ptr;"; try_protect(ptr, src) reloads ptr from src and
// returns old == ptr; make_hazard_pointer_batch fills exactly the empty elements;
// clear_hazard_pointer_batch makes every element empty. Reclamation ([saferecl.hp.general]/5-6):
// a retired object is reclaimed by invoking its deleter (the one move-assigned by retire) at
// most once, and never while a hazard pointer protects it since before its retirement. Readers
// on several threads protect the current node and check it has not been reclaimed while a
// writer replaces and retires nodes.
// FLAGS: -pthread
#include <hazard_pointer>
#include <atomic>
#include <span>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

constexpr int alive = 0x5eed, dead = 0xdead;

struct Node;
struct MarkDead {
  int tag = 0;
  void operator()(Node* p) const noexcept;
};
struct Node : std::hazard_pointer_obj_base<Node, MarkDead> {
  std::atomic<int> magic{alive};
  std::atomic<int> reclaimed{0};
  std::atomic<int> deleter_tag{0};
};
void MarkDead::operator()(Node* p) const noexcept {
  p->magic.store(dead);
  p->deleter_tag.store(tag);
  p->reclaimed.fetch_add(1);
}

static_assert(std::is_nothrow_default_constructible_v<std::hazard_pointer>);
static_assert(std::is_nothrow_move_constructible_v<std::hazard_pointer> && std::is_nothrow_move_assignable_v<std::hazard_pointer>);
static_assert(!std::is_copy_constructible_v<std::hazard_pointer> && !std::is_copy_assignable_v<std::hazard_pointer>);
static_assert(noexcept(std::declval<Node&>().retire()));
static_assert(noexcept(std::declval<std::hazard_pointer&>().protect(std::declval<const std::atomic<Node*>&>())));
static_assert(!std::is_constructible_v<std::hazard_pointer_obj_base<Node, MarkDead>>);

constexpr int pool_size = 60000;
static Node pool[pool_size];

int main() {
  std::hazard_pointer e;
  CHECK(e.empty());
  std::hazard_pointer h = std::make_hazard_pointer();
  CHECK(!h.empty());
  std::hazard_pointer m(std::move(h));
  CHECK(h.empty() && !m.empty());
  m = std::move(m);
  CHECK(!m.empty());
  h = std::move(m);
  CHECK(!h.empty() && m.empty());
  swap(h, m);
  CHECK(h.empty() && !m.empty());
  m.swap(h);
  CHECK(!h.empty() && m.empty());

  // protect / try_protect
  std::atomic<Node*> src{&pool[0]};
  Node* p = h.protect(src);
  CHECK(p == &pool[0]);
  Node* q = &pool[1];
  CHECK(!h.try_protect(q, src) && q == &pool[0]);
  CHECK(h.try_protect(q, src) && q == &pool[0]);
  std::atomic<Node*> null_src{nullptr};
  CHECK(h.protect(null_src) == nullptr);
  h.reset_protection();
  h.reset_protection(nullptr);
  h.reset_protection(&pool[0]);

  // a protected node survives retirement and much reclamation of other nodes
  std::hazard_pointer guard = std::make_hazard_pointer();
  std::atomic<Node*> gsrc{&pool[2]};
  Node* g = guard.protect(gsrc);
  g->retire(MarkDead{7});
  for (int i = 3; i < 20000; ++i) pool[i].retire(MarkDead{i});
  CHECK(g->magic == alive && g->reclaimed == 0);
  int reclaimed = 0;
  for (int i = 3; i < 20000; ++i) {
    CHECK(pool[i].reclaimed <= 1);
    if (pool[i].reclaimed) {
      ++reclaimed;
      CHECK(pool[i].deleter_tag == i);  // its own deleter
    }
  }
  guard.reset_protection();  // ends the epoch; g may now be reclaimed later
  (void)reclaimed;

  // batches
  std::hazard_pointer batch[4];
  batch[1] = std::make_hazard_pointer();
  std::atomic<Node*> bsrc{&pool[20000]};
  Node* bp = batch[1].protect(bsrc);
  std::make_hazard_pointer_batch(std::span<std::hazard_pointer>(batch));
  for (auto& b : batch) CHECK(!b.empty());
  // batch[1] kept its hazard pointer and association: the node is still protected
  bp->retire(MarkDead{1});
  for (int i = 20001; i < 30000; ++i) pool[i].retire(MarkDead{i});
  CHECK(bp->reclaimed == 0);
  std::clear_hazard_pointer_batch(std::span<std::hazard_pointer>(batch));
  for (auto& b : batch) CHECK(b.empty());

  // concurrent readers and a retiring writer
  std::atomic<Node*> current{&pool[30000]};
  std::atomic<bool> stop{false};
  std::atomic<long> bad{0}, reads{0};
  std::vector<std::thread> readers;
  for (int t = 0; t < 4; ++t) {
    readers.emplace_back([&] {
      std::hazard_pointer hp = std::make_hazard_pointer();
      while (!stop.load()) {
        Node* n = hp.protect(current);
        for (int spin = 0; spin < 50; ++spin)
          if (n->magic.load() != alive) ++bad;
        hp.reset_protection();
        ++reads;
      }
    });
  }
  for (int i = 30001; i < pool_size; ++i) {
    Node* old = current.exchange(&pool[i], std::memory_order_acq_rel);
    old->retire(MarkDead{i});
  }
  stop = true;
  for (auto& th : readers) th.join();
  CHECK(bad == 0 && reads > 0);
  for (int i = 0; i < pool_size; ++i) CHECK(pool[i].reclaimed <= 1);
  return 0;
}
