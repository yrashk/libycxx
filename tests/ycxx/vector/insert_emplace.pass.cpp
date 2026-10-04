// [sequence.reqmts]/20-44: emplace(p, args), insert(p, t), insert(p, rv), insert(p, n, t),
// insert(p, i, j), insert_range(p, rg), insert(p, il) insert before p and return an
// iterator to the first new element (or p if nothing was inserted). /22 note: args may
// refer to an element of the vector. [vector.modifiers]/2: without reallocation, iterators
// before the insertion point stay valid.
#include <vector>
#include <ranges>
#include <string>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pair {
  int a;
  int b;
  constexpr Pair(int x, int y) : a(x), b(y) {}
  constexpr bool operator==(const Pair&) const = default;
};

constexpr bool test() {
  std::vector<int> v{1, 5};
  auto it = v.insert(v.cbegin() + 1, 3);
  if (v != std::vector<int>{1, 3, 5} || it != v.begin() + 1) return false;
  int four = 4;
  it = v.insert(v.cbegin() + 2, four);
  if (v != std::vector<int>{1, 3, 4, 5} || *it != 4) return false;
  it = v.insert(v.cend(), 2, 9);
  if (v != std::vector<int>{1, 3, 4, 5, 9, 9} || it != v.begin() + 4) return false;
  it = v.insert(v.cbegin() + 1, 0, 7);
  if (it != v.begin() + 1 || v.size() != 6) return false;
  int a[] = {-1, -2};
  it = v.insert(v.cbegin(), InputIter<int>(a), InputIter<int>(a + 2));
  if (v != std::vector<int>{-1, -2, 1, 3, 4, 5, 9, 9} || it != v.begin()) return false;
  it = v.insert(v.cbegin() + 3, a, a);
  if (it != v.begin() + 3) return false;
  it = v.insert_range(v.cend(), InputRange<int>{a, a + 1});
  if (v.back() != -1 || it != v.end() - 1) return false;
  it = v.insert_range(v.cbegin() + 1, std::views::iota(100, 102));
  if (v[1] != 100 || v[2] != 101 || v[3] != -2 || it != v.begin() + 1) return false;
  it = v.insert_range(v.cbegin(), std::views::empty<int>);
  if (it != v.begin()) return false;
  it = v.insert(v.cbegin() + 2, {50, 51});
  if (v[2] != 50 || v[3] != 51 || v[4] != 101 || it != v.begin() + 2) return false;

  std::vector<Pair> p;
  p.emplace(p.cbegin(), 1, 2);
  auto pit = p.emplace(p.cbegin(), 0, 0);
  if (p.size() != 2 || pit != p.begin() || !(p[1] == Pair(1, 2))) return false;
  pit = p.emplace(p.cend(), 5, 6);
  if (!(*pit == Pair(5, 6)) || pit != p.end() - 1) return false;

  // Arguments referring to elements of the vector itself.
  for (int round = 0; round < 2; ++round) {
    std::vector<std::string> s{"aa", "bb", "cc"};
    if (round == 1) s.reserve(10);  // exercise both the reallocating and in-place paths
    s.insert(s.begin(), s[2]);
    if (s[0] != "cc" || s[3] != "cc" || s.size() != 4) return false;
    s.insert(s.begin() + 1, 2, s[1]);
    if (s[1] != "aa" || s[2] != "aa" || s[3] != "aa" || s[4] != "bb") return false;
    s.emplace(s.begin(), s.back());
    if (s[0] != "cc" || s.size() != 7) return false;
    s.emplace(s.begin() + 1, std::move(s[0]));
    if (s[1] != "cc" || s.size() != 8) return false;
  }

  // Without reallocation, elements before the insertion point keep their addresses.
  std::vector<int> r;
  r.reserve(10);
  r = {1, 2, 3};
  int* before = &r[0];
  r.insert(r.begin() + 1, 9);
  if (&r[0] != before || r[0] != 1 || r[1] != 9 || r[3] != 3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
