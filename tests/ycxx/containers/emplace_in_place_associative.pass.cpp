// The associative and unordered containers construct their elements in place from the
// forwarded arguments, and the key-first operations construct nothing when the key exists.
//   [associative.reqmts.general], [unord.req.general] a_uniq.emplace(args) / a_eq.emplace /
//     a.emplace_hint(p, args): Preconditions: value_type is Cpp17EmplaceConstructible from args;
//     "Inserts a value_type object t constructed with std::forward<Args>(args)..." So a type
//     that can be neither copied nor moved works, and an inserted element is made once.
//   a.insert(p, rv) for an rvalue t: Cpp17MoveInsertable; one move construction, no copy.
//   a.insert(nh): no element is constructed, copied or moved.
//   [map.modifiers]/3-13, [unord.map.modifiers] try_emplace(k, args) (also with a hint and
//     with a heterogeneous K): "If the map already contains an element whose key is equivalent
//     to k, there is no effect" (args are not consumed); otherwise inserts value_type
//     constructed with piecewise_construct, forward_as_tuple(k) (or std::move(k), or
//     std::forward<K>(k)), forward_as_tuple(std::forward<Args>(args)...).
//   insert_or_assign(k, obj): assigns std::forward<M>(obj) to the mapped value, or inserts
//     value_type constructed with k, std::forward<M>(obj).
//   [map.access]/1-4, [unord.map.elem]: operator[](x) is try_emplace(x / std::move(x) /
//     std::forward<K>(x)).first->second.
//   [set.modifiers]/3, [unord.set.modifiers]/3: insert(K&& x) with a transparent comparator /
//     hash: "If the set already contains an element that is equivalent to x, there is no
//     effect. Otherwise ... Constructs an object u of type value_type with
//     std::forward<K>(x)" and inserts it.
#include <functional>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "inplace_probe.hpp"
#include "check.hpp"

using probe::Arg;
using probe::counts;
using probe::Pinned;
using probe::Probe;

// Transparent comparison, hash and equality that never convert their arguments.
constexpr int key_of(int k) { return k; }
template <bool M>
int key_of(const probe::Basic<M>& p) {
  return p.key;
}
struct Less {
  using is_transparent = void;
  template <class A, class B>
  bool operator()(const A& a, const B& b) const {
    return key_of(a) < key_of(b);
  }
};
struct Eq {
  using is_transparent = void;
  template <class A, class B>
  bool operator()(const A& a, const B& b) const {
    return key_of(a) == key_of(b);
  }
};
struct Hash {
  using is_transparent = void;
  template <class A>
  std::size_t operator()(const A& a) const {
    return static_cast<std::size_t>(key_of(a));
  }
};

template <class S>
void set_like(S s) {
  Arg a{3};
  probe::reset();
  auto r = s.emplace(1, a);
  CHECK(r.second && r.first->cat == probe::lref);
  CHECK(counts.made == 1 && counts.extra() == 0 && counts.destroyed == 0);
  probe::reset();
  auto it = s.emplace_hint(s.end(), 2, std::move(a));
  CHECK(it->key == 2 && it->cat == probe::rref);
  CHECK(counts.made == 1 && counts.extra() == 0 && counts.destroyed == 0);
  // Heterogeneous insert: u is constructed from the int; nothing when the key exists.
  probe::reset();
  auto h = s.insert(5);
  CHECK(h.second && h.first->key == 5);
  CHECK(counts.made == 1 && counts.extra() == 0 && counts.destroyed == 0);
  probe::reset();
  h = s.insert(5);
  CHECK(!h.second && h.first->key == 5);
  CHECK(counts.made == 0 && counts.extra() == 0 && counts.destroyed == 0);
  probe::reset();
  auto hi = s.insert(s.end(), 6);
  CHECK(hi->key == 6 && counts.made == 1 && counts.extra() == 0);
  probe::reset();
  hi = s.insert(s.begin(), 6);
  CHECK(hi->key == 6 && counts.made == 0 && counts.extra() == 0);
  // insert(hint, rvalue): one move.
  probe::reset();
  hi = s.insert(s.end(), Probe(7));
  CHECK(hi->key == 7 && counts.moves == 1 && counts.copies == 0);
  // Node handles: nothing constructed.
  probe::reset();
  auto nh = s.extract(5);
  CHECK(!nh.empty() && counts.extra() == 0 && counts.made == 0);
  auto ir = s.insert(std::move(nh));
  CHECK(ir.inserted && counts.extra() == 0 && counts.made == 0 && counts.destroyed == 0);
  nh = s.extract(6);
  hi = s.insert(s.end(), std::move(nh));
  CHECK(hi->key == 6 && counts.extra() == 0 && counts.made == 0 && counts.destroyed == 0);
}

template <class S>
void multiset_like(S s) {
  Arg a{};
  probe::reset();
  s.emplace(1, a);
  s.emplace(1, std::move(a));
  s.emplace_hint(s.begin(), 1);
  CHECK(s.size() == 3 && counts.made == 3 && counts.extra() == 0 && counts.destroyed == 0);
}

template <class S>
void pinned_set(S s) {
  Arg a{};
  CHECK(s.emplace(2, a).first->cat == probe::lref);
  CHECK(s.emplace_hint(s.end(), 1, std::move(a))->cat == probe::rref);
  CHECK(!s.emplace(2).second);
  CHECK(s.size() == 2);
}

template <class S>
void pinned_multiset(S s) {
  s.emplace(1, Arg{});
  s.emplace(1);
  s.emplace_hint(s.end(), 1);
  CHECK(s.size() == 3);
}

// map-like with key Probe and mapped Pinned.
template <class M>
void map_like(M m) {
  Arg a{4};
  probe::reset();
  auto r = m.emplace(std::piecewise_construct, std::forward_as_tuple(1), std::forward_as_tuple(10, a));
  CHECK(r.second && r.first->second.cat == probe::lref);
  CHECK(counts.made == 2 && counts.extra() == 0 && counts.destroyed == 0);

  // try_emplace: key exists -> nothing constructed, the rvalue argument is untouched.
  Probe k1(1);
  probe::reset();
  auto t = m.try_emplace(k1, 11, std::move(a));
  CHECK(!t.second && !a.moved_from && t.first->second.key == 10);
  CHECK(counts.made == 0 && counts.extra() == 0 && counts.destroyed == 0);
  t = m.try_emplace(Probe(1), 11, std::move(a));
  CHECK(!t.second && !a.moved_from);
  CHECK(counts.made == 1 && counts.extra() == 0);  // only the caller's Probe(1)
  probe::reset();
  t = m.try_emplace(1, 11, std::move(a));  // heterogeneous
  CHECK(!t.second && !a.moved_from && counts.made == 0 && counts.extra() == 0);
  auto ti = m.try_emplace(m.end(), 1, 11, std::move(a));
  CHECK(ti->first.key == 1 && !a.moved_from && counts.made == 0 && counts.extra() == 0);

  // try_emplace inserting: key copied (const&), moved (&&) or made from K; value made once.
  Probe k2(2);
  probe::reset();
  t = m.try_emplace(k2, 20, a);
  CHECK(t.second && t.first->second.cat == probe::lref);
  CHECK(counts.copies == 1 && counts.moves == 0 && counts.made == 1);
  probe::reset();
  t = m.try_emplace(Probe(3), 30, std::move(a));
  CHECK(t.second && a.moved_from && t.first->second.cat == probe::rref);
  CHECK(counts.copies == 0 && counts.moves == 1 && counts.made == 2);
  probe::reset();
  t = m.try_emplace(4, 40);
  CHECK(t.second && counts.made == 2 && counts.extra() == 0 && counts.destroyed == 0);
  probe::reset();
  ti = m.try_emplace(m.end(), 5, 50, Arg{});
  CHECK(ti->first.key == 5 && ti->second.cat == probe::rref);
  CHECK(counts.made == 2 && counts.extra() == 0 && counts.destroyed == 0);

  // operator[]: heterogeneous key made in place, mapped value made once.
  probe::reset();
  auto& v6 = m[6];
  CHECK(v6.key == 0 && counts.made == 2 && counts.extra() == 0 && counts.destroyed == 0);
  probe::reset();
  (void)m[6];
  CHECK(counts.made == 0 && counts.extra() == 0);
  probe::reset();
  (void)m[Probe(7)];
  CHECK(counts.made == 2 && counts.moves == 1 && counts.copies == 0);
  Probe k8(8);
  probe::reset();
  (void)m[k8];
  CHECK(counts.made == 1 && counts.copies == 1 && counts.moves == 0);
}

template <class M>
void insert_or_assign_like(M m) {
  probe::reset();
  Probe p(1, 2);
  auto r = m.insert_or_assign(1, std::move(p));
  CHECK(r.second && r.first->second.extra == 2);
  CHECK(counts.moves == 1 && counts.copies == 0 && counts.made == 1);
  probe::reset();
  r = m.insert_or_assign(1, p);
  CHECK(!r.second && counts.copy_assigns == 1 && counts.copies + counts.moves + counts.move_assigns == 0);
  probe::reset();
  r = m.insert_or_assign(1, std::move(p));
  CHECK(!r.second && counts.move_assigns == 1 && counts.copies + counts.moves + counts.copy_assigns == 0);
  probe::reset();
  auto it = m.insert_or_assign(m.end(), 2, std::move(p));
  CHECK(it->first == 2 && counts.moves == 1 && counts.copies == 0);
  // emplace(k, v) with a pair-constructible argument list, insert(P&&) and insert(hint, P&&).
  probe::reset();
  auto e = m.emplace(3, std::move(p));
  CHECK(e.second && counts.moves == 1 && counts.copies == 0);
  probe::reset();
  auto i = m.insert(std::pair<int, Probe>(4, Probe(4)));
  CHECK(i.second && counts.copies == 0);
  probe::reset();
  std::pair<const int, Probe> vt(5, Probe(5));
  probe::reset();
  auto ih = m.insert(m.end(), std::move(vt));
  CHECK(ih->first == 5 && counts.copies == 0 && counts.moves == 1);
}

template <class M>
void multimap_like(M m) {
  probe::reset();
  m.emplace(std::piecewise_construct, std::forward_as_tuple(1), std::forward_as_tuple(1, Arg{}));
  m.emplace_hint(m.end(), std::piecewise_construct, std::forward_as_tuple(1), std::forward_as_tuple(2));
  CHECK(m.size() == 2 && counts.made == 4 && counts.extra() == 0 && counts.destroyed == 0);
}

int main() {
  set_like(std::set<Probe, Less>{});
  set_like(std::unordered_set<Probe, Hash, Eq>{});
  multiset_like(std::multiset<Probe, Less>{});
  multiset_like(std::unordered_multiset<Probe, Hash, Eq>{});
  pinned_set(std::set<Pinned, Less>{});
  pinned_set(std::unordered_set<Pinned, Hash, Eq>{});
  pinned_multiset(std::multiset<Pinned, Less>{});
  pinned_multiset(std::unordered_multiset<Pinned, Hash, Eq>{});
  map_like(std::map<Probe, Pinned, Less>{});
  map_like(std::unordered_map<Probe, Pinned, Hash, Eq>{});
  insert_or_assign_like(std::map<int, Probe>{});
  insert_or_assign_like(std::unordered_map<int, Probe>{});
  multimap_like(std::multimap<Probe, Pinned, Less>{});
  multimap_like(std::unordered_multimap<Probe, Pinned, Hash, Eq>{});
  return 0;
}
