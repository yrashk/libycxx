// [deque.overview]: operator[](n) and at(n) (const and non-const), front(), back();
// [sequence.reqmts]/117-128: a[n] returns *(a.begin() + n); a.at(n) returns
// *(a.begin() + n) and throws out_of_range if n >= a.size(). Checked on a deque grown at
// both ends to well over any single block, so indices cross internal storage boundaries,
// and after elements are removed from the front (indices are relative to the current
// first element).
#include <deque>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include "container_values.hpp"
#include "check.hpp"

template <class T>
constexpr bool test(int n) {
  std::deque<T> d;
  // elements -n+1 .. n-1 with value index i + n, built from the middle outwards
  d.push_back(val<T>(0 % 80));
  for (int i = 1; i < n; ++i) {
    d.push_front(val<T>(i % 80));
    d.push_back(val<T>(i % 80));
  }
  const auto& cd = d;
  const std::size_t sz = d.size();
  if (sz != static_cast<std::size_t>(2 * n - 1)) return false;
  for (std::size_t k = 0; k < sz; ++k) {
    int dist = static_cast<int>(k) - (n - 1);
    int idx = (dist < 0 ? -dist : dist) % 80;
    if (!(d[k] == val<T>(idx)) || !(cd[k] == val<T>(idx))) return false;
    if (!(d.at(k) == val<T>(idx)) || !(cd.at(k) == val<T>(idx))) return false;
    if (&d[k] != &*(d.begin() + static_cast<std::ptrdiff_t>(k))) return false;
    if (&cd.at(k) != &d[k]) return false;
  }
  if (&d.front() != &d[0] || &d.back() != &d[sz - 1] || &cd.front() != &d[0] || &cd.back() != &d[sz - 1]) return false;
  // writing through operator[] and at
  d[3] = val<T>(77);
  d.at(4) = val<T>(78);
  if (!(cd[3] == val<T>(77)) || !(cd.at(4) == val<T>(78))) return false;
  // indices follow the current front
  for (int i = 0; i < n / 2; ++i) d.pop_front();
  if (d.size() != sz - static_cast<std::size_t>(n / 2)) return false;
  for (std::size_t k = 0; k < d.size(); ++k)
    if (&d[k] != &*(d.begin() + static_cast<std::ptrdiff_t>(k))) return false;
  return true;
}

template <class T>
bool at_throws() {
  std::deque<T> d;
  for (int i = 0; i < 100; ++i) d.push_front(val<T>(i));
  const auto& cd = d;
  int caught = 0;
  try { (void)d.at(100); } catch (const std::out_of_range&) { ++caught; }
  try { (void)cd.at(100); } catch (const std::out_of_range&) { ++caught; }
  try { (void)d.at(static_cast<std::size_t>(-1)); } catch (const std::out_of_range&) { ++caught; }
  d.pop_front();
  try { (void)d.at(99); } catch (const std::out_of_range&) { ++caught; }
  try { (void)d.at(98); } catch (const std::out_of_range&) { caught += 100; }
  return caught == 4;
}

static_assert(std::is_same_v<decltype(std::declval<std::deque<int>&>()[0]), int&>);
static_assert(std::is_same_v<decltype(std::declval<const std::deque<int>&>()[0]), const int&>);
static_assert(std::is_same_v<decltype(std::declval<std::deque<int>&>().at(0)), int&>);
static_assert(std::is_same_v<decltype(std::declval<const std::deque<int>&>().at(0)), const int&>);
static_assert(test<int>(5));
static_assert(test<int>(300));
static_assert(test<Elem>(70));

int main() {
  CHECK(test<int>(5));
  CHECK(test<int>(3000));
  CHECK(test<char>(1000));
  CHECK(test<Elem>(500));
  CHECK(test<double>(700));
  CHECK(at_throws<int>());
  CHECK(at_throws<Elem>());
  return 0;
}
