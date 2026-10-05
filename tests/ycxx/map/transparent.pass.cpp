// [associative.reqmts.general]/180: with Compare::is_transparent the member templates find,
// count, contains, lower_bound, upper_bound, equal_range, erase and extract accept a value of
// another type and compare it with the keys directly (no key_type temporary is made);
// without it they do not participate and the argument is converted to key_type; erase and
// extract do not take the template path for arguments convertible to iterator or
// const_iterator. [map.access], [map.modifiers]: the transparent operator[](K&&), at(const
// K&), try_emplace(K&&, args...) and insert_or_assign(K&&, M&&) construct a key from the
// argument only when an element is actually inserted.
// REQUIRES: exceptions
#include <map>
#include <functional>
#include <stdexcept>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "check.hpp"

using reqs::associative::Id;
using reqs::associative::IdLess;
using reqs::associative::IdLessPlain;

bool map_members() {
  std::map<Id, int, IdLess> m;
  m.emplace(Id(1), 10);
  m.emplace(Id(3), 30);
  Id::from_int = 0;
  // existing keys: no Id is made from the int
  if (m[1] != 10 || m.at(3) != 30 || std::as_const(m).at(1) != 10) return false;
  auto [it, inserted] = m.try_emplace(3, 99);
  if (inserted || it->second != 30) return false;
  auto [it2, ins2] = m.insert_or_assign(1, 11);
  if (ins2 || it2->second != 11) return false;
  auto h = m.try_emplace(m.cend(), 3, 98);
  if (h->second != 30) return false;
  auto h2 = m.insert_or_assign(m.cbegin(), 3, 31);
  if (h2->second != 31) return false;
  if (Id::from_int != 0) return false;
  // new keys: inserted, with a key made from the int
  m[5] = 50;
  auto [it3, ins3] = m.try_emplace(7, 70);
  auto [it4, ins4] = m.insert_or_assign(9, 90);
  if (!ins3 || !ins4 || it3->first.v != 7 || it4->second != 90 || m.size() != 5) return false;
  if (Id::from_int == 0) return false;
  int caught = 0;
  try { (void)m.at(4); } catch (const std::out_of_range&) { ++caught; }
  try { (void)std::as_const(m).at(4); } catch (const std::out_of_range&) { ++caught; }
  return caught == 2 && m.size() == 5;
}

int main() {
  CHECK((reqs::associative::transparent<std::map<Id, int, IdLess>, std::map<Id, int, IdLessPlain>>()));
  CHECK((reqs::associative::transparent<std::multimap<Id, Elem, IdLess>, std::multimap<Id, Elem, IdLessPlain>>()));
  CHECK(map_members());
  // std::less<> is transparent
  std::map<long, int, std::less<>> m{{1, 1}, {2, 2}};
  CHECK(m.find(2) != m.end() && m.contains(1) && m.count(3) == 0);
  return 0;
}
