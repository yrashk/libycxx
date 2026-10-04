// [vector.bool.pspc]/2: unless described otherwise, vector<bool> has the same requirements
// and semantics as the primary template: constructors (n, n + value, iterator pair,
// from_range, initializer_list, copy/move), assign family, element access (at throws
// out_of_range), push_back / emplace_back / pop_back, insert / emplace / insert_range /
// erase / append_range, resize(sz, c = false), reserve / capacity, swap, comparison.
#include <vector>
#include <ranges>
#include <stdexcept>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  std::vector<bool> a(10);
  for (bool b : a)
    if (b) return false;
  std::vector<bool> b(5, true);
  if (b.size() != 5 || !b[4]) return false;
  bool src[] = {true, false, true, true};
  std::vector<bool> c(InputIter<bool>(src), InputIter<bool>(src + 4));
  if (c.size() != 4 || c[1] || !c[3]) return false;
  std::vector<bool> d(std::from_range, InputRange<bool>{src, src + 3});
  if (d.size() != 3 || !d[2]) return false;
  std::vector<bool> e{true, false};
  if (e.size() != 2 || !e[0] || e[1]) return false;
  std::vector<bool> f(std::from_range, std::views::iota(0, 4) | std::views::transform([](int i) { return i % 2 == 1; }));
  if (f.size() != 4 || f[0] || !f[1]) return false;

  std::vector<bool> v;
  for (int i = 0; i < 100; ++i) v.push_back(i % 3 == 0);
  if (v.size() != 100 || !v[99] || v[98]) return false;
  bool& unused = src[0];
  (void)unused;
  auto r = v.emplace_back(true);
  if (!r || !v.back()) return false;
  v.pop_back();
  auto it = v.insert(v.begin() + 1, true);
  if (it != v.begin() + 1 || !v[1] || v.size() != 101) return false;
  it = v.insert(v.end(), 3, true);
  if (it != v.end() - 3 || !v[100] || !v[103]) return false;
  it = v.insert(v.begin(), {false, false});
  if (it != v.begin() || v[0] || v[1] || v.size() != 106) return false;
  it = v.insert_range(v.begin() + 2, std::vector<bool>{true, true});
  if (it != v.begin() + 2 || !v[2] || !v[3]) return false;
  it = v.emplace(v.begin(), true);
  if (it != v.begin() || !v[0] || v.size() != 109) return false;
  it = v.erase(v.begin());
  if (it != v.begin() || v[0] || v.size() != 108) return false;
  it = v.erase(v.begin(), v.begin() + 4);
  if (it != v.begin() || v.size() != 104) return false;
  v.append_range(std::vector<bool>{false, true});
  if (v.size() != 106 || !v.back()) return false;
  v.resize(110);
  if (v[109] || v.size() != 110) return false;
  v.resize(112, true);
  if (!v[111] || !v[110] || v[109]) return false;
  v.resize(3);
  if (v.size() != 3) return false;
  v.reserve(1000);
  if (v.capacity() < 1000) return false;
  v.shrink_to_fit();
  if (v.capacity() < 3) return false;

  v.assign(4, true);
  if (v.size() != 4 || !v[3]) return false;
  v.assign({false, true});
  if (v.size() != 2 || v[0]) return false;
  v.assign(src, src + 4);
  if (v.size() != 4 || v[1]) return false;
  v.assign_range(std::vector<bool>(7, true));
  if (v.size() != 7 || !v[6]) return false;
  v = {true};
  if (v.size() != 1) return false;
  std::vector<bool> w = v;
  if (w != v || !(w == v)) return false;
  std::vector<bool> x = std::move(w);
  if (x.size() != 1 || !x[0]) return false;
  x.swap(v);
  std::vector<bool> lo{false, true}, hi{true};
  if (!(lo < hi) || (hi <=> lo) <= 0) return false;
  if (std::erase(lo, true) != 1 || lo.size() != 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::vector<bool> v(3);
  const std::vector<bool>& c = v;
  int threw = 0;
  try { (void)v.at(3); } catch (const std::out_of_range&) { ++threw; }
  try { (void)c.at(3); } catch (const std::out_of_range&) { ++threw; }
  CHECK(threw == 2);
  return 0;
}
