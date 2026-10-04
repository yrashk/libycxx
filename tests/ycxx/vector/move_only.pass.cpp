// A vector of a move-only type supports the operations whose preconditions only require
// Cpp17MoveInsertable / Cpp17MoveAssignable / Cpp17EmplaceConstructible
// ([sequence.reqmts]: emplace, insert(p, rv), erase; [vector.capacity]: reserve,
// shrink_to_fit, resize(sz); emplace_back / push_back(rv); move construction / assignment
// and swap).
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

using P = std::unique_ptr<int>;
static_assert(!std::is_copy_constructible_v<P>);
static_assert(std::is_move_constructible_v<std::vector<P>>);

int main() {
  std::vector<P> v;
  for (int i = 0; i < 20; ++i) v.push_back(std::make_unique<int>(i));
  v.emplace_back(new int(20));
  v.insert(v.begin(), std::make_unique<int>(-1));
  v.emplace(v.begin() + 5, new int(100));
  CHECK(v.size() == 23 && *v[0] == -1 && *v[5] == 100 && *v[22] == 20);
  v.erase(v.begin() + 5);
  v.erase(v.begin(), v.begin() + 1);
  CHECK(v.size() == 21 && *v[0] == 0 && *v[20] == 20);
  v.reserve(100);
  v.shrink_to_fit();
  v.resize(25);
  CHECK(v.size() == 25 && v[24] == nullptr && *v[20] == 20);
  v.resize(10);
  std::vector<P> w = std::move(v);
  CHECK(w.size() == 10 && *w[9] == 9);
  v = std::move(w);
  CHECK(v.size() == 10);
  std::vector<P> z;
  z.swap(v);
  CHECK(z.size() == 10 && v.empty());
  z.pop_back();
  CHECK(z.size() == 9 && *z.back() == 8);
  return 0;
}
