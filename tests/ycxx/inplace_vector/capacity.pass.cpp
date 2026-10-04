// [inplace.vector.overview]/4: "Any member function of inplace_vector<T, N> that would cause
// the size to exceed N throws an exception of type bad_alloc." [inplace.vector.capacity]:
// capacity() and max_size() are static and return N; reserve(n) has no effect and throws
// bad_alloc if n > capacity(); shrink_to_fit() has no effect; resize has no effects on
// *this if it throws. [inplace.vector.modifiers]/7: push_back / emplace_back have no
// effects if they throw; /3: insertion that throws other than from T's copy / move
// operations or an InputIterator operation has no effects.
#include <inplace_vector>
#include <cstddef>
#include <new>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

using IV = std::inplace_vector<int, 4>;
static_assert(IV::capacity() == 4 && IV::max_size() == 4);
static_assert(std::inplace_vector<int, 0>::capacity() == 0);

template <class F>
bool throws_bad_alloc(F f) {
  try {
    f();
  } catch (const std::bad_alloc&) {
    return true;
  }
  return false;
}

bool same(const IV& v, std::initializer_list<int> want) {
  if (v.size() != want.size()) return false;
  auto w = want.begin();
  for (int x : v)
    if (x != *w++) return false;
  return true;
}

int main() {
  int five[] = {1, 2, 3, 4, 5};
  CHECK(throws_bad_alloc([] { IV v(5); }));
  CHECK(throws_bad_alloc([] { IV v(5, 1); }));
  CHECK(throws_bad_alloc([&] { IV v(five, five + 5); }));
  CHECK(throws_bad_alloc([&] { IV v(InputIter<int>(five), InputIter<int>(five + 5)); }));
  CHECK(throws_bad_alloc([&] { IV v(std::from_range, InputRange<int>{five, five + 5}); }));
  CHECK(throws_bad_alloc([] { IV v{1, 2, 3, 4, 5}; }));
  CHECK(throws_bad_alloc([] { IV::reserve(5); }));
  IV::reserve(4);
  IV::reserve(0);

  IV v{1, 2, 3};
  v.shrink_to_fit();
  CHECK(same(v, {1, 2, 3}));
  v.push_back(4);
  CHECK(throws_bad_alloc([&] { v.push_back(5); }) && same(v, {1, 2, 3, 4}));
  CHECK(throws_bad_alloc([&] { v.emplace_back(5); }) && same(v, {1, 2, 3, 4}));
  const int c = 6;
  CHECK(throws_bad_alloc([&] { v.push_back(c); }) && same(v, {1, 2, 3, 4}));
  CHECK(throws_bad_alloc([&] { v.insert(v.begin(), 0); }) && same(v, {1, 2, 3, 4}));
  CHECK(throws_bad_alloc([&] { v.emplace(v.begin() + 1, 0); }) && same(v, {1, 2, 3, 4}));
  CHECK(throws_bad_alloc([&] { v.resize(5); }) && same(v, {1, 2, 3, 4}));
  CHECK(throws_bad_alloc([&] { v.resize(9, 7); }) && same(v, {1, 2, 3, 4}));
  v.resize(2);
  CHECK(same(v, {1, 2}));
  CHECK(throws_bad_alloc([&] { v.insert(v.begin() + 1, std::size_t(3), 9); }) && same(v, {1, 2}));
  CHECK(throws_bad_alloc([&] { v.insert(v.begin(), five, five + 3); }) && same(v, {1, 2}));
  CHECK(throws_bad_alloc([&] { v.insert(v.end(), {7, 8, 9}); }) && same(v, {1, 2}));
  CHECK(throws_bad_alloc([&] { v.insert_range(v.begin() + 1, ForwardRange<int>{five, five + 3}); }) && same(v, {1, 2}));
  CHECK(throws_bad_alloc([&] { v.append_range(five); }) && same(v, {1, 2}));
  CHECK(throws_bad_alloc([&] { v.assign(5, 0); }));
  CHECK(throws_bad_alloc([&] { v.assign({1, 2, 3, 4, 5}); }));
  CHECK(throws_bad_alloc([&] { v.assign_range(InputRange<int>{five, five + 5}); }));
  CHECK(throws_bad_alloc([&] { v = {1, 2, 3, 4, 5}; }));
  // filling exactly to capacity is fine
  IV w;
  w.append_range(InputRange<int>{five, five + 4});
  CHECK(same(w, {1, 2, 3, 4}) && w.size() == w.capacity());
  return 0;
}
