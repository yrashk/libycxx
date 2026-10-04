// [util.smartptr.shared.general] (C++26): the constructors, destructor, assignment,
// modifiers, observers other than owner_before / owner_hash, comparisons, casts and the
// make_shared family of shared_ptr, and weak_ptr's construction, use_count, expired and
// lock, are constexpr, so shared ownership works during constant evaluation.
#include <memory>
#include <utility>
#include "check.hpp"

struct Node {
  int v;
  constexpr explicit Node(int x) : v(x) {}
};

constexpr bool test() {
  std::shared_ptr<Node> a = std::make_shared<Node>(1);
  std::shared_ptr<Node> b = a;
  if (a.use_count() != 2 || b->v != 1 || a != b) return false;
  std::weak_ptr<Node> w = a;
  if (w.expired() || w.use_count() != 2) return false;
  b.reset();
  if (a.use_count() != 1 || !w.lock()) return false;
  std::shared_ptr<Node> c(new Node(2));
  a = std::move(c);
  if (c || a->v != 2 || !w.expired()) return false;
  std::shared_ptr<const Node> k = std::const_pointer_cast<const Node>(a);
  if (k.get() != a.get() || !k.owner_equal(a)) return false;
  std::shared_ptr<int[]> arr = std::make_shared<int[]>(3, 4);
  if (arr[2] != 4) return false;
  std::shared_ptr<int> alias(a, &a->v);
  return *alias == 2 && a.use_count() == 3;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
