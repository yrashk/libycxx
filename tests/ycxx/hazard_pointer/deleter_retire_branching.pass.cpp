// Deleters that retire several objects each (trees, a deleter retiring a million objects), with
// default_delete, stateful move-only deleters and function-pointer deleters.
//   [saferecl.hp.base]/7 retire: "Move-assigns d to deleter, thereby setting it as the deleter of
//     x, then retires x. May reclaim possibly-reclaimable objects." [saferecl.hp.general]/5: a
//     retired object is reclaimed "by invoking its deleter with a pointer to x". Nothing in
//     [saferecl.hp] limits what a deleter does nor how many objects it retires, so a deleter
//     (or, with default_delete<T>, the destructor of T) may retire any number of objects, which
//     must all be reclaimed, each once, with the deleter set by its own retire
//     ([saferecl.hp.general]/4-6; 6.1: a reclaimed object is not possibly-reclaimable again).
//   [saferecl.hp.general]/7: "The number of possibly-reclaimable objects has an unspecified
//     bound", so retiring further objects eventually reclaims every unprotected retired object.
//   [saferecl.hp.general]/6.3.1, [saferecl.hp.holder.mem]/2-5,9: a node protected since before
//     the tree's root was retired (so before it was retired itself, by its parent's deleter) is
//     not reclaimed during the protection epoch; its subtree, retired only by its deleter, waits.
//   [saferecl.hp.base]/1,3: D is a function object type (a pointer to function is one,
//     [function.objects]/1) that is Cpp17DefaultConstructible and Cpp17MoveAssignable; a
//     move-only D holding state is one. retire's default argument D() is a default-constructed
//     deleter; here every retire passes its own, so a call of a default-constructed or moved-from
//     deleter is an error the deleters detect.
#include <hazard_pointer>
#include <atomic>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "watchdog.hpp"

static std::atomic<int> errors{0};

struct Filler;
struct FillerDeleter {
  void operator()(Filler* p) const noexcept;
};
struct Filler : std::hazard_pointer_obj_base<Filler, FillerDeleter> {};
void FillerDeleter::operator()(Filler* p) const noexcept { delete p; }
// Retires fillers until done(); false if that took more than `budget` retirements.
template<class Done> static bool drive(Done done, long budget) {
  for (long i = 0; !done(); ++i) {
    if (i == budget) return false;
    (new Filler)->retire();
  }
  return true;
}

// 1. A complete binary tree of 2^21 - 1 nodes in heap order (children of i: 2i+1, 2i+2), made
// as it is reclaimed: each node's deleter makes its two children and retires them. Node P, made
// in advance, is protected before the root is retired.
constexpr std::uint32_t TreeDepth = 21, TreeNodes = (1u << TreeDepth) - 1;
constexpr std::uint32_t P = (1u << 10) - 1 + 300;  // a node at depth 10
constexpr std::uint32_t PSubtree = (1u << (TreeDepth - 10)) - 1;
struct Tree;
struct TreeDeleter {
  std::uint32_t id = ~0u;
  void operator()(Tree* p) const noexcept;
};
struct Tree : std::hazard_pointer_obj_base<Tree, TreeDeleter> {
  std::uint32_t id = 0;
};
static std::atomic<std::uint8_t> tree_reclaimed[TreeNodes];
static std::atomic<std::uint32_t> trees_reclaimed{0};
static std::atomic<bool> p_protected{false};
static Tree* p_node = nullptr;
static void retire_tree(std::uint32_t id) {
  Tree* t = id == P ? p_node : new Tree;
  t->id = id;
  t->retire(TreeDeleter{id});
}
void TreeDeleter::operator()(Tree* p) const noexcept {
  const std::uint32_t i = p->id;
  if (id != i) errors.fetch_add(1);
  if (tree_reclaimed[i].fetch_add(1) != 0) errors.fetch_add(1);
  if (i == P && p_protected.load()) errors.fetch_add(1);
  delete p;
  if (2 * i + 2 < TreeNodes) {
    retire_tree(2 * i + 1);
    retire_tree(2 * i + 2);
  }
  trees_reclaimed.fetch_add(1);
}

// 2. One deleter retires 1,000,000 objects, each of whose deleters retires two more.
constexpr long Wide = 1'000'000;
struct Leaf;
struct LeafDeleter {
  std::atomic<long>* counter = nullptr;
  void operator()(Leaf* p) const noexcept;
};
struct Leaf : std::hazard_pointer_obj_base<Leaf, LeafDeleter> {
  int fan = 0;  // how many leaves this one's deleter retires
};
static std::atomic<long> leaves_reclaimed{0}, leaves_made{0};
static void retire_leaf(int fan) {
  Leaf* l = new Leaf;
  l->fan = fan;
  leaves_made.fetch_add(1);
  l->retire(LeafDeleter{&leaves_reclaimed});
}
void LeafDeleter::operator()(Leaf* p) const noexcept {
  if (!counter) {
    errors.fetch_add(1);
    return;
  }
  const int fan = p->fan;
  delete p;
  if (fan == -1)
    for (long k = 0; k < Wide; ++k) retire_leaf(2);
  else
    for (int k = 0; k < fan; ++k) retire_leaf(0);
  counter->fetch_add(1);
}

// 3. default_delete<T>: T's destructor retires the next node (made by it): a chain of 1,000,000.
constexpr std::uint32_t DChain = 1'000'000;
static std::atomic<std::uint8_t> dnode_destroyed[DChain];
static std::atomic<std::uint32_t> dnodes_destroyed{0};
struct DNode : std::hazard_pointer_obj_base<DNode> {
  std::uint32_t index;
  explicit DNode(std::uint32_t i) : index(i) {}
  ~DNode() {
    if (dnode_destroyed[index].fetch_add(1) != 0) errors.fetch_add(1);
    if (index + 1 < DChain) (new DNode(index + 1))->retire();
    dnodes_destroyed.fetch_add(1);
  }
};

// 4. A stateful, move-only deleter: a ternary tree of (3^13 - 1) / 2 = 797,161 nodes, each with
// a deleter owning its key; a default-constructed or moved-from deleter has none.
constexpr int TernaryDepth = 13;
struct Owned;
struct Key {
  std::uint64_t value;
  std::atomic<long>* counter;
};
struct OwningDeleter {
  std::unique_ptr<Key> key;
  void operator()(Owned* p) const noexcept;
};
static_assert(!std::is_copy_constructible_v<OwningDeleter>);
struct Owned : std::hazard_pointer_obj_base<Owned, OwningDeleter> {
  std::uint64_t expected = 0;
  int depth = 0;
};
static std::atomic<long> owned_reclaimed{0};
static std::uint64_t owned_key(std::uint64_t serial) { return serial * 0x9e3779b97f4a7c15u + 1; }
static std::atomic<std::uint64_t> owned_serial{0};
static void retire_owned(int depth) {
  Owned* o = new Owned;
  o->depth = depth;
  o->expected = owned_key(owned_serial.fetch_add(1));
  OwningDeleter d;
  d.key.reset(new Key{o->expected, &owned_reclaimed});
  o->retire(std::move(d));
}
void OwningDeleter::operator()(Owned* p) const noexcept {
  if (!key || key->value != p->expected) {
    errors.fetch_add(1);
    return;
  }
  std::atomic<long>* counter = key->counter;
  const int depth = p->depth;
  delete p;  // destroys this deleter, which p holds: nothing of it is used after this
  if (depth + 1 < TernaryDepth)
    for (int k = 0; k < 3; ++k) retire_owned(depth + 1);
  counter->fetch_add(1);
}

// 5. A pointer-to-function deleter: a chain of 500,000 alternating between two functions.
constexpr std::uint32_t FChain = 500'000;
struct FNode : std::hazard_pointer_obj_base<FNode, void (*)(FNode*)> {
  std::uint32_t index = 0;
};
static std::atomic<std::uint32_t> fnodes_reclaimed{0};
static void reclaim_odd(FNode*);
static void reclaim_even(FNode* p) {
  if (p->index % 2 != 0) errors.fetch_add(1);
  const std::uint32_t i = p->index;
  delete p;
  fnodes_reclaimed.fetch_add(1);
  if (i + 1 < FChain) {
    FNode* n = new FNode;
    n->index = i + 1;
    n->retire(&reclaim_odd);
  }
}
static void reclaim_odd(FNode* p) {
  if (p->index % 2 != 1) errors.fetch_add(1);
  const std::uint32_t i = p->index;
  delete p;
  fnodes_reclaimed.fetch_add(1);
  if (i + 1 < FChain) {
    FNode* n = new FNode;
    n->index = i + 1;
    n->retire(&reclaim_even);
  }
}

int main() {
  watchdog(150);

  // 1.
  p_node = new Tree;
  p_node->id = P;
  std::atomic<Tree*> src{p_node};
  std::hazard_pointer hp = std::make_hazard_pointer();
  CHECK(hp.protect(src) == p_node);
  p_protected.store(true);
  retire_tree(0);
  CHECK(drive([] { return trees_reclaimed.load() >= TreeNodes - PSubtree; }, 4L * TreeNodes));
  CHECK(drive([] { return false; }, 20000) == false);
  CHECK(trees_reclaimed.load() == TreeNodes - PSubtree);
  CHECK(tree_reclaimed[P].load() == 0 && p_node->id == P);
  CHECK(errors.load() == 0);
  p_protected.store(false);
  src.store(nullptr);
  hp.reset_protection();
  CHECK(drive([] { return trees_reclaimed.load() == TreeNodes; }, 4L * TreeNodes));
  for (std::uint32_t i = 0; i < TreeNodes; ++i) CHECK(tree_reclaimed[i].load() == 1);
  CHECK(errors.load() == 0);

  // 2.
  retire_leaf(-1);
  CHECK(drive([] { return leaves_reclaimed.load() == 1 + 3 * Wide; }, 8 * Wide));
  CHECK(leaves_made.load() == 1 + 3 * Wide);
  CHECK(errors.load() == 0);

  // 3.
  (new DNode(0))->retire();
  CHECK(drive([] { return dnodes_destroyed.load() == DChain; }, 4L * DChain));
  for (std::uint32_t i = 0; i < DChain; ++i) CHECK(dnode_destroyed[i].load() == 1);
  CHECK(errors.load() == 0);

  // 4.
  constexpr long TernaryNodes = (1594323 - 1) / 2;  // (3^13 - 1) / 2
  retire_owned(0);
  CHECK(drive([] { return owned_reclaimed.load() == TernaryNodes; }, 4 * TernaryNodes));
  CHECK(owned_serial.load() == std::uint64_t(TernaryNodes));
  CHECK(errors.load() == 0);

  // 5.
  {
    FNode* f = new FNode;
    f->retire(&reclaim_even);
  }
  CHECK(drive([] { return fnodes_reclaimed.load() == FChain; }, 4L * FChain));
  CHECK(errors.load() == 0);
  // No more reclamations than objects: nothing reclaimed twice.
  CHECK(drive([] { return false; }, 20000) == false);
  CHECK(trees_reclaimed.load() == TreeNodes && leaves_reclaimed.load() == 1 + 3 * Wide &&
        dnodes_destroyed.load() == DChain && owned_reclaimed.load() == TernaryNodes &&
        fnodes_reclaimed.load() == FChain);
  CHECK(errors.load() == 0);
  return 0;
}
