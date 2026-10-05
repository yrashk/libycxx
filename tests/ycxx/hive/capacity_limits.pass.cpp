// [hive.capacity]: capacity() is the number of elements storable without allocating new
// blocks; reserve(n) has postcondition capacity() >= n, has no effects if n <= capacity(),
// keeps all references and iterators valid, and throws length_error beyond max_size();
// trim_capacity() deallocates reserved blocks, trim_capacity(n) does nothing if n >=
// capacity(), both keep references and iterators valid; shrink_to_fit never increases
// capacity(); block_capacity_limits() returns the limits passed at construction (or
// reshape, or copied from the source by copy / move construction, [hive.cons]) and copy
// assignment leaves them unchanged; reshape keeps size(); swap exchanges contents,
// capacity() and limits ([hive.modifiers]/18).
// REQUIRES: exceptions
#include <hive>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>
#include "check.hpp"

using H = std::hive<int>;

bool same_elements(const H& h, std::vector<int> want) {
  std::vector<int> got(h.begin(), h.end());
  std::sort(got.begin(), got.end());
  std::sort(want.begin(), want.end());
  return got == want;
}

int main() {
  const std::hive_limits hard = H::block_capacity_hard_limits();
  const std::hive_limits def = H::block_capacity_default_limits();
  CHECK(hard.min <= def.min && def.min <= def.max && def.max <= hard.max);
  CHECK(H::is_within_hard_limits(std::hive_limits(hard.min, hard.min)));
  CHECK(!H::is_within_hard_limits(std::hive_limits(hard.min + 1, hard.min)) || hard.min + 1 == 0);
  CHECK(hard.max == static_cast<std::size_t>(-1) || !H::is_within_hard_limits(std::hive_limits(hard.min, hard.max + 1)));

  H h;
  CHECK(h.block_capacity_limits().min == def.min && h.block_capacity_limits().max == def.max);
  CHECK(h.capacity() >= h.size());
  h.reserve(100);
  CHECK(h.capacity() >= 100 && h.empty());
  const std::size_t cap = h.capacity();
  h.reserve(10);
  CHECK(h.capacity() == cap);
  for (int i = 0; i < 50; ++i) h.insert(i);
  auto it = h.begin();
  int* p = &*it;
  auto end = h.end();
  h.reserve(5000);
  CHECK(h.capacity() >= 5000 && &*it == p && end == h.end() && h.size() == 50);
  h.trim_capacity(h.capacity() + 10);
  CHECK(h.capacity() >= 5000);
  h.trim_capacity();
  CHECK(h.capacity() >= h.size() && &*it == p && end == h.end());
  const std::size_t before = h.capacity();
  h.shrink_to_fit();
  CHECK(h.capacity() <= before && h.capacity() >= h.size() && h.size() == 50);
  bool threw = false;
  try {
    h.reserve(h.max_size() == static_cast<std::size_t>(-1) ? h.max_size() : h.max_size() + 1);
  } catch (const std::length_error&) {
    threw = true;
  } catch (...) {
    threw = true;  // any exception thrown by the allocator is allowed as well
  }
  CHECK(threw || h.max_size() == static_cast<std::size_t>(-1));

  // limits: chosen at construction, copied by copy / move construction
  const std::hive_limits lim(std::max<std::size_t>(hard.min, 4), std::max<std::size_t>(hard.min, 8));
  CHECK(H::is_within_hard_limits(lim));
  H l(lim);
  CHECK(l.block_capacity_limits().min == lim.min && l.block_capacity_limits().max == lim.max);
  for (int i = 0; i < 100; ++i) l.insert(i);
  H copy(l);
  CHECK(copy.block_capacity_limits().min == lim.min && copy.block_capacity_limits().max == lim.max);
  H moved(std::move(copy));
  CHECK(moved.block_capacity_limits().max == lim.max && moved.size() == 100);
  H assigned;
  assigned = l;  // copy assignment leaves current-limits unchanged
  CHECK(assigned.block_capacity_limits().min == def.min && assigned.size() == 100);
  H with_n(std::size_t(20), 3, lim);
  CHECK(with_n.size() == 20 && with_n.block_capacity_limits().min == lim.min);
  // reshape keeps the elements
  std::vector<int> all(l.begin(), l.end());
  l.reshape(def);
  CHECK(l.block_capacity_limits().min == def.min && l.block_capacity_limits().max == def.max);
  CHECK(l.size() == 100 && same_elements(l, all));
  // swap exchanges contents, capacity and limits
  H a(lim), b;
  a.insert(1);
  b.reserve(500);
  b.insert(2);
  const std::size_t acap = a.capacity(), bcap = b.capacity();
  a.swap(b);
  CHECK(*a.begin() == 2 && *b.begin() == 1 && a.capacity() == bcap && b.capacity() == acap);
  CHECK(b.block_capacity_limits().min == lim.min && a.block_capacity_limits().min == def.min);
  return 0;
}
