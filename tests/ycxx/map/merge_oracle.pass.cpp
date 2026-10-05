// merge between pseudo-random map / multimap / set / multiset pairs (also with a source ordered
// by greater<>, and as an rvalue), checked against an oracle.
// [associative.reqmts.general]/114: merge "Attempts to extract each element in a2 and insert it
// into a using the comparison object of a. In containers with unique keys, if there is an
// element in a with key equivalent to the key of an element from a2, then that element is not
// extracted from a2." /115: "Pointers and references to the transferred elements of a2 refer to
// those same elements but as members of a. If a.begin() and a2.begin() have the same type,
// iterators referring to the transferred elements will continue to refer to their elements, but
// they now behave as iterators into a". Insertion into a multi container (/68 and insert(nh))
// puts the element at the end of its range of equivalent keys, so a's own elements stay first.
// [map.modifiers], [multimap.modifiers], [set.modifiers], [multiset.modifiers]: merge takes
// map/multimap (set/multiset) with any comparator, as lvalue or rvalue.
#include <map>
#include <set>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>
#include "check.hpp"

struct Item {
  int key;
  int id;
};
struct ByKey {
  bool operator()(const Item& a, const Item& b) const { return a.key < b.key; }
};
struct ByKeyDesc {
  bool operator()(const Item& a, const Item& b) const { return a.key > b.key; }
};

unsigned st = 99u;
unsigned rnd(unsigned n) {
  st = st * 1103515245u + 12345u;
  return (st >> 16) % n;
}

template <class C>
std::pair<int, int> kid(const typename C::value_type& v) {
  if constexpr (requires { v.first; }) return {v.first, v.second};
  else return {v.key, v.id};
}
template <class C>
typename C::value_type make(int k, int id) {
  if constexpr (requires { typename C::mapped_type; }) return typename C::value_type(k, id);
  else return Item{k, id};
}
template <class C>
constexpr bool is_multi = requires(C c, typename C::value_type v) {
  { c.insert(v) } -> std::same_as<typename C::iterator>;
};

template <class A, class B>
void run(int na, int nb, int keys, bool as_rvalue) {
  A a;
  B b;
  int id = 0;
  for (int i = 0; i < na; ++i) a.insert(make<A>(static_cast<int>(rnd(static_cast<unsigned>(keys))), id++));
  for (int i = 0; i < nb; ++i) b.insert(make<B>(static_cast<int>(rnd(static_cast<unsigned>(keys))), id++));

  // oracle
  std::vector<std::pair<int, int>> a_before, b_before;
  for (auto& e : a) a_before.push_back(kid<A>(e));
  for (auto& e : b) b_before.push_back(kid<B>(e));
  std::vector<const typename B::value_type*> b_ptrs;
  std::vector<typename B::iterator> b_its;
  for (auto it = b.begin(); it != b.end(); ++it) {
    b_ptrs.push_back(&*it);
    b_its.push_back(it);
  }
  const std::size_t total = a.size() + b.size();

  if (as_rvalue) a.merge(std::move(b));
  else a.merge(b);

  // every element ends up in exactly one of the two; a stays sorted
  CHECK(a.size() + b.size() == total);
  CHECK(std::is_sorted(a.begin(), a.end(), [](const auto& x, const auto& y) { return kid<A>(x).first < kid<A>(y).first; }));
  std::vector<std::pair<int, int>> all, now;
  all.insert(all.end(), a_before.begin(), a_before.end());
  all.insert(all.end(), b_before.begin(), b_before.end());
  for (auto& e : a) now.push_back(kid<A>(e));
  for (auto& e : b) now.push_back(kid<B>(e));
  std::sort(all.begin(), all.end());
  std::sort(now.begin(), now.end());
  CHECK(all == now);
  // a's own elements are all still there, in their original order
  {
    std::vector<std::pair<int, int>> own;
    for (auto& e : a)
      if (std::find(a_before.begin(), a_before.end(), kid<A>(e)) != a_before.end()) own.push_back(kid<A>(e));
    CHECK(own == a_before);
  }
  if constexpr (is_multi<A>) {
    CHECK(b.empty());  // nothing is ever refused
    // within each run of equivalent keys a's own elements come first
    for (auto it = a.begin(); it != a.end();) {
      int k = kid<A>(*it).first;
      bool seen_new = false;
      for (; it != a.end() && kid<A>(*it).first == k; ++it) {
        bool own = std::find(a_before.begin(), a_before.end(), kid<A>(*it)) != a_before.end();
        CHECK(!(own && seen_new));
        seen_new = seen_new || !own;
      }
    }
  } else {
    // unique keys in a; an element of b stays exactly when its key was in a already or another
    // element of b with that key was transferred
    for (auto& e : b) {
      int k = kid<B>(e).first;
      if constexpr (requires { typename A::mapped_type; }) CHECK(a.count(k) == 1);
      else CHECK(a.count(Item{k, -1}) == 1);
    }
    for (auto& [k, i] : b_before) {
      bool in_a_before = std::any_of(a_before.begin(), a_before.end(), [&](auto& p) { return p.first == k; });
      if (!in_a_before) {
        // exactly one element with key k came over from b
        int from_b = 0;
        for (auto& e : a)
          if (kid<A>(e).first == k && std::find(a_before.begin(), a_before.end(), kid<A>(e)) == a_before.end()) ++from_b;
        CHECK(from_b == 1);
      } else {
        // b's element with key k was not extracted
        bool stays = false;
        for (auto& e : b) stays = stays || kid<B>(e) == std::pair{k, i};
        CHECK(stays);
      }
    }
  }
  // pointers to transferred elements now point into a; with the same iterator type, iterators
  // keep referring to them
  for (std::size_t j = 0; j < b_ptrs.size(); ++j) {
    bool in_b = false;
    for (auto& e : b) in_b = in_b || &e == b_ptrs[j];
    if (in_b) continue;
    bool found = false;
    for (auto& e : a) found = found || &e == b_ptrs[j];
    CHECK(found);
    CHECK(kid<B>(*b_ptrs[j]) == b_before[j]);
    if constexpr (std::is_same_v<decltype(a.begin()), decltype(b.begin())>) {
      CHECK(&*b_its[j] == b_ptrs[j]);
      // the iterator now walks a: advancing it reaches a.end()
      auto it = b_its[j];
      std::size_t steps = 0;
      while (it != a.end() && steps <= a.size()) {
        ++it;
        ++steps;
      }
      CHECK(it == a.end());
    }
  }
}

template <class A, class B>
void sweep() {
  for (int na : {0, 1, 5, 40})
    for (int nb : {0, 1, 7, 60})
      for (int keys : {1, 3, 20, 1000})
        for (bool rv : {false, true}) run<A, B>(na, nb, keys, rv);
}

int main() {
  using M = std::map<int, int>;
  using MM = std::multimap<int, int>;
  using MG = std::map<int, int, std::greater<>>;
  using MMG = std::multimap<int, int, std::greater<>>;
  sweep<M, M>();
  sweep<M, MM>();
  sweep<MM, M>();
  sweep<MM, MM>();
  sweep<M, MMG>();
  sweep<MM, MG>();
  using S = std::set<Item, ByKey>;
  using MS = std::multiset<Item, ByKey>;
  using SG = std::set<Item, ByKeyDesc>;
  using MSG = std::multiset<Item, ByKeyDesc>;
  sweep<S, S>();
  sweep<S, MS>();
  sweep<MS, S>();
  sweep<MS, MS>();
  sweep<S, MSG>();
  sweep<MS, SG>();
}
