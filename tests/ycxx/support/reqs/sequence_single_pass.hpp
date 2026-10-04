// Generic requirement checks extracted from tests/ycxx/containers/sequence_single_pass.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <ranges>
#include "container_values.hpp"
#include "test_iterators.hpp"

namespace reqs::sequence_single_pass {

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  T arr[30];
  for (int i = 0; i < 30; ++i) arr[i] = val<T>(i);
  int d = 0;
  auto once = [&](int n) {
    bool ok = d == n;
    d = 0;
    return ok;
  };
  {
    X a(InputIter<T>(arr, &d), InputIter<T>(arr + 30, &d));
    if (!once(30) || count_elems(a) != 30) return false;
    X b(ForwardIter<T>(arr, &d), ForwardIter<T>(arr + 30, &d));
    if (!once(30) || !(a == b)) return false;
    X c(std::from_range, InputRange<T>{arr, arr + 30, &d});
    if (!once(30) || !(c == a)) return false;
    X f(std::from_range, ForwardRange<T>{arr, arr + 30, &d});
    if (!once(30) || !(f == a)) return false;
  }
  {
    X a = make<X>({1, 2, 3});
    a.insert(cnth(a, 1), InputIter<T>(arr, &d), InputIter<T>(arr + 25, &d));
    if (!once(25) || count_elems(a) != 28) return false;
    a.insert(cnth(a, 2), ForwardIter<T>(arr, &d), ForwardIter<T>(arr + 25, &d));
    if (!once(25) || count_elems(a) != 53) return false;
    a.insert_range(cnth(a, 3), InputRange<T>{arr, arr + 20, &d});
    if (!once(20) || count_elems(a) != 73) return false;
    a.insert_range(a.cend(), ForwardRange<T>{arr, arr + 20, &d});
    if (!once(20) || count_elems(a) != 93) return false;
  }
  {
    X a = make<X>({1, 2, 3});
    a.assign(InputIter<T>(arr, &d), InputIter<T>(arr + 30, &d));
    if (!once(30) || count_elems(a) != 30) return false;
    a.assign(ForwardIter<T>(arr, &d), ForwardIter<T>(arr + 2, &d));
    if (!once(2) || count_elems(a) != 2) return false;
    a.assign_range(InputRange<T>{arr, arr + 30, &d});
    if (!once(30) || count_elems(a) != 30) return false;
    a.assign_range(ForwardRange<T>{arr, arr + 5, &d});
    if (!once(5) || count_elems(a) != 5) return false;
    a.append_range(InputRange<T>{arr, arr + 30, &d});
    if (!once(30) || count_elems(a) != 35) return false;
    a.append_range(ForwardRange<T>{arr, arr + 30, &d});
    if (!once(30) || count_elems(a) != 65) return false;
  }
  return true;
}

}  // namespace reqs::sequence_single_pass
