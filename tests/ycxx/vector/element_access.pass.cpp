// [sequence.reqmts]/71-77, 121-128: front(), back(), operator[] and at() (at throws
// out_of_range if n >= size()), with const_reference results on a const vector.
// [vector.data]/1: [data(), data() + size()) is a valid range and, for a non-empty vector,
// data() == addressof(front()).
#include <vector>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

using V = std::vector<int>;
static_assert(std::is_same_v<decltype(std::declval<V&>()[0]), int&>);
static_assert(std::is_same_v<decltype(std::declval<const V&>()[0]), const int&>);
static_assert(std::is_same_v<decltype(std::declval<V&>().at(0)), int&>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().at(0)), const int&>);
static_assert(std::is_same_v<decltype(std::declval<V&>().front()), int&>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().back()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<V&>().data()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().data()), const int*>);
static_assert(noexcept(std::declval<V&>().data()));

constexpr bool test() {
  V v{10, 20, 30};
  const V& c = v;
  if (v[0] != 10 || c[2] != 30 || v.at(1) != 20 || c.at(2) != 30) return false;
  if (v.front() != 10 || c.back() != 30) return false;
  v[0] = 1;
  v.at(1) = 2;
  v.back() = 3;
  if (v != V{1, 2, 3}) return false;
  if (v.data() != std::addressof(v.front()) || c.data() + 2 != &c.back()) return false;
  V e;
  (void)e.data();  // valid (possibly null) pointer for an empty vector
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  V v{1, 2};
  const V& c = v;
  int threw = 0;
  try { (void)v.at(2); } catch (const std::out_of_range&) { ++threw; }
  try { (void)c.at(5); } catch (const std::out_of_range&) { ++threw; }
  try { (void)V().at(0); } catch (const std::out_of_range&) { ++threw; }
  CHECK(threw == 3);
  return 0;
}
