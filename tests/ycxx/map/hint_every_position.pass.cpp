// Hinted insertion into map, multimap, set and multiset at every hint position, for keys below,
// between, equal to and above the existing ones (with runs of equivalent keys), checked against
// a sorted-vector oracle.
// [associative.reqmts.general]/57-58 a.emplace_hint(p, args): "Equivalent to
// a.emplace(std::forward<Args>(args)...), except that the element is inserted as close as
// possible to the position just prior to p"; /72-73 a.insert(p, t): inserts t iff its key is
// absent (unique keys) or always (equivalent keys), "as close as possible to the position just
// prior to p", and returns an iterator to the element with key equivalent to the key of t;
// a.insert(p, nh) likewise (an empty nh inserts nothing and returns end()). For equivalent
// keys the element thus goes to the valid position nearest to p: p itself when p is within or
// at an end of the run of equivalent keys, otherwise the end of the run nearer to p. Unique
// keys: an existing key is left alone and its element returned. [map.modifiers]: try_emplace
// and insert_or_assign with a hint, the latter assigning to an existing element.
#include <map>
#include <set>
#include <concepts>
#include <cstddef>
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

using Seq = std::vector<std::pair<int, int>>;  // (key, id) in container order

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
Seq contents(const C& c) {
  Seq s;
  for (auto& e : c) s.push_back(kid<C>(e));
  return s;
}

template <class C>
constexpr bool is_multi = requires(C c, typename C::value_type v) {
  { c.insert(v) } -> std::same_as<typename C::iterator>;
};

// The position at which a hinted insertion of key k before index h ends up.
std::size_t expected_index(const Seq& s, int k, std::size_t h, bool multi, bool& inserts) {
  std::size_t lb = 0;
  while (lb < s.size() && s[lb].first < k) ++lb;
  std::size_t ub = lb;
  while (ub < s.size() && s[ub].first == k) ++ub;
  if (!multi && ub != lb) {
    inserts = false;
    return lb;
  }
  inserts = true;
  return h < lb ? lb : h > ub ? ub : h;
}

template <class C>
void check_form(const Seq& base, int k, std::size_t h, int form) {
  C c;
  for (auto& [bk, bid] : base) c.insert(c.end(), make<C>(bk, bid));
  CHECK(contents(c) == base);
  const bool multi = is_multi<C>;
  const int id = 1000;
  auto p = std::next(c.cbegin(), static_cast<long>(h));
  typename C::iterator r;
  switch (form) {
    case 0: r = c.emplace_hint(p, make<C>(k, id)); break;
    case 1: {
      const auto v = make<C>(k, id);
      r = c.insert(p, v);
      break;
    }
    case 2: r = c.insert(p, make<C>(k, id)); break;
    default: {  // node handle from another container
      C other;
      other.insert(make<C>(k, id));
      auto nh = other.extract(other.begin());
      r = c.insert(p, std::move(nh));
      if (!multi && c.size() == base.size()) CHECK(!nh.empty());  // not inserted: nh keeps it
    }
  }
  bool inserts = false;
  std::size_t at = expected_index(base, k, h, multi, inserts);
  Seq want = base;
  if (inserts) want.insert(want.begin() + static_cast<long>(at), {k, id});
  CHECK(contents(c) == want);
  CHECK(r != c.end() && kid<C>(*r).first == k);
  CHECK(static_cast<std::size_t>(std::distance(c.begin(), r)) == at);
  if (!inserts) CHECK(kid<C>(*r).second == base[at].second);
}

template <class C>
void run() {
  // bases: sizes 0..9; keys 0, 2, 4, ... with some runs of equal keys (unique: distinct keys)
  for (std::size_t n = 0; n <= 9; ++n) {
    for (int pattern = 0; pattern < 3; ++pattern) {
      Seq base;
      int key = 0;
      for (std::size_t i = 0; i < n; ++i) {
        base.push_back({key, static_cast<int>(i)});
        bool repeat = is_multi<C> && (pattern == 1 ? i % 3 != 2 : pattern == 2);
        if (!repeat) key += 2;
      }
      for (int k = -1; k <= key + 1; ++k)
        for (std::size_t h = 0; h <= n; ++h)
          for (int form = 0; form < 4; ++form) check_form<C>(base, k, h, form);
    }
  }
}

// Unique-key maps: try_emplace and insert_or_assign with every hint.
template <class M>
void map_only() {
  for (std::size_t n = 0; n <= 6; ++n) {
    for (int k = -1; k <= 2 * static_cast<int>(n) + 1; ++k) {
      for (std::size_t h = 0; h <= n; ++h) {
        M m;
        for (std::size_t i = 0; i < n; ++i) m.emplace(2 * static_cast<int>(i), static_cast<int>(i));
        bool present = k >= 0 && k % 2 == 0 && k < 2 * static_cast<int>(n);
        auto r = m.try_emplace(std::next(m.cbegin(), static_cast<long>(h)), k, 77);
        CHECK(r->first == k && r->second == (present ? k / 2 : 77));
        CHECK(m.size() == n + !present);
        auto r2 = m.insert_or_assign(std::next(m.cbegin(), static_cast<long>(h > m.size() ? m.size() : h)), k, 88);
        CHECK(r2 == r && r2->second == 88 && m.size() == n + !present);
        int prev = -100;
        for (auto& [mk, mv] : m) {
          CHECK(mk > prev);
          prev = mk;
        }
      }
    }
  }
}

int main() {
  run<std::map<int, int>>();
  run<std::multimap<int, int>>();
  run<std::set<Item, ByKey>>();
  run<std::multiset<Item, ByKey>>();
  map_only<std::map<int, int>>();
  // an empty node handle inserts nothing and returns end()
  std::multimap<int, int> mm{{1, 1}};
  std::multimap<int, int>::node_type empty;
  CHECK(mm.insert(mm.begin(), std::move(empty)) == mm.end() && mm.size() == 1);
  std::set<int> s{1};
  std::set<int>::node_type empty2;
  CHECK(s.insert(s.end(), std::move(empty2)) == s.end() && s.size() == 1);
}
