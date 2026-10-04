// [forward.list.overview]/5: "An incomplete type T may be used when instantiating forward_list if the
// allocator meets the allocator completeness requirements ([allocator.requirements.completeness]).
// T shall be complete before any member of the resulting specialization of forward_list is
// referenced." std::allocator meets them ([default.allocator]/1). The classic use: a node
// type holding a list of itself.
#include <forward_list>

struct Node {
  int value = 0;
  std::forward_list<Node> children;  // Node is incomplete here
};

struct Fwd;
using ListOfFwd = std::forward_list<Fwd>;  // naming the specialization only
struct Holder {
  ListOfFwd* p = nullptr;
};
struct Fwd {
  int x;
};

int main() {
  Node root;
  root.children.push_front(Node{1, {}});
  root.children.front().children.emplace_front();
  ListOfFwd l{{1}, {2}};
  Holder h{&l};
  return (!root.children.empty() && !root.children.front().children.empty() && h.p->front().x == 1) ? 0 : 1;
}
