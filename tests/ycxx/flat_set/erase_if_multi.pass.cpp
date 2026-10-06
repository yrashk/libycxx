// [flat.multiset.erasure]/2-5 and [flat.multimap.erasure]/2-5 (and [flat.set.erasure],
// [flat.map.erasure]): erase_if erases the elements e for which bool(pred(as_const(e))) (for the
// maps, bool(pred(pair<const Key&, const T&>(e)))) holds: the predicate sees const lvalues, its
// result is converted to bool explicitly; it returns the number erased, applies the predicate
// exactly c.size() times, and is stable: the remaining equivalent elements keep their order.
// erase_if is constexpr: checked in constant evaluation too.
#include <flat_map>
#include <flat_set>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

// A predicate result that converts to bool only explicitly.
struct Verdict {
  bool v;
  constexpr explicit operator bool() const { return v; }
};

struct Item {
  int key;
  int id;
  constexpr bool operator==(const Item&) const = default;
};
struct ByKey {
  constexpr bool operator()(const Item& a, const Item& b) const { return a.key < b.key; }
};

constexpr bool run() {
  // flat_multiset: equivalent items (same key) with distinct ids; erase the odd ids.
  std::flat_multiset<Item, ByKey> ms{{1, 0}, {2, 1}, {1, 2}, {2, 3}, {1, 4}, {3, 5}, {2, 6}};
  // The order of equivalent elements is whatever the constructor made: record it first.
  std::vector<int> before;
  for (const Item& i : ms) before.push_back(i.id);
  int calls = 0;
  auto n = std::erase_if(ms, [&calls](auto&& e) {
    static_assert(std::is_same_v<decltype(e), const Item&>);
    ++calls;
    return Verdict{e.id % 2 == 1};
  });
  if (n != 3 || calls != 7 || ms.size() != 4) return false;
  std::vector<int> expect;
  for (int id : before)
    if (id % 2 == 0) expect.push_back(id);
  std::vector<int> after;
  for (const Item& i : ms) after.push_back(i.id);
  if (after != expect) return false; // stable

  // flat_multimap: the predicate gets pair<const Key&, const T&>.
  std::flat_multimap<int, int> mm;
  for (int i = 0; i < 8; ++i) mm.emplace(i % 3, i);
  std::vector<int> mbefore;
  for (const auto& [k, v] : mm) mbefore.push_back(v);
  int mcalls = 0;
  auto m = std::erase_if(mm, [&mcalls](auto&& p) {
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(p)>, std::pair<const int&, const int&>>);
    ++mcalls;
    return p.first == 1 || p.second == 6;
  });
  if (m != 4 || mcalls != 8 || mm.size() != 4) return false; // keys 1: 1, 4, 7; value 6
  std::vector<int> mexpect;
  for (int v : mbefore)
    if (v % 3 != 1 && v != 6) mexpect.push_back(v);
  std::vector<int> mafter;
  for (const auto& [k, v] : mm) mafter.push_back(v);
  if (mafter != mexpect) return false;

  // flat_set and flat_map: the same rules.
  std::flat_set<int, std::greater<int>> s{5, 1, 4, 2, 3};
  int scalls = 0;
  if (std::erase_if(s, [&](const int& x) { ++scalls; return Verdict{x > 3}; }) != 2 || scalls != 5) return false;
  if (s != std::flat_set<int, std::greater<int>>{3, 2, 1}) return false;
  std::flat_map<int, char> fm{{1, 'a'}, {2, 'b'}, {3, 'c'}};
  if (std::erase_if(fm, [](std::pair<const int&, const char&> p) { return p.second == 'b'; }) != 1) return false;
  if (fm.size() != 2 || fm.contains(2)) return false;
  // Nothing to erase, and an empty container.
  if (std::erase_if(fm, [](auto) { return false; }) != 0 || fm.size() != 2) return false;
  std::flat_multiset<int> empty;
  int ecalls = 0;
  if (std::erase_if(empty, [&](int) { ++ecalls; return true; }) != 0 || ecalls != 0) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
