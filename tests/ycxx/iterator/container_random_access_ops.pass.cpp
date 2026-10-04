// The operational semantics of [random.access.iterators] (Cpp17RandomAccessIterator, which
// the vector and basic_string iterators meet, [container.reqmts]/68), together with
// [bidirectional.iterators] and [forward.iterators]: r += n / r -= n return r& and move by n;
// a + n == n + a; a - n; b - a == n when b == a + n; a[n] is *(a + n); ++r / --r return r&,
// r++ / r-- return the old value; a->m is (*a).m; < is a total order consistent with -.
// Exercised on vector, vector<bool> (through its proxy reference) and basic_string, in
// constant expressions as well ([vector.overview]/3, [basic.string.general]/5: constexpr
// iterators).
#include <vector>
#include <string>
#include <iterator>
#include <type_traits>
#include "check.hpp"

struct P {
  int x;
  int y;
};

template <class X>
constexpr bool test(X a) {  // a holds at least 6 elements
  using It = typename X::iterator;
  using D = typename X::difference_type;
  It b = a.begin();
  It r = b;
  static_assert(std::is_same_v<decltype(r += D(1)), It&>);
  static_assert(std::is_same_v<decltype(r -= D(1)), It&>);
  static_assert(std::is_same_v<decltype(++r), It&>);
  static_assert(std::is_same_v<decltype(--r), It&>);
  static_assert(std::is_same_v<decltype(b + D(1)), It>);
  static_assert(std::is_same_v<decltype(D(1) + b), It>);
  static_assert(std::is_same_v<decltype(b - D(1)), It>);
  static_assert(std::is_same_v<decltype(b - b), D>);
  if (&(r += 3) != &r || r != b + 3) return false;
  if (&(r -= 2) != &r || r != b + 1) return false;
  if (b + 4 != D(4) + b || (b + 4) - 4 != b) return false;
  if ((b + 5) - b != 5 || b - (b + 5) != -5) return false;
  if (r += 0, r != b + 1) return false;
  if (r += -1, r != b) return false;
  for (D n = 0; n < 6; ++n)
    if (!(b[n] == *(b + n))) return false;
  It i = b + 2;
  It old = i++;
  if (old != b + 2 || i != b + 3) return false;
  old = i--;
  if (old != b + 3 || i != b + 2) return false;
  if (&++i != &i || i != b + 3 || &--i != &i || i != b + 2) return false;
  // total order consistent with the difference
  for (D m = 0; m < 6; ++m)
    for (D n = 0; n < 6; ++n) {
      It x = b + m, y = b + n;
      if ((x < y) != (y - x > 0) || (x == y) != (m == n) || (x >= y) != !(x < y)) return false;
    }
  // writing through b[n]
  b[1] = b[0];
  if (!(a[1] == a[0])) return false;
  It e = a.end();
  if (e - b != static_cast<D>(a.size()) || *(e - 1) != a.back()) return false;
  return true;
}

constexpr bool arrow() {
  std::vector<P> v{{1, 2}, {3, 4}};
  auto it = v.begin() + 1;
  if (it->x != 3 || (*it).y != 4) return false;
  it->y = 40;
  auto cit = v.cbegin();
  return v[1].y == 40 && cit->x == 1;
}

static_assert(test(std::vector<int>{1, 2, 3, 4, 5, 6}));
static_assert(test(std::vector<bool>{true, false, false, true, true, false}));
static_assert(test(std::string("abcdef")));
static_assert(arrow());

int main() {
  CHECK(test(std::vector<int>{1, 2, 3, 4, 5, 6}));
  CHECK(test(std::vector<bool>{true, false, false, true, true, false}));
  CHECK(test(std::vector<std::string>{"a", "b", "c", "d", "e", "f"}));
  CHECK(test(std::string("abcdefghijklmnopqrstuvwxyz0123456789")));
  CHECK(test(std::wstring(L"abcdef")));
  CHECK(arrow());
  return 0;
}
