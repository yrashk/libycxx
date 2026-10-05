// flat_map's emplace, try_emplace, insert_or_assign and operator[] construct exactly the objects
// the draft names:
//   [flat.map.modifiers]/2 emplace(args): "Initializes an object t of type pair<key_type,
//     mapped_type> with std::forward<Args>(args)...; if the map already contains an element
//     whose key is equivalent to t.first, *this is unchanged. Otherwise, equivalent to: ...
//     c.keys.insert(key_it, std::move(t.first)); c.values.insert(value_it,
//     std::move(t.second));" So t is made even when the key exists (the arguments are
//     consumed), and an inserted mapped value is moved once from t, never copied.
//   /5 insert(P&& x) is emplace(std::forward<P>(x)).
//   /17 try_emplace(k, args): "If the map already contains an element whose key is equivalent
//     to k, *this and args... are unchanged. Otherwise ... c.keys.insert(key_it,
//     std::forward<decltype(k)>(k)); c.values.emplace(value_it, std::forward<Args>(args)...);"
//     /22 the heterogeneous form: c.keys.emplace(key_it, std::forward<K>(k)).
//   /26-27, /31 insert_or_assign: assigns std::forward<M>(obj) to e.second, or try_emplace(k,
//     std::forward<M>(obj)).
//   [flat.map.access]/1-4 operator[](x): try_emplace(x / std::move(x) /
//     std::forward<K>(x)).first->second.
// Insertions here are at the end of the containers, so the vector operations add no moves of
// other elements. flat_multimap's emplace ([flat.multimap.modifiers] is [flat.map.modifiers]'s
// wording without the uniqueness test via [flat.multimap.overview]) also makes t first.
#include <flat_map>
#include <utility>
#include <vector>
#include "inplace_probe.hpp"
#include "check.hpp"

using probe::Arg;
using probe::counts;
using probe::Probe;

struct Less {
  using is_transparent = void;
  static int key(int k) { return k; }
  static int key(const Probe& p) { return p.key; }
  template <class A, class B>
  bool operator()(const A& a, const B& b) const {
    return key(a) < key(b);
  }
};

// A map whose containers have room for 16 elements, so that no insertion reallocates.
template <class M>
M reserved() {
  typename M::key_container_type k;
  typename M::mapped_container_type v;
  k.reserve(16);
  v.reserve(16);
  return M(std::move(k), std::move(v));
}

void emplace_rules() {
  auto m = reserved<std::flat_map<int, Probe>>();
  Arg a{5};
  probe::reset();
  auto r = m.emplace(1, Probe(1, a));
  // Probe(1, a) made by the caller, t.second moved from it, the element moved from t.second.
  CHECK(r.second && r.first->second.cat == probe::lref);
  CHECK(counts.made == 1 && counts.copies == 0 && counts.moves == 2);
  CHECK(counts.copy_assigns + counts.move_assigns == 0);

  // Key exists: t is still initialized from the arguments (one move), then destroyed.
  probe::reset();
  Probe p(1, 9);
  r = m.emplace(1, std::move(p));
  CHECK(!r.second && r.first->second.extra == 5);
  CHECK(counts.moves == 1 && counts.copies == 0 && counts.destroyed == 1);

  // piecewise: t.second made from the arguments, then moved into the container.
  probe::reset();
  r = m.emplace(std::piecewise_construct, std::forward_as_tuple(2), std::forward_as_tuple(2, std::move(a)));
  CHECK(r.second && r.first->second.cat == probe::rref && a.moved_from);
  CHECK(counts.made == 1 && counts.copies == 0 && counts.moves == 1);

  // insert(P&&) = emplace(std::forward<P>(x)).
  probe::reset();
  std::pair<int, Probe> pr(3, Probe(3));
  probe::reset();
  r = m.insert(std::move(pr));
  CHECK(r.second && counts.copies == 0 && counts.moves == 2);
  probe::reset();
  r = m.insert(pr);  // lvalue pair: t.second copied; key 3 exists
  CHECK(!r.second && counts.copies == 1 && counts.moves == 0);
}

void try_emplace_rules() {
  auto m = reserved<std::flat_map<int, Probe>>();
  Arg a{5};
  probe::reset();
  auto r = m.try_emplace(1, 1, std::move(a));
  CHECK(r.second && a.moved_from && r.first->second.cat == probe::rref);
  CHECK(counts.made == 1 && counts.extra() == 0 && counts.destroyed == 0);
  a.moved_from = false;
  probe::reset();
  r = m.try_emplace(1, 7, std::move(a));
  CHECK(!r.second && !a.moved_from && r.first->second.key == 1);
  CHECK(counts.made == 0 && counts.extra() == 0);
  auto it = m.try_emplace(m.end(), 1, 7, std::move(a));
  CHECK(it->first == 1 && !a.moved_from && counts.made == 0 && counts.extra() == 0);
  it = m.try_emplace(m.end(), 2, 2, a);
  CHECK(it->second.cat == probe::lref && counts.made == 1 && counts.extra() == 0);

  // operator[]: the mapped value is value-initialized in place.
  probe::reset();
  Probe& v = m[3];
  CHECK(v.key == 0 && counts.made == 1 && counts.extra() == 0 && counts.destroyed == 0);
  probe::reset();
  (void)m[3];
  CHECK(counts.made == 0 && counts.extra() == 0);

  // insert_or_assign.
  Probe q(4, 4);
  probe::reset();
  r = m.insert_or_assign(4, std::move(q));
  CHECK(r.second && counts.moves == 1 && counts.copies == 0);
  probe::reset();
  r = m.insert_or_assign(4, q);
  CHECK(!r.second && counts.copy_assigns == 1 && counts.copies + counts.moves + counts.move_assigns == 0);
}

// Key type Probe: the key is copied (const key_type&), moved (key_type&&) or made from K.
void key_rules() {
  auto m = reserved<std::flat_map<Probe, int, Less>>();
  Probe k1(1);
  probe::reset();
  auto r = m.try_emplace(k1, 1);
  CHECK(r.second && counts.copies == 1 && counts.moves == 0 && counts.made == 0);
  probe::reset();
  r = m.try_emplace(Probe(2), 2);
  CHECK(r.second && counts.copies == 0 && counts.moves == 1 && counts.made == 1);
  probe::reset();
  r = m.try_emplace(3, 3);  // heterogeneous: made in place from the int
  CHECK(r.second && counts.made == 1 && counts.extra() == 0 && counts.destroyed == 0);
  probe::reset();
  r = m.try_emplace(3, 4);
  CHECK(!r.second && counts.made == 0 && counts.extra() == 0);
  probe::reset();
  m[4] = 4;
  CHECK(counts.made == 1 && counts.extra() == 0);
  probe::reset();
  m[4] = 5;
  CHECK(counts.made == 0 && counts.extra() == 0);
  probe::reset();
  auto ri = m.insert_or_assign(5, 5);
  CHECK(ri.second && counts.made == 1 && counts.extra() == 0);
  CHECK(m.size() == 5 && m.at(Probe(4)) == 5);
}

void multimap_rules() {
  auto m = reserved<std::flat_multimap<int, Probe>>();
  probe::reset();
  m.emplace(std::piecewise_construct, std::forward_as_tuple(1), std::forward_as_tuple(1, Arg{}));
  m.emplace(1, Probe(2));
  CHECK(m.size() == 2);
  CHECK(counts.copies == 0 && counts.copy_assigns == 0);
}

int main() {
  emplace_rules();
  try_emplace_rules();
  key_rules();
  multimap_rules();
  return 0;
}
