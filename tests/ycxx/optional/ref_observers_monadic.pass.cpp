// [optional.ref.observe]: operator-> returns T*, operator* returns T& (both const noexcept),
// value() returns T& or throws bad_optional_access, value_or returns remove_cv_t<T> (by value).
// [optional.ref.monadic]: and_then / transform invoke with T& (const-ness of the optional does
// not matter); transform yields optional<remove_cv_t<invoke_result_t<F, T&>>> (references
// preserved); or_else returns optional<T&>.
// [optional.ref.iterators]: iterator models contiguous_iterator, reference T&.
// REQUIRES: exceptions
#include <optional>
#include <iterator>
#include <type_traits>
#include <utility>
#include "check.hpp"

using O = std::optional<int&>;
static_assert(std::is_same_v<decltype(*std::declval<const O&>()), int&>);
static_assert(std::is_same_v<decltype(*std::declval<O&&>()), int&>);
static_assert(std::is_same_v<decltype(std::declval<const O&>().operator->()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const O&>().value()), int&>);
static_assert(std::is_same_v<decltype(std::declval<O&&>().value()), int&>);
static_assert(std::is_same_v<decltype(std::declval<const O&>().value_or(1)), int>);
static_assert(std::is_same_v<decltype(std::declval<std::optional<const int&>&>().value_or(1)), int>);
static_assert(noexcept(*std::declval<const O&>()));
static_assert(noexcept(std::declval<const O&>().has_value()));
static_assert(std::contiguous_iterator<O::iterator>);
static_assert(std::is_same_v<std::iter_reference_t<O::iterator>, int&>);
static_assert(std::is_same_v<std::iter_value_t<std::optional<const int&>::iterator>, int>);
static_assert(std::is_same_v<std::iter_reference_t<std::optional<const int&>::iterator>, const int&>);
static_assert(std::is_same_v<decltype(std::declval<const O&>().begin()), O::iterator>);
static_assert(noexcept(std::declval<const O&>().begin()) && noexcept(std::declval<const O&>().end()));

template <class T> concept has_begin = requires(T t) { t.begin(); };
static_assert(!has_begin<std::optional<int (&)()>>);  // T not an object type
static_assert(!has_begin<std::optional<int (&)[]>>);  // array of unknown bound
static_assert(has_begin<std::optional<int (&)[3]>>);

struct Cat {
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(const int&) const { return 2; }
  constexpr int operator()(int&&) const { return 3; }
};

constexpr bool test() {
  int a = 4;
  const O o(a);
  O e;
  if (o.value() != 4 || &o.value() != &a || o.operator->() != &a) return false;
  if (o.value_or(9) != 4 || e.value_or(9) != 9) return false;
  // monadic: always invoked with T& even through const or rvalue optional
  if (o.transform(Cat{}) != 1 || std::move(o).transform(Cat{}) != 1) return false;
  if (e.transform(Cat{}).has_value()) return false;
  auto tr = o.transform([](int& x) -> int& { return x; });
  static_assert(std::is_same_v<decltype(tr), std::optional<int&>>);
  if (&*tr != &a) return false;
  auto tv = o.transform([](int x) { return x * 2L; });
  static_assert(std::is_same_v<decltype(tv), std::optional<long>>);
  if (*tv != 8) return false;
  auto at = o.and_then([](int& x) { return std::optional<int&>(x); });
  static_assert(std::is_same_v<decltype(at), std::optional<int&>>);
  if (&*at != &a) return false;
  if (e.and_then([](int& x) { return std::optional<int&>(x); }).has_value()) return false;
  int b = 7;
  auto oe = e.or_else([&] { return O(b); });
  static_assert(std::is_same_v<decltype(oe), O>);
  if (&*oe != &b) return false;
  auto oo = o.or_else([&] { return O(b); });
  if (&*oo != &a) return false;
  // iteration
  int sum = 0;
  for (int& x : o) { x += 1; sum += x; }
  for (int& x : e) sum += x + 100;
  if (sum != 5 || a != 5) return false;
  if (o.end() - o.begin() != 1 || e.begin() != e.end()) return false;
  if (&*o.begin() != &a) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  O e;
  bool threw = false;
  try { (void)e.value(); } catch (const std::bad_optional_access&) { threw = true; }
  CHECK(threw);
  return 0;
}
