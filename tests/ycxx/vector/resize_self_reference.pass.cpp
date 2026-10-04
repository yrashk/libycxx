// [vector.capacity]/17-18: resize(sz, c): "If sz < size(), erases the last size() - sz
// elements from the sequence. Otherwise, appends sz - size() copies of c to the sequence."
// There is no precondition that c does not refer to an element of the vector, so the
// appended copies have the value c had before the call, also when growing reallocates the
// storage c lives in. /19: "If an exception is thrown, there are no effects."
// Also push_back(const T&) with an element of the vector at full capacity ([sequence.reqmts]
// /101-103 "Appends a copy of t").
#include <vector>
#include <string>
#include "check.hpp"

template <class T>
constexpr bool test(T a, T b) {
  using V = std::vector<T>;
  V v{a, b};
  v.shrink_to_fit();
  v.resize(6, v[0]);  // must reallocate
  if (v != V{a, b, a, a, a, a}) return false;
  v.resize(v.capacity() + 3, v.back());
  if (v.size() < 9 || !(v.back() == a) || !(v[1] == b)) return false;
  V w{a, b};
  w.reserve(50);
  w.resize(5, w[1]);  // no reallocation
  if (w != V{a, b, b, b, b}) return false;
  w.resize(2, w[0]);  // shrinking ignores c
  if (w != V{a, b}) return false;
  w.resize(2, w[1]);
  if (w != V{a, b}) return false;
  V p{a};
  p.shrink_to_fit();
  for (int i = 0; i < 20; ++i) p.push_back(p[0]);
  if (p.size() != 21 || !(p[20] == a)) return false;
  return true;
}

static_assert(test<int>(5, 6));
static_assert(test<std::string>("an element longer than any small buffer would be", "b"));

int main() {
  CHECK(test<int>(5, 6));
  CHECK(test<double>(0.5, 1.5));
  CHECK(test<std::string>("an element longer than any small buffer would be", "b"));
  CHECK(test<std::vector<int>>({1, 2, 3}, {4}));
  return 0;
}
