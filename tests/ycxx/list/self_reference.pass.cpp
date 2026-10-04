// list members whose value argument refers to an element of the same list.
// [sequence.reqmts]: insert(p, t), insert(p, n, t) and the front/back insertions have no
// precondition excluding t referring into a (only assign(n, t) does, /67); /22 Note 1: emplace's
// args "can directly or indirectly refer to a value in a". [list.capacity]: resize(sz, c)
// appends sz - size() copies of c. [list.ops]/15-17: remove(value) "Erases all the elements in
// the list referred to by a list iterator i for which ... *i == value", so value is compared
// against every element even after the element it refers to has been erased.
#include <list>
#include <iterator>
#include <string>
#include "check.hpp"

template <class T>
bool test(const T& a, const T& b, const T& c) {
  using L = std::list<T>;
  L l{a, b, c};
  l.insert(std::next(l.cbegin()), l.back());
  if (l != L{a, c, b, c}) return false;
  l.insert(l.cbegin(), 2, *std::next(l.begin(), 2));
  if (l != L{b, b, a, c, b, c}) return false;
  l.emplace(l.cend(), l.front());
  if (l != L{b, b, a, c, b, c, b}) return false;
  l.push_front(l.back());
  l.push_back(*std::next(l.begin(), 3));
  if (l != L{b, b, b, a, c, b, c, b, a}) return false;
  l.emplace_front(*std::next(l.begin(), 4));
  l.emplace_back(l.front());
  if (l != L{c, b, b, b, a, c, b, c, b, a, c}) return false;
  l.resize(14, l.back());
  if (l != L{c, b, b, b, a, c, b, c, b, a, c, c, c, c}) return false;

  // remove with the value aliasing the first, a middle and the last matching element
  L r{a, b, a, c, a};
  if (r.remove(r.front()) != 3 || r != L{b, c}) return false;
  r = L{b, a, c, a, b, a};
  if (r.remove(*std::next(r.begin(), 3)) != 3 || r != L{b, c, b}) return false;
  r = L{c, a, b, a, c};
  if (r.remove(r.back()) != 2 || r != L{a, b, a}) return false;
  return true;
}

int main() {
  CHECK(test(1, 2, 3));
  CHECK(test<std::string>("first string, longer than a small buffer", "second string, longer than a small buffer",
                          "third string, longer than a small buffer"));
  return 0;
}
