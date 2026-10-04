// [sequence.reqmts]/84-88: emplace_back(args) appends T(std::forward<Args>(args)...) and
// returns a.back() (reference); /101-108 push_back(t) / push_back(rv); /109-111
// append_range(rg) (each element dereferenced exactly once); /117-119 pop_back destroys the
// last element. Appending an element of the vector itself works even when the vector has
// to reallocate.
#include <vector>
#include <memory>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::declval<std::vector<int>&>().emplace_back()), int&>);
static_assert(std::is_same_v<decltype(std::declval<std::vector<int>&>().push_back(1)), void>);
static_assert(std::is_same_v<decltype(std::declval<std::vector<int>&>().pop_back()), void>);
static_assert(std::is_same_v<decltype(std::declval<std::vector<int>&>().append_range(std::declval<int(&)[1]>())),
                             void>);

struct Two {
  int a;
  std::string b;
  constexpr Two(int x, std::string y) : a(x), b(std::move(y)) {}
};

constexpr bool test() {
  std::vector<int> v;
  int& r = v.emplace_back(4);
  if (&r != &v.back() || r != 4) return false;
  v.emplace_back();
  if (v.back() != 0) return false;
  const int x = 7;
  v.push_back(x);
  v.push_back(8);
  if (v != std::vector<int>{4, 0, 7, 8}) return false;
  int a[] = {1, 2, 3};
  int derefs = 0;
  v.append_range(InputRange<int>{a, a + 3, &derefs});
  if (v.size() != 7 || v.back() != 3 || derefs != 3) return false;
  v.append_range(std::views::iota(10, 12));
  if (v.size() != 9 || v.back() != 11) return false;
  v.pop_back();
  v.pop_back();
  if (v.size() != 7 || v.back() != 3) return false;

  std::vector<Two> t;
  Two& tr = t.emplace_back(1, "one");
  if (tr.a != 1 || tr.b != "one") return false;

  // push_back / emplace_back of an element of the vector itself, at full capacity.
  std::vector<std::string> s{"first", "second-long-enough-to-allocate-on-the-heap"};
  s.shrink_to_fit();
  for (int i = 0; i < 20; ++i) {
    s.push_back(s[1]);
    s.emplace_back(s[0]);
  }
  for (std::size_t i = 2; i < s.size(); i += 2)
    if (s[i] != s[1] || s[i + 1] != "first") return false;
  std::vector<int> g{5};
  for (int i = 0; i < 40; ++i) g.push_back(g.back() + 1);
  if (g.size() != 41 || g[40] != 45) return false;
  for (int i = 0; i < 40; ++i) g.push_back(std::move(g[0]));
  if (g.back() != 5) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::vector<std::unique_ptr<int>> u;
  u.push_back(std::make_unique<int>(1));
  u.emplace_back(new int(2));
  CHECK(u.size() == 2 && *u[1] == 2);
  return 0;
}
