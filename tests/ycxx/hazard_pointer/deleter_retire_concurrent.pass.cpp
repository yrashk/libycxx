// Deleters that retire other objects, on several threads that protect and retire concurrently.
//   [saferecl.hp.base]/7 retire: "Move-assigns d to deleter, thereby setting it as the deleter of
//     x, then retires x. May reclaim possibly-reclaimable objects." [saferecl.hp.general]/5:
//     reclaimed "by invoking its deleter with a pointer to x". A deleter may run on any thread
//     that retires, and may itself retire objects (chains, trees); nothing in [saferecl.hp]
//     restricts that when other threads protect and retire at the same time.
//   [saferecl.hp.general]/4-6: every object is retired once (here, most of them by a deleter),
//     reclaimed at most once (6.1) and with its own deleter (the stateful deleter names the
//     thread whose pool the object comes from). /7: the number of possibly-reclaimable objects
//     is bounded, so retiring further objects eventually reclaims every one.
//   [saferecl.hp.general]/6.3.2 (Note 1: "a store to src sequenced before retiring x"): each
//     thread publishes a deep node X of its next structure in an atomic slot; it later replaces
//     X (the modification B) and then retires the structure's root, whose deleters retire X's
//     ancestors and X: B happens before X is retired. A reader whose try_protect load of the slot
//     read X (so a value modification-ordered before B) protects X until it ends that epoch.
//   [saferecl.hp.general]/6.3.1: a node a thread protects before retiring its structure's root
//     (so before the node is retired, by a deleter) survives until that thread ends the epoch.
// FLAGS: -pthread
#include <hazard_pointer>
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

constexpr int alive = 0x5eed, dead = 0xdead;
constexpr int Threads = 4;
constexpr std::uint32_t PerThread = 250'000;
constexpr std::uint32_t Structure = 63;     // nodes in each published structure
constexpr std::uint32_t LongChain = 60'000; // per thread, retired in the middle of its run
constexpr std::uint32_t Iterations = (PerThread - LongChain) / Structure;

struct Node;
struct NodeDeleter {
  int owner = -1;
  void operator()(Node* p) const noexcept;
};
struct Node : std::hazard_pointer_obj_base<Node, NodeDeleter> {
  std::atomic<int> magic{alive};
  std::atomic<int> reclaimed{0};
  int owner = -1;
  Node* child[2] = {nullptr, nullptr};
};
static Node pool[Threads][PerThread];
static std::atomic<long> reclaimed_total{0};
static std::atomic<int> errors{0};
void NodeDeleter::operator()(Node* p) const noexcept {
  if (owner != p->owner) errors.fetch_add(1);
  if (p->magic.exchange(dead) != alive) errors.fetch_add(1);
  if (p->reclaimed.fetch_add(1) != 0) errors.fetch_add(1);
  for (Node* c : p->child)
    if (c) c->retire(NodeDeleter{c->owner});
  reclaimed_total.fetch_add(1);
}

struct Filler;
struct FillerDeleter {
  void operator()(Filler* p) const noexcept;
};
struct Filler : std::hazard_pointer_obj_base<Filler, FillerDeleter> {};
void FillerDeleter::operator()(Filler* p) const noexcept { delete p; }

static std::atomic<Node*> slot[Threads];
static std::atomic<long> reads{0};
static std::atomic<int> started{0};

// Links Structure nodes from `base`: a binary tree in heap order (even k) or a chain (odd k).
// Returns the root; *deep is the deepest node (the last one).
static Node* build(Node* base, int owner, std::uint32_t k, Node** deep) {
  for (std::uint32_t i = 0; i < Structure; ++i) base[i].owner = owner;
  for (std::uint32_t i = 0; i < Structure; ++i) {
    if (k % 2 == 0) {
      if (2 * i + 2 < Structure) {
        base[i].child[0] = &base[2 * i + 1];
        base[i].child[1] = &base[2 * i + 2];
      }
    } else if (i + 1 < Structure) {
      base[i].child[i % 2] = &base[i + 1];
    }
  }
  *deep = &base[Structure - 1];
  return &base[0];
}

static void read_slots(std::hazard_pointer& hp) {
  for (int j = 0; j < Threads; ++j) {
    Node* x = hp.protect(slot[j]);
    if (!x) continue;
    for (int spin = 0; spin < 8; ++spin)
      if (x->magic.load() != alive || x->reclaimed.load() != 0) errors.fetch_add(1);
    hp.reset_protection();
    reads.fetch_add(1);
  }
}

static void worker(int t) {
  Node* mine = pool[t];
  std::hazard_pointer reader = std::make_hazard_pointer();
  std::hazard_pointer local = std::make_hazard_pointer();
  std::atomic<Node*> local_src{nullptr};
  // The long chain, linked through child[0]; retired in the middle of the run.
  Node* chain = mine + (PerThread - LongChain);
  for (std::uint32_t i = 0; i < LongChain; ++i) {
    chain[i].owner = t;
    if (i + 1 < LongChain) chain[i].child[0] = &chain[i + 1];
  }
  Node* chain_mid = &chain[LongChain / 2];
  started.fetch_add(1);
  while (started.load() < Threads) std::this_thread::yield();  // all threads run together
  Node* prev_root = nullptr;
  for (std::uint32_t k = 0; k < Iterations; ++k) {
    Node* deep;
    Node* root = build(mine + k * Structure, t, k, &deep);
    // B: replaces the previous structure's deep node, before that structure's root is retired.
    slot[t].exchange(deep);
    if (prev_root) {
      // Protect a node of the previous structure, then retire its root.
      local_src.store(prev_root + Structure / 2);
      Node* m = local.protect(local_src);
      prev_root->retire(NodeDeleter{t});
      read_slots(reader);
      if (m->magic.load() != alive || m->reclaimed.load() != 0) errors.fetch_add(1);
      local.reset_protection();
    }
    if (k == Iterations / 2) {
      local_src.store(chain_mid);
      Node* m = local.protect(local_src);
      chain[0].retire(NodeDeleter{t});
      for (int r = 0; r < 200; ++r) {
        (new Filler)->retire();
        read_slots(reader);
      }
      if (m->magic.load() != alive || m->reclaimed.load() != 0) errors.fetch_add(1);
      local.reset_protection();
    }
    prev_root = root;
  }
  slot[t].store(nullptr);
  prev_root->retire(NodeDeleter{t});
}

int main() {
  watchdog(170);
  std::vector<std::thread> threads;
  for (int t = 0; t < Threads; ++t) threads.emplace_back(worker, t);
  for (auto& th : threads) th.join();
  CHECK(errors.load() == 0);
  CHECK(reads.load() > 0);
  constexpr long Used = long(Threads) * (Iterations * Structure + LongChain);
  long budget = 8 * Used;
  while (reclaimed_total.load() < Used && budget-- > 0) (new Filler)->retire();
  CHECK(reclaimed_total.load() == Used);
  for (int t = 0; t < Threads; ++t)
    for (std::uint32_t i = 0; i < PerThread; ++i) {
      const bool used = i < Iterations * Structure || i >= PerThread - LongChain;
      CHECK(pool[t][i].reclaimed.load() == (used ? 1 : 0));
    }
  for (int r = 0; r < 20000; ++r) (new Filler)->retire();  // nothing is reclaimed twice
  CHECK(reclaimed_total.load() == Used);
  CHECK(errors.load() == 0);
  return 0;
}
