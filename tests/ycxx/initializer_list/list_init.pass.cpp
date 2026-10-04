// [support.initlist]/1: "An object of type initializer_list<E> provides access to an array of
// objects of type const E." Note 1: "Copying an initializer_list does not copy the underlying
// elements." [support.initlist.access]: end() "Returns: begin() + size()"; size() "The
// number of elements in the array"; data() "Returns: begin()". [dcl.init.list]/5-6: a braced
// list initialises a std::initializer_list<E> parameter from a backing array, and
// [dcl.type.auto.deduct] deduces std::initializer_list<int> for auto x = {1, 2}. All members
// are constexpr ([initializer.list.syn]); usable in constant evaluation.
#include <initializer_list>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

constexpr int sum(std::initializer_list<int> il) {
  int s = 0;
  for (int v : il) s += v;
  return s;
}
static_assert(sum({}) == 0 && sum({1, 2, 3, 4}) == 10);

constexpr bool members() {
  std::initializer_list<int> il = {5, 6, 7};
  if (il.size() != 3 || il.empty() || il.data() != il.begin()) return false;
  if (il.end() != il.begin() + 3 || *il.begin() != 5 || il.begin()[2] != 7) return false;
  std::initializer_list<int> copy = il;  // shares the array
  if (copy.begin() != il.begin() || copy.size() != 3) return false;
  std::initializer_list<int> e = {};
  if (e.size() != 0 || !e.empty() || e.begin() != e.end()) return false;
  std::initializer_list<int> d;
  if (d.size() != 0 || d.begin() != d.end()) return false;
  return true;
}
static_assert(members());

auto deduced = {1, 2};
static_assert(std::is_same_v<decltype(deduced), std::initializer_list<int>>);
static_assert(std::is_same_v<decltype(*deduced.begin()), const int&>);

struct NonTrivial {
  int v;
  constexpr NonTrivial(int x) : v(x) {}
  constexpr NonTrivial(const NonTrivial& o) : v(o.v + 100) {}
};
struct TakesList {
  std::size_t n;
  int first;
  constexpr TakesList(std::initializer_list<NonTrivial> il) : n(il.size()), first(il.begin()->v) {}
};
static_assert(TakesList{1, 2}.n == 2);

int main() {
  CHECK(members());
  CHECK(sum({10, 20}) == 30);
  std::initializer_list<const char*> words = {"a", "bc"};
  CHECK(words.size() == 2 && words.begin()[1][1] == 'c');
  // a list of lists
  std::initializer_list<std::initializer_list<int>> nested = {{1}, {2, 3}, {}};
  std::size_t total = 0;
  for (auto inner : nested) total += inner.size();
  CHECK(nested.size() == 3 && total == 3);
  TakesList t{NonTrivial(4)};
  CHECK(t.n == 1);
  return 0;
}
