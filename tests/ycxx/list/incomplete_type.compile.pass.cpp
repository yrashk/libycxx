// [list.overview]/4: "An incomplete type T may be used when instantiating list if the
// allocator meets the allocator completeness requirements ([allocator.requirements.completeness]).
// T shall be complete before any member of the resulting specialization of list is
// referenced." std::allocator meets them ([default.allocator]/1). The classic use: a node
// type holding a list of itself.
#include <list>

struct Node {
  int value = 0;
  std::list<Node> children;  // Node is incomplete here
};

struct Fwd;
using ListOfFwd = std::list<Fwd>;  // naming the specialization only
struct Holder {
  ListOfFwd* p = nullptr;
};
struct Fwd {
  int x;
};

int main() {
  Node root;
  root.children.push_back(Node{1, {}});
  root.children.front().children.emplace_back();
  ListOfFwd l{{1}, {2}};
  Holder h{&l};
  return (root.children.size() == 1 && root.children.front().children.size() == 1 && h.p->size() == 2) ? 0 : 1;
}
