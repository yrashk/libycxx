// [vector.data]/1: data() returns "A pointer such that [data(), data() + size()) is a valid
// range. For a non-empty vector, data() == addressof(front()) is true." Both overloads are
// noexcept ([vector.overview]); non-const data() returns T* and writes through it are writes
// to the elements; const data() returns const T*. For an empty vector the range
// [data(), data()) is valid (possibly null), also after reserve() and after clear().
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::declval<std::vector<int>&>().data()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const std::vector<int>&>().data()), const int*>);
static_assert(noexcept(std::declval<const std::vector<int>&>().data()));

constexpr bool test() {
  std::vector<int> e;
  if (e.data() + e.size() != e.data()) return false;
  for (const int* p = e.data(); p != e.data() + e.size(); ++p) return false;
  e.reserve(10);
  if (e.data() + e.size() != e.data()) return false;
  std::vector<int> v{1, 2, 3};
  int* p = v.data();
  if (p != std::addressof(v.front()) || p + 2 != std::addressof(v.back())) return false;
  p[1] = 20;
  if (v[1] != 20) return false;
  const std::vector<int>& cv = v;
  if (cv.data() != p || cv.data()[1] != 20) return false;
  v.clear();
  if (v.data() + v.size() != v.data()) return false;
  v.push_back(5);
  if (v.data() != std::addressof(v.front()) || *v.data() != 5) return false;
  std::vector<int> moved_from{1, 2};
  std::vector<int> to(std::move(moved_from));
  (void)moved_from.data();  // valid object
  return to.data()[1] == 2;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
