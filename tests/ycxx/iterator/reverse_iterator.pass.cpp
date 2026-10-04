// [reverse.iterator], [reverse.iter.elem], [reverse.iter.nav], [reverse.iter.nonmember]:
// operator* is "Iterator tmp = current; return *--tmp;"; operator-> returns prev(current) for
// pointers and prev(current).operator->() otherwise; operator[](n) returns current[-n - 1];
// n + x is reverse_iterator(x.base() - n); x - y is y.base() - x.base();
// make_reverse_iterator(i) is reverse_iterator<Iterator>(i). All constexpr.
#include <iterator>
#include <array>
#include <type_traits>
#include "check.hpp"

struct P {
  int v;
};

constexpr bool test() {
  int a[5] = {1, 2, 3, 4, 5};
  std::reverse_iterator<int*> r(a + 5), e(a);
  if (*r != 5 || r.base() != a + 5) return false;
  if (r[0] != 5 || r[1] != 4 || r[4] != 1) return false;
  if (e - r != 5 || r - e != -5) return false;
  std::reverse_iterator<int*> r2 = r + 2;
  if (*r2 != 3 || r2.base() != a + 3) return false;
  if (*(2 + r) != 3) return false;
  if (*(r2 - 1) != 4) return false;
  ++r2;
  if (*r2 != 2) return false;
  auto old = r2++;
  if (*old != 2 || *r2 != 1) return false;
  --r2;
  if (*r2 != 2) return false;
  old = r2--;
  if (*old != 2 || *r2 != 3) return false;
  r2 += 2;
  if (*r2 != 1) return false;
  r2 -= 4;
  if (*r2 != 5) return false;
  int sum = 0;
  for (auto it = r; it != e; ++it) sum = sum * 10 + *it;
  if (sum != 54321) return false;

  P ps[2] = {{7}, {8}};
  std::reverse_iterator<P*> rp(ps + 2);
  if (rp->v != 8 || rp.operator->() != ps + 1) return false;
  std::array<P, 2> arr{{{3}, {4}}};
  auto ra = std::make_reverse_iterator(arr.end());
  static_assert(std::is_same_v<decltype(ra), std::reverse_iterator<std::array<P, 2>::iterator>>);
  if (ra->v != 4) return false;

  std::reverse_iterator<int*> def;
  std::reverse_iterator<int*> def2{};
  if (def.base() != nullptr || def2.base() != nullptr) return false;
  // converting construction and assignment from reverse_iterator<int*> to <const int*>
  std::reverse_iterator<const int*> cr(r);
  if (cr.base() != a + 5) return false;
  cr = e;
  if (cr.base() != a) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
