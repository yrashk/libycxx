// [sequence.reqmts]/79-100, /113-116, the front operations required for deque, forward_list
// and list: a.emplace_front(args) has type reference, prepends T(std::forward<Args>(args)...)
// and returns a.front(); a.push_front(t) / a.push_front(rv) have type void and prepend a
// copy; a.prepend_range(rg) has type void, inserts copies of the elements of rg before
// begin() without reversing their order and dereferences each iterator of rg exactly once;
// a.pop_front() has type void and destroys the first element. Only begin(), front() and the
// front operations are used, so the checks also apply to forward_list.
#pragma once
#include <ranges>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_iterators.hpp"

namespace reqs::sequence_front_ops {

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  static_assert(std::is_same_v<decltype(std::declval<X&>().emplace_front(val<T>(1))), typename X::reference>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().push_front(std::declval<const T&>())), void>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().push_front(std::declval<T>())), void>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().prepend_range(std::declval<T (&)[2]>())), void>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().pop_front()), void>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().front()), typename X::reference>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().front()), typename X::const_reference>);

  X a;
  const T t = val<T>(1);
  a.push_front(t);
  a.push_front(val<T>(2));
  T tmp = val<T>(3);
  a.push_front(std::move(tmp));
  if (!holds(a, {3, 2, 1})) return false;
  auto&& r = a.emplace_front(val<T>(4));
  if (std::addressof(r) != std::addressof(a.front()) || !(r == val<T>(4)) || !holds(a, {4, 3, 2, 1}))
    return false;
  const X& ca = a;
  if (std::addressof(ca.front()) != std::addressof(*ca.begin())) return false;
  a.front() = val<T>(5);
  if (!holds(a, {5, 3, 2, 1})) return false;
  a.pop_front();
  if (!holds(a, {3, 2, 1})) return false;
  a.pop_front();
  a.pop_front();
  a.pop_front();
  if (!(a.begin() == a.end())) return false;
  for (int i = 0; i < 50; ++i) a.push_front(val<T>(i));
  if (count_elems(a) != 50 || !(a.front() == val<T>(49))) return false;
  for (int i = 0; i < 50; ++i) a.pop_front();
  if (!(a.begin() == a.end())) return false;

  // emplace_front with an argument referring to an element of a
  a.push_front(val<T>(7));
  for (int i = 0; i < 40; ++i) a.emplace_front(a.front());
  if (count_elems(a) != 41) return false;
  for (const auto& x : a)
    if (!(x == val<T>(7))) return false;

  T arr[30];
  for (int i = 0; i < 30; ++i) arr[i] = val<T>(i);
  int d = 0;
  X b;
  b.push_front(val<T>(40));
  b.prepend_range(InputRange<T>{arr, arr + 3, &d});
  if (d != 3 || !holds(b, {0, 1, 2, 40})) return false;
  d = 0;
  b.prepend_range(ForwardRange<T>{arr + 10, arr + 12, &d});
  if (d != 2 || !holds(b, {10, 11, 0, 1, 2, 40})) return false;
  b.prepend_range(std::ranges::subrange(arr, arr));
  if (!holds(b, {10, 11, 0, 1, 2, 40})) return false;
  d = 0;
  b.prepend_range(InputRange<T>{arr, arr + 30, &d});
  if (d != 30 || count_elems(b) != 36 || !(b.front() == val<T>(0))) return false;
  X e;
  e.prepend_range(arr);
  if (count_elems(e) != 30 || !(e.front() == val<T>(0))) return false;
  return true;
}

}  // namespace reqs::sequence_front_ops
