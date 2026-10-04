// [alg.fill]: fill assigns value through every iterator in [first, last); fill_n assigns
// through first + i for 0 <= i < n and returns first + n for n > 0, first otherwise.
// ranges::fill returns last; ranges::fill_n returns first + n. [alg.generate]: generate /
// generate_n assign the result of successive gen() calls (in order); generate_n returns
// first + max(0, n); ranges::generate(_n) likewise, with gen invoked via invoke. C++26:
// fill's T defaults to the value type, so braced values work.
#include <algorithm>
#include <ranges>
#include "check.hpp"

struct Pt {
  int x, y;
};

constexpr bool test() {
  int a[5] = {};
  std::fill(a, a + 5, 7);
  for (int v : a)
    if (v != 7) return false;
  if (std::fill_n(a, 2, 1) != a + 2 || a[1] != 1 || a[2] != 7) return false;
  if (std::fill_n(a, 0, 3) != a || std::fill_n(a, -4, 3) != a || a[0] != 1) return false;

  int* rl = std::ranges::fill(a, 4);
  if (rl != a + 5 || a[4] != 4) return false;
  if (std::ranges::fill_n(a, 3, 8) != a + 3 || a[2] != 8 || a[3] != 4) return false;
  if (std::ranges::fill(a + 1, a + 2, 0) != a + 2 || a[1] != 0) return false;

  Pt ps[2] = {};
  std::ranges::fill(ps, {3, 4});
  if (ps[1].x != 3 || ps[1].y != 4) return false;
  std::fill(ps, ps + 2, {5, 6});
  if (ps[0].y != 6) return false;

  int g[4] = {};
  int n = 0;
  std::generate(g, g + 4, [&n] { return n++; });
  if (g[0] != 0 || g[3] != 3) return false;
  if (std::generate_n(g, 2, [&n] { return n++; }) != g + 2 || g[0] != 4 || g[1] != 5 || g[2] != 2) return false;
  if (std::generate_n(g, -1, [&n] { return n++; }) != g || n != 6) return false;

  int* rg = std::ranges::generate(g, [&n] { return n++ * 10; });
  if (rg != g + 4 || g[0] != 60 || g[3] != 90) return false;
  if (std::ranges::generate_n(g, 1, [] { return -1; }) != g + 1 || g[0] != -1) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
