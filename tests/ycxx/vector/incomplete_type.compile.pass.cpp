// [vector.overview]/4: "An incomplete type T may be used when instantiating vector if the
// allocator meets the allocator completeness requirements." std::allocator does
// ([default.allocator]/1). T must be complete before any member is referenced.
#include <vector>

struct Node {
  int value;
  std::vector<Node> children;
};

struct Holder;
std::vector<Holder>* forward_declared();

inline int count(const Node& n) {
  int c = 1;
  for (const Node& k : n.children) c += count(k);
  return c;
}

void use() {
  Node root{1, {}};
  root.children.push_back(Node{2, {}});
  root.children[0].children.emplace_back();
  (void)count(root);
}
