// [hive.operations]: splice(x) moves the elements of x into *this (x becomes empty;
// pointers, references and iterators to them now refer to elements of *this); unique
// erases all but the first of each consecutive group of equivalent elements, returns the
// number erased and calls the predicate exactly size() - 1 times; sort orders the elements
// by comp (less<> by default). [hive.erasure]: erase(c, value) / erase_if(c, pred) erase the
// matching elements and return their number.
#include <hive>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>
#include "check.hpp"

int main() {
  std::hive<int> a{1, 2, 3};
  std::hive<int> b{4, 5};
  int* p4 = nullptr;
  for (auto& x : b)
    if (x == 4) p4 = &x;
  auto it4 = b.get_iterator(p4);
  a.splice(b);
  CHECK(a.size() == 5 && b.empty() && b.size() == 0 && *it4 == 4 && &*it4 == p4);
  CHECK(a.get_iterator(p4) == it4);
  int n = 0;
  for (auto it = a.begin(); it != a.end(); ++it) n += it == it4 ? 1 : 0;
  CHECK(n == 1);  // it4 now walks a
  a.splice(std::hive<int>{6});
  CHECK(a.size() == 6);

  std::hive<int> s{5, 3, 9, 1, 3, 7, 1};
  s.sort();
  CHECK(std::is_sorted(s.begin(), s.end()) && s.size() == 7);
  s.sort(std::greater<int>());
  CHECK(std::is_sorted(s.begin(), s.end(), std::greater<int>()));
  s.sort();
  int calls = 0;
  auto erased = s.unique([&](int x, int y) {
    ++calls;
    return x == y;
  });
  static_assert(std::is_same_v<decltype(erased), std::hive<int>::size_type>);
  CHECK(erased == 2 && calls == 6 && s.size() == 5);
  std::vector<int> v(s.begin(), s.end());
  CHECK(v == std::vector<int>({1, 3, 5, 7, 9}));
  CHECK(s.unique() == 0);
  std::hive<int> e;
  calls = 0;
  CHECK(e.unique([&](int, int) { return ++calls, true; }) == 0 && calls == 0);

  std::hive<int> r{1, 2, 3, 2, 5, 2};
  CHECK(std::erase(r, 2) == 3 && r.size() == 3);
  CHECK(std::erase_if(r, [](int x) { return x > 2; }) == 2 && r.size() == 1 && *r.begin() == 1);
  return 0;
}
