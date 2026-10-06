// [std.modules]/2: `import std;` provides the containers and their non-member functions, the
// container adaptors and views, with nothing #included.
// MODULES: std
import std;
#include "module_check.hpp"

struct key {
  int v;
  friend bool operator==(const key&, const key&) = default;
};
// A program may specialize a standard class template for its own type ([namespace.std]/2).
template <>
struct std::hash<key> {
  std::size_t operator()(const key& k) const noexcept { return std::hash<int>{}(k.v); }
};

int main() {
  std::vector<int> v{3, 1, 2};
  v.push_back(4);
  CHECK(v.size() == 4 && v[3] == 4);
  CHECK(std::erase(v, 1) == 1 && v == std::vector<int>({3, 2, 4}));
  std::vector<bool> vb(3, true);
  CHECK(vb[2]);
  std::inplace_vector<int, 4> iv{1, 2};
  CHECK(iv.size() == 2 && iv.capacity() == 4);
  std::array<int, 3> a{1, 2, 3};
  CHECK(std::get<1>(a) == 2 && std::to_array({1, 2}).size() == 2);
  std::deque<int> d{1, 2};
  d.push_front(0);
  CHECK(d.front() == 0);
  std::list<int> l{2, 1};
  l.sort();
  CHECK(l.front() == 1);
  std::forward_list<int> fl{1};
  fl.push_front(0);
  CHECK(fl.front() == 0);
  std::map<std::string, int> m{{"a", 1}};
  m["b"] = 2;
  CHECK(m.at("b") == 2 && m.contains("a"));
  std::multimap<int, int> mm{{1, 1}, {1, 2}};
  CHECK(mm.count(1) == 2);
  std::set<int> s{3, 1};
  CHECK(*s.begin() == 1);
  std::multiset<int> ms{1, 1};
  CHECK(ms.size() == 2);
  std::unordered_map<key, int> um{{key{1}, 10}};
  CHECK(um.at(key{1}) == 10);
  std::unordered_set<int> us{1, 2};
  CHECK(us.contains(2));
  std::unordered_multiset<int> ums{1, 1};
  CHECK(ums.count(1) == 2);
  std::flat_map<int, int> fm{{2, 20}, {1, 10}};
  CHECK(fm.begin()->first == 1);
  std::flat_set<int> fs{2, 1, 2};
  CHECK(fs.size() == 2);
  std::hive<int> h{1, 2, 3};
  CHECK(h.size() == 3);
  std::stack<int> st;
  st.push(1);
  CHECK(st.top() == 1);
  std::queue<int> q;
  q.push(1);
  CHECK(q.front() == 1);
  std::priority_queue<int> pq;
  pq.push(1);
  pq.push(3);
  CHECK(pq.top() == 3);
  std::span<const int> sp(v);
  CHECK(sp.size() == 3 && sp.subspan(1).front() == 2);
  int raw[6] = {0, 1, 2, 3, 4, 5};
  std::mdspan<int, std::extents<std::size_t, 2, 3>> md(raw);
  CHECK((md[1, 2] == 5));
  std::bitset<8> bs("1010");
  CHECK(bs.count() == 2);
  std::valarray<int> va{1, 2, 3};
  CHECK(va.sum() == 6);
  std::pmr::vector<int> pv{1, 2};
  CHECK(pv.size() == 2);
  std::pmr::monotonic_buffer_resource mr;
  std::pmr::polymorphic_allocator<int> pa(&mr);
  std::pmr::vector<int> pv2({1, 2, 3}, pa);
  CHECK(pv2.get_allocator().resource() == &mr);
  CHECK(std::begin(a) + 3 == std::end(a) && std::size(raw) == 6 && !std::empty(v));
  return 0;
}
