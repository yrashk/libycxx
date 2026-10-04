// [sequence.reqmts]/70-128, the operations required for basic_string and vector:
// a.front() / a.back() return *a.begin() / *--a.end() (const_reference for a const X,
// reference otherwise); a.push_back(t) and a.push_back(rv) append a copy (type void);
// a.pop_back() destroys the last element (type void); a[n] is *(a.begin() + n); a.at(n) is
// *(a.begin() + n) and throws out_of_range if n >= a.size(). Also append_range(rg) (type
// void, inserts copies of rg before end()), required for vector ([sequence.reqmts]/112) and
// provided by basic_string ([string.append]; there it returns *this).
#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  using S = typename X::size_type;
  using R = typename X::reference;
  using CR = typename X::const_reference;
  static_assert(std::is_same_v<decltype(std::declval<X&>().front()), R>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().front()), CR>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().back()), R>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().back()), CR>);
  static_assert(std::is_same_v<decltype(std::declval<X&>()[S()]), R>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>()[S()]), CR>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().at(S())), R>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().at(S())), CR>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().push_back(std::declval<const T&>())), void>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().push_back(std::declval<T>())), void>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().pop_back()), void>);

  X a;
  const T t = val<T>(1);
  a.push_back(t);
  a.push_back(val<T>(2));
  T tmp = val<T>(3);
  a.push_back(std::move(tmp));
  if (!holds(a, {1, 2, 3})) return false;
  const X& ca = a;
  if (!(a.front() == val<T>(1)) || !(ca.front() == *ca.begin())) return false;
  if (!(a.back() == val<T>(3)) || !(ca.back() == *(ca.end() - 1))) return false;
  for (S n = 0; n < 3; ++n) {
    if (!(a[n] == *(a.begin() + n)) || !(ca[n] == *(ca.begin() + n))) return false;
    if (!(a.at(n) == *(a.begin() + n)) || !(ca.at(n) == *(ca.begin() + n))) return false;
  }
  a.front() = val<T>(4);
  a.back() = val<T>(5);
  a[1] = val<T>(6);
  if (!holds(a, {4, 6, 5})) return false;
  a.at(1) = val<T>(7);
  if (!holds(a, {4, 7, 5})) return false;
  a.pop_back();
  if (!holds(a, {4, 7})) return false;
  a.pop_back();
  a.pop_back();
  if (!a.empty()) return false;
  for (int i = 0; i < 50; ++i) a.push_back(val<T>(i));
  if (count_elems(a) != 50 || !(a.back() == val<T>(49)) || !(a.front() == val<T>(0))) return false;

  T arr[] = {val<T>(10), val<T>(11)};
  X b = make<X>({1});
  b.append_range(arr);
  b.append_range(InputRange<T>{arr, arr + 1});
  b.append_range(ForwardRange<T>{arr + 1, arr + 2});
  return holds(b, {1, 10, 11, 10, 11});
}

template <class X>
bool at_throws() {
  X a = make<X>({1, 2});
  const X& ca = a;
  int n = 0;
  try { (void)a.at(2); } catch (const std::out_of_range&) { ++n; }
  try { (void)ca.at(2); } catch (const std::out_of_range&) { ++n; }
  try { (void)a.at(static_cast<typename X::size_type>(-1)); } catch (const std::out_of_range&) { ++n; }
  X e;
  try { (void)e.at(0); } catch (const std::out_of_range&) { ++n; }
  return n == 4 && holds(a, {1, 2});
}

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  CHECK(at_throws<std::vector<int>>());
  CHECK(at_throws<std::vector<Elem>>());
  CHECK(at_throws<std::vector<bool>>());
  CHECK(at_throws<std::string>());
  return 0;
}
