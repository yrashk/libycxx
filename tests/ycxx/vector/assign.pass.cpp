// [container.reqmts]/17-23: t = v (copy) and t = rv (move) give t == v / the value rv had.
// [sequence.reqmts]/16-19: a = il; /57-59: a.assign(i, j); /60-63: a.assign_range(rg);
// /65: a.assign(il); /66-68: a.assign(n, t). Each iterator of the source range is
// dereferenced exactly once. [vector.overview]: operator= returns vector&; assign returns
// void.
#include <vector>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::declval<std::vector<int>&>() = {1}), std::vector<int>&>);
static_assert(std::is_same_v<decltype(std::declval<std::vector<int>&>().assign(1, 1)), void>);
static_assert(std::is_same_v<decltype(std::declval<std::vector<int>&>().assign_range(std::declval<int(&)[2]>())),
                             void>);
// [sequence.reqmts]/61: assign_range Mandates assignable_from<T&, range_reference_t<R>>; a
// range of a type that does not even convert is rejected by the container-compatible-range
// constraint.
struct Unrelated {};
template <class R>
concept can_assign_range = requires(std::vector<int>& v, R&& r) { v.assign_range(r); };
static_assert(can_assign_range<long (&)[2]>);
static_assert(!can_assign_range<Unrelated (&)[2]>);

constexpr bool test() {
  std::vector<int> v{1, 2, 3};
  std::vector<int> w{9};
  if (&(w = v) != &w || w != v) return false;
  w = w;
  if (w != v) return false;
  std::vector<int> big(100, 5);
  w = big;  // grow
  if (w != big) return false;
  w = v;  // shrink
  if (w != v) return false;
  std::vector<int> m{4, 5};
  w = std::move(m);
  if (w.size() != 2 || w[1] != 5) return false;
  w = {7, 8, 9, 10};
  if (w.size() != 4 || w[3] != 10) return false;
  w = {};
  if (!w.empty()) return false;

  int a[] = {11, 12, 13, 14, 15};
  int derefs = 0;
  w.assign(InputIter<int>(a, &derefs), InputIter<int>(a + 5, &derefs));
  if (w.size() != 5 || w[4] != 15 || derefs != 5) return false;
  derefs = 0;
  w.assign(ForwardIter<int>(a, &derefs), ForwardIter<int>(a + 2, &derefs));
  if (w.size() != 2 || w[1] != 12 || derefs != 2) return false;
  w.assign(3, 42);
  if (w.size() != 3 || w[0] != 42 || w[2] != 42) return false;
  w.assign(0, 1);
  if (!w.empty()) return false;
  w.assign({1, 2});
  if (w.size() != 2 || w[1] != 2) return false;
  derefs = 0;
  w.assign_range(InputRange<int>{a, a + 4, &derefs});
  if (w.size() != 4 || w[3] != 14 || derefs != 4) return false;
  w.assign_range(ForwardRange<int>{a + 3, a + 5});
  if (w.size() != 2 || w[0] != 14) return false;
  long longs[] = {100, 200};
  w.assign_range(longs);
  if (w.size() != 2 || w[1] != 200) return false;
  w.assign(longs, longs + 1);
  if (w.size() != 1 || w[0] != 100) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
