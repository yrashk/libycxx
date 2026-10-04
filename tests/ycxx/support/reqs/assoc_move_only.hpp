// Generic check, instantiated per associative / unordered associative / flat container with
// a move-only element (map<K, MOElem>, set<MOElem>, ...; support/move_only_elem.hpp): every
// member whose requirements need only Cpp17MoveInsertable / Cpp17EmplaceConstructible
// elements must work with a type that cannot be copied.
//   [associative.reqmts.general]/47-56 (emplace, emplace_hint: Cpp17EmplaceConstructible
//     from args), /61-74 (insert(t), insert(p, t): "If t is a non-const rvalue, value_type
//     is Cpp17MoveInsertable"), /84-99 (extract / insert(nh): node handles never copy),
//     /112 (merge), /118-137 (erase); [unord.req.general]: the same members.
//   [container.reqmts]: move construction and move assignment, swap (no element requirement).
//   [map.modifiers]/7-31: try_emplace / insert_or_assign (mapped_type
//     Cpp17EmplaceConstructible from args, Cpp17MoveAssignable from obj for
//     insert_or_assign); [map.access]/1-4: operator[](key_type&&) needs only
//     Cpp17DefaultInsertable mapped_type; [unord.map.modifiers], [unord.map.elem]: the same.
//   [flat.map.modifiers]: try_emplace, insert_or_assign; extract() && moves the containers
//     out, replace(key_cont&&, mapped_cont&&) moves them in; [flat.set.modifiers]: the same
//     for flat_set.
#pragma once
#include <cstddef>
#include <iterator>
#include <utility>
#include "assoc_common.hpp"

namespace reqs::assoc_move_only {

using namespace reqs::assoc;

template <class X>
constexpr bool node_based = requires { typename X::node_type; };

template <class X>
constexpr bool test() {
  using V = typename X::value_type;
  using K = typename X::key_type;
  const bool multi = is_multi<X>;
  X a;
  a.emplace(elem<X>(3));
  if constexpr (is_map<X>) {
    a.emplace(K(1), MOElem(101));
  } else {
    a.emplace(1);
  }
  V v5 = elem<X>(5);
  a.insert(std::move(v5));  // insert(value_type&&)
  V v7 = elem<X>(7);
  a.insert(a.cend(), std::move(v7));  // insert(p, value_type&&)
  a.emplace_hint(a.cbegin(), elem<X>(0));
  if (!contents(a, {0, 1, 3, 5, 7})) return false;

  if constexpr (is_map<X>) {
    if constexpr (!multi) {
      auto [it, ins] = a.try_emplace(K(9), 109);
      if (!ins || !(it->second == MOElem(109))) return false;
      auto [it2, ins2] = a.try_emplace(K(9), 999);  // existing: no effect
      if (ins2 || !(it2->second == MOElem(109))) return false;
      a.try_emplace(a.cend(), K(11), 111);
      MOElem m(113);
      auto [it3, ins3] = a.insert_or_assign(K(13), std::move(m));
      if (!ins3 || m.value() != -1) return false;
      MOElem m2(113);
      a.insert_or_assign(K(13), std::move(m2));  // assigns (same value: contents unchanged)
      a.insert_or_assign(a.cend(), K(15), MOElem(115));
      if (!(a[K(13)] == MOElem(113))) return false;
      a[K(17)] = MOElem(117);  // operator[] value-initializes, then assignment
      K k19(19);
      a[std::move(k19)] = MOElem(119);
      if (!contents(a, {0, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19})) return false;
      a.erase(K(9));
      a.erase(K(11));
      a.erase(K(13));
      a.erase(K(15));
      a.erase(K(17));
      a.erase(K(19));
    }
  }
  if (!contents(a, {0, 1, 3, 5, 7})) return false;

  // move construction / assignment / swap
  X b(std::move(a));
  if (!contents(b, {0, 1, 3, 5, 7})) return false;
  X c;
  c.emplace(elem<X>(2));
  c = std::move(b);
  if (!contents(c, {0, 1, 3, 5, 7})) return false;
  X d;
  d.emplace(elem<X>(4));
  c.swap(d);
  using std::swap;
  swap(c, d);
  if (!contents(c, {0, 1, 3, 5, 7}) || !contents(d, {4})) return false;

  if constexpr (node_based<X>) {
    auto nh = c.extract(K(3));
    if (nh.empty()) return false;
    d.insert(std::move(nh));
    auto nh2 = c.extract(c.begin());
    d.insert(d.cend(), std::move(nh2));
    if (c.size() != 3 || d.size() != 3 || !d.contains(K(3))) return false;
    c.merge(d);
    if (!contents(c, {0, 1, 3, 4, 5, 7}) || !d.empty()) return false;
  } else {
    // flat: move the underlying containers out and back in
    auto conts = std::move(c).extract();
    if (!c.empty()) return false;
    if constexpr (is_map<X>) {
      c.replace(std::move(conts.keys), std::move(conts.values));
    } else {
      c.replace(std::move(conts));
    }
    if (!contents(c, {0, 1, 3, 5, 7})) return false;
    c.insert(elem<X>(4));
  }
  if (c.erase(K(4)) != 1) return false;
  c.erase(c.find(K(0)));
  if (!contents(c, {1, 3, 5, 7})) return false;
  if constexpr (multi) {
    c.emplace(elem<X>(3));
    if (c.count(K(3)) != 2) return false;
  }
  c.clear();
  return c.empty();
}

}  // namespace reqs::assoc_move_only
