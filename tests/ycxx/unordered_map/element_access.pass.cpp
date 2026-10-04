// [unord.map.elem]: operator[](k) inserts a value-initialized mapped value when the key is
// absent (try_emplace(k).first->second), at(k) throws out_of_range when absent;
// [unord.map.modifiers]: try_emplace has no effect (arguments not moved from) when the key
// exists; insert_or_assign assigns to the existing mapped value or inserts; insert(P&&) is
// emplace(std::forward<P>(x)). References returned stay valid across rehashing
// ([unord.req.general]/9).
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

template <class M>
concept has_subscript = requires(M& m) { m[1]; };
template <class M>
concept has_at = requires(M& m) { m.at(1); };

int main() {
  std::unordered_map<int, long> m;
  long& r = m[5];
  CHECK(r == 0 && m.size() == 1);
  r = 50;
  for (int i = 0; i < 500; ++i) m[100 + i] = i;  // forces rehashing
  CHECK(&m[5] == &r && m[5] == 50 && m.size() == 501);
  CHECK(m.at(5) == 50 && std::as_const(m).at(100) == 0);
  int caught = 0;
  try { (void)m.at(7); } catch (const std::out_of_range&) { ++caught; }
  try { (void)std::as_const(m).at(7); } catch (const std::out_of_range&) { ++caught; }
  CHECK(caught == 2 && m.size() == 501);

  std::unordered_map<int, std::unique_ptr<int>> u;
  auto p = std::make_unique<int>(1);
  auto [i1, b1] = u.try_emplace(1, std::move(p));
  CHECK(b1 && !p && *i1->second == 1);
  auto q = std::make_unique<int>(2);
  auto [i2, b2] = u.try_emplace(1, std::move(q));
  CHECK(!b2 && q && i2 == i1);
  auto h = u.try_emplace(u.cbegin(), 1, std::move(q));
  CHECK(h == i1 && q);
  auto [i3, b3] = u.insert_or_assign(1, std::move(q));
  CHECK(!b3 && !q && *i3->second == 2 && i3 == i1);
  auto [i4, b4] = u.insert_or_assign(2, std::make_unique<int>(3));
  CHECK(b4 && *i4->second == 3 && u.size() == 2);
  auto h2 = u.insert_or_assign(u.cend(), 3, nullptr);
  CHECK(h2->first == 3 && !h2->second && u.size() == 3);

  std::unordered_map<int, long> c;
  c.insert(std::pair<long, int>(1, 10));  // P&&: converted
  c.insert({2, 20});
  c.emplace(3, 30);
  c.emplace(std::piecewise_construct, std::forward_as_tuple(4), std::forward_as_tuple(40));
  CHECK(c.size() == 4 && c[1] == 10 && c[4] == 40);
  static_assert(!has_subscript<std::unordered_multimap<int, int>> && !has_at<std::unordered_multimap<int, int>>);
  static_assert(has_subscript<std::unordered_map<int, int>> && has_at<std::unordered_map<int, int>>);
  return 0;
}
