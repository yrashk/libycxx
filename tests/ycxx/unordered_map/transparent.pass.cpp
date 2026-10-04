// [unord.req.general]/245: find, count, equal_range, contains, extract, erase and bucket
// templates take part only if both Pred::is_transparent and Hash::is_transparent name types;
// they then hash and compare the argument without converting it to key_type. With only one
// of the two transparent, the argument is converted. erase / extract with an iterator are
// the position forms. [unord.map.elem] / [unord.map.modifiers]: the heterogeneous
// operator[], at, try_emplace and insert_or_assign make a key only when inserting.
#include <unordered_map>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Id {
  int v;
  static inline int from_int = 0;
  Id(int x) : v(x) { ++from_int; }
  Id(const Id&) = default;
  bool operator==(const Id&) const = default;
};
struct THash {
  using is_transparent = void;
  std::size_t operator()(const Id& i) const { return static_cast<std::size_t>(i.v) * 31u; }
  std::size_t operator()(int i) const { return static_cast<std::size_t>(i) * 31u; }
};
struct TEq {
  using is_transparent = void;
  bool operator()(const Id& a, const Id& b) const { return a.v == b.v; }
  bool operator()(const Id& a, int b) const { return a.v == b; }
  bool operator()(int a, const Id& b) const { return a == b.v; }
};
struct PHash {  // not transparent
  std::size_t operator()(const Id& i) const { return static_cast<std::size_t>(i.v) * 31u; }
};
struct PEq {  // not transparent
  bool operator()(const Id& a, const Id& b) const { return a.v == b.v; }
};

template <class M>
bool transparent_lookup() {
  M m;
  for (int k : {1, 2, 3}) m.emplace(Id(k), k * 10);
  m.emplace(Id(2), 21);
  const bool multi = m.size() == 4;
  Id::from_int = 0;
  if (m.find(2) == m.end() || m.find(5) != m.end() || !m.contains(3) || m.count(2) != (multi ? 2u : 1u)) return false;
  auto [lo, hi] = m.equal_range(2);
  if (std::distance(lo, hi) != (multi ? 2 : 1) || m.bucket(2) != m.bucket(m.find(2)->first)) return false;
  if (Id::from_int != 0) return false;
  if (m.erase(2) != (multi ? 2u : 1u) || m.extract(3).empty() || !m.extract(9).empty() || Id::from_int != 0) return false;
  static_assert(std::is_same_v<decltype(m.erase(m.begin())), typename M::iterator>);
  auto it = m.erase(m.begin());
  return it == m.end() && m.empty() && m.extract(m.cbegin() == m.cend() ? 1 : 1).empty();
}

template <class M>
bool converting_lookup() {
  M m;
  m.emplace(Id(1), 10);
  Id::from_int = 0;
  if (m.find(1) == m.end() || !m.contains(1) || m.count(2) != 0) return false;
  return Id::from_int >= 3;
}

bool map_members() {
  std::unordered_map<Id, int, THash, TEq> m;
  m.emplace(Id(1), 10);
  Id::from_int = 0;
  if (m[1] != 10 || m.at(1) != 10 || std::as_const(m).at(1) != 10) return false;
  if (m.try_emplace(1, 5).second || m.insert_or_assign(1, 11).second || m.at(1) != 11) return false;
  if (Id::from_int != 0) return false;
  m[2] = 20;
  if (!m.try_emplace(3, 30).second || !m.insert_or_assign(4, 40).second || m.size() != 4) return false;
  int caught = 0;
  try { (void)m.at(9); } catch (const std::out_of_range&) { ++caught; }
  return caught == 1 && Id::from_int >= 3;
}

int main() {
  CHECK((transparent_lookup<std::unordered_map<Id, int, THash, TEq>>()));
  CHECK((transparent_lookup<std::unordered_multimap<Id, int, THash, TEq>>()));
  CHECK((converting_lookup<std::unordered_map<Id, int, PHash, PEq>>()));
  CHECK((converting_lookup<std::unordered_map<Id, int, THash, PEq>>()));  // only the hash is transparent
  CHECK((converting_lookup<std::unordered_map<Id, int, PHash, TEq>>()));  // only the predicate is transparent
  CHECK(map_members());
  return 0;
}
