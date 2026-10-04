// [associative.reqmts.general]/88-99: a_uniq.insert(nh) / a.insert(p, nh) take X::node_type;
// [container.node.overview] Table 75: only map / multimap with the same Key, T and Allocator
// have compatible nodes. A node handle of map<long, int> cannot be inserted into a
// map<int, int> (node handles have no converting constructors, [container.node.cons], and
// insert(P&&) needs value_type constructible from P). With a compatible container the
// insertion is fine (map/node_handle.pass.cpp).
#include <map>

int main() {
  std::map<int, int> a;
  std::map<long, int> b{{1, 2}};
  a.insert(b.extract(b.begin()));
  return 0;
}
