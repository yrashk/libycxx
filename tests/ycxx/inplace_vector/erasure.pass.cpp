// [inplace.vector.erasure]: erase(c, value) / erase_if(c, pred) are remove / remove_if
// followed by erase(it, end()), returning the count (size_t); U defaults to T.
// [inplace.vector.modifiers]/19-21: erase calls the destructor once per erased element and
// the assignment operator exactly once per element after the erased ones; [inplace.vector.data]:
// data() == addressof(front()) for a non-empty inplace_vector.
#include <inplace_vector>
#include <cstddef>
#include <memory>
#include <type_traits>
#include "check.hpp"

struct P {
  int a, b;
  constexpr bool operator==(const P&) const = default;
};

constexpr bool erasure() {
  std::inplace_vector<int, 20> v{1, 2, 3, 2, 5, 2, 7};
  static_assert(std::is_same_v<decltype(std::erase(v, 2)), std::size_t>);
  if (std::erase(v, 2) != 3 || v.size() != 4 || v[1] != 3) return false;
  if (std::erase_if(v, [](int x) { return x > 4; }) != 2 || v.size() != 2) return false;
  std::inplace_vector<P, 4> ps{{1, 2}, {3, 4}};
  if (std::erase(ps, {1, 2}) != 1 || !(ps[0] == P{3, 4})) return false;
  return v.data() == std::addressof(v.front());
}

struct Counts {
  int dtors = 0, assigns = 0;
};
inline Counts counts;
struct T {
  int v = 0;
  T(int x) : v(x) {}
  T(const T&) = default;
  T& operator=(const T& o) { v = o.v; ++counts.assigns; return *this; }
  T& operator=(T&& o) noexcept { v = o.v; ++counts.assigns; return *this; }
  ~T() { ++counts.dtors; }
};

bool erase_counts() {
  for (int first = 0; first < 10; ++first) {
    for (int len = 1; first + len <= 10; ++len) {
      std::inplace_vector<T, 10> v;
      for (int i = 0; i < 10; ++i) v.emplace_back(i);
      counts = {};
      v.erase(v.begin() + first, v.begin() + first + len);
      if (counts.dtors != len || counts.assigns != 10 - first - len) return false;
    }
  }
  std::inplace_vector<T, 4> v;
  v.emplace_back(1);
  v.emplace_back(2);
  counts = {};
  v.pop_back();
  return counts.dtors == 1 && counts.assigns == 0;
}

static_assert(erasure());

int main() {
  CHECK(erasure());
  CHECK(erase_counts());
  return 0;
}
