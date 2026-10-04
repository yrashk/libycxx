// Explicit instantiation of standard class templates for a program-defined type, split across
// two translation units: the definitions ([temp.explicit]/2, /11) in
// support/linkage/explicit_inst_tu2.cpp, the declarations (extern template) in the shared
// header, used here. [namespace.std]/5: "A program may explicitly instantiate a class template
// defined in the standard library only if the declaration (5.1) depends on the name of at least
// one program-defined type, and (5.2) the instantiation meets the standard library requirements
// for the original template" - both hold (P is copyable, default constructible, equality- and
// three-way comparable and hashable; Alloc meets Cpp17Allocator; Q is trivially copyable).
// So the explicit instantiation definitions must compile (every non-template member, e.g.
// list::remove, list::sort, vector::resize, is instantiated) and the program must link and
// behave normally ([temp.explicit]/14: the declarations suppress implicit instantiation of
// non-inline members, which the definitions then provide).
// FLAGS: -latomic ../../../../tests/ycxx/support/linkage/explicit_inst_tu2.cpp
//   (relative to the per-test temporary directory build/lit-*/linkage/<name>.XXXX)
#include "linkage/explicit_inst.hpp"
#include "check.hpp"

int main() {
  std::vector<P> v = tu2_vector();
  v.push_back(P(4));
  v.resize(6);
  CHECK(v.size() == 6 && v[3] == P(4) && v[5] == P());
  std::list<P> l(v.begin(), v.end());
  l.remove(P());
  l.sort(std::greater<>());
  l.unique();
  CHECK(l.size() == 4 && l.front() == P(4));
  std::forward_list<P> fl(l.begin(), l.end());
  fl.reverse();
  CHECK(fl.front() == P(1));
  std::deque<P> d(v.begin(), v.end());
  d.push_front(P(0));
  CHECK(d.size() == 7);
  std::map<P, P> m = tu2_map();
  m[P(3)] = P(30);
  CHECK(m.size() == 3 && m.at(P(2)) == P(20));
  std::multimap<P, P> mm(m.begin(), m.end());
  std::set<P> s(v.begin(), v.end());
  std::multiset<P> ms(v.begin(), v.end());
  CHECK(s.size() == 5 && ms.size() == 6 && mm.count(P(1)) == 1);
  std::unordered_map<P, P> um(m.begin(), m.end());
  std::unordered_multimap<P, P> umm(m.begin(), m.end());
  std::unordered_set<P> us(v.begin(), v.end());
  std::unordered_multiset<P> ums(v.begin(), v.end());
  CHECK(um.at(P(3)) == P(30) && umm.size() == 3 && us.size() == 5 && ums.size() == 6);
  std::flat_map<P, P> fm(m.begin(), m.end());
  std::flat_set<P> fs(v.begin(), v.end());
  CHECK(fm.size() == 3 && fs.size() == 5 && fs.contains(P(4)));
  std::inplace_vector<P, 4> iv{P(1), P(2)};
  CHECK(iv.try_push_back(P(3)) && iv.size() == 3);
  std::array<P, 3> a{P(1), P(2), P(3)};
  CHECK(a.back() == P(3));
  std::stack<P> st;
  st.push(P(1));
  std::queue<P> q;
  q.push(P(2));
  std::priority_queue<P> pq(v.begin(), v.end());
  CHECK(st.top() == P(1) && q.front() == P(2) && pq.top() == P(4));
  auto str = tu2_string();
  str += "!";
  CHECK(str.size() == 43 && str.back() == '!');
  std::vector<int, Alloc<int>> vi{1, 2, 3};
  CHECK(vi.size() == 3);
  std::optional<P> o = P(5);
  std::variant<P, int> var = P(6);
  P p7(7), p8(8);
  int i7 = 7, i9 = 9;
  std::tuple<P&, int&> t{p7, i7};
  std::pair<P&, int&> pr{p8, i9};
  const std::tuple<P&, int&> ct{p8, i9};
  ct.swap(t);  // the const swap: swaps the referred-to objects
  CHECK(p7.v == 8 && p8.v == 7 && i7 == 9 && i9 == 7);
  std::expected<P, int> e1 = P(10);
  std::expected<int, P> e2 = std::unexpected(P(11));
  CHECK(o->v == 5 && std::get<P>(var).v == 6 && std::get<0>(t).v == 8 && pr.second == 7);
  CHECK(e1->v == 10 && e2.error().v == 11);
  std::unique_ptr<P> up = std::make_unique<P>(12);
  std::unique_ptr<P[]> ua = std::make_unique<P[]>(2);
  std::shared_ptr<P> sp = std::make_shared<P>(13);
  std::weak_ptr<P> wp = sp;
  CHECK(up->v == 12 && ua[1] == P() && wp.lock()->v == 13);
  std::function<P(P)> f = [](P x) { return P(x.v + 1); };
  CHECK(f(P(1)) == P(2));
  std::span<P> sv(v);
  std::mdspan<P, std::dextents<std::size_t, 2>> md(v.data(), 2, 3);
  CHECK(sv.size() == 6 && (md[1, 0] == P(4)));
  std::atomic<Q> aq(Q{1, 2});
  aq.store(Q{3, 4});
  CHECK(aq.load().b == 4);
  std::reference_wrapper<P> rw(v[0]);
  CHECK(rw.get() == P(1));
  return 0;
}
