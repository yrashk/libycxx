// [sequence.reqmts]/45-48: a.erase(q) has type iterator, erases the element q points to and
// returns an iterator to the element immediately following q prior to the erase, or a.end()
// if there is none. /49-52: a.erase(q1, q2) has type iterator, erases [q1, q2) and returns
// an iterator to the element q2 pointed to prior to the erase, or a.end(); an empty range
// erases nothing. /53-55: a.clear() has type void, destroys all elements, and
// a.empty() is true afterwards. q, q1, q2 are const_iterators.
#include <vector>
#include <string>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  using It = typename X::iterator;
  static_assert(std::is_same_v<decltype(std::declval<X&>().erase(std::declval<X&>().cbegin())), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().erase(std::declval<X&>().cbegin(), std::declval<X&>().cend())), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().clear()), void>);

  X a = make<X>({1, 2, 3, 4, 5, 6});
  It r = a.erase(a.cbegin() + 1);
  if (r != a.begin() + 1 || !(*r == val<T>(3)) || !holds(a, {1, 3, 4, 5, 6})) return false;
  r = a.erase(a.cbegin());
  if (r != a.begin() || !holds(a, {3, 4, 5, 6})) return false;
  r = a.erase(a.cend() - 1);
  if (r != a.end() || !holds(a, {3, 4, 5})) return false;

  a = make<X>({1, 2, 3, 4, 5, 6});
  r = a.erase(a.cbegin() + 1, a.cbegin() + 3);
  if (r != a.begin() + 1 || !(*r == val<T>(4)) || !holds(a, {1, 4, 5, 6})) return false;
  r = a.erase(a.cbegin() + 2, a.cbegin() + 2);
  if (r != a.begin() + 2 || !holds(a, {1, 4, 5, 6})) return false;
  r = a.erase(a.cbegin() + 2, a.cend());
  if (r != a.end() || !holds(a, {1, 4})) return false;
  r = a.erase(a.cbegin(), a.cend());
  if (r != a.end() || r != a.begin() || !a.empty()) return false;

  X e;
  r = e.erase(e.cbegin(), e.cend());
  if (r != e.end()) return false;

  X c = make<X>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23});
  c.clear();
  if (!c.empty() || c.size() != 0 || c.begin() != c.end()) return false;
  c.clear();
  if (!c.empty()) return false;
  c.insert(c.end(), val<T>(1));  // still usable
  return holds(c, {1});
}

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::u8string>());
  return 0;
}
