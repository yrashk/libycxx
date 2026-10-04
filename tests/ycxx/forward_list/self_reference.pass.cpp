// forward_list members whose value argument refers to an element of the same list.
// [forward.list.modifiers]: insert_after(p, x) "Inserts a copy of x after p",
// insert_after(p, n, x) "Inserts n copies of x after p", emplace_after/emplace_front construct
// from args, push_front inserts a copy; none has a precondition excluding x referring into the
// list. [forward.list.modifiers]: resize(sz, c) "inserts sz - distance(begin(), end()) copies
// of c at the end of the list". [forward.list.ops]: remove(value) "Erases all the elements in
// the list referred to by a list iterator i for which ... *i == value": the comparison
// continues after the element value refers to has been erased.
#include <forward_list>
#include <iterator>
#include <string>
#include "check.hpp"

template <class T>
bool test(const T& a, const T& b, const T& c) {
  using L = std::forward_list<T>;
  L l{a, b, c};
  l.insert_after(l.cbegin(), *std::next(l.begin(), 2));
  if (l != L{a, c, b, c}) return false;
  l.insert_after(std::next(l.cbegin(), 2), 2, l.front());
  if (l != L{a, c, b, a, a, c}) return false;
  l.emplace_after(l.cbegin(), *std::next(l.begin(), 2));
  if (l != L{a, b, c, b, a, a, c}) return false;
  l.push_front(*std::next(l.begin(), 6));
  l.emplace_front(*std::next(l.begin(), 2));
  if (l != L{b, c, a, b, c, b, a, a, c}) return false;
  l.resize(11, *std::next(l.begin(), 1));
  if (l != L{b, c, a, b, c, b, a, a, c, c, c}) return false;
  l.resize(13, l.front());
  if (l != L{b, c, a, b, c, b, a, a, c, c, c, b, b}) return false;
  l.insert_after(l.cbefore_begin(), 3, *std::next(l.begin(), 10));
  if (l != L{c, c, c, b, c, a, b, c, b, a, a, c, c, c, b, b}) return false;

  L r{a, b, a, c, a};
  if (r.remove(r.front()) != 3 || r != L{b, c}) return false;
  r = L{b, a, c, a, b, a};
  if (r.remove(*std::next(r.begin(), 1)) != 3 || r != L{b, c, b}) return false;
  r = L{c, a, b, a, c};
  if (r.remove(*std::next(r.begin(), 4)) != 2 || r != L{a, b, a}) return false;
  r = L{c, a, b, a, c};
  if (r.remove(*std::next(r.begin(), 3)) != 2 || r != L{c, b, c}) return false;
  return true;
}

int main() {
  CHECK(test(1, 2, 3));
  CHECK(test<std::string>("first string, longer than a small buffer", "second string, longer than a small buffer",
                          "third string, longer than a small buffer"));
  return 0;
}
