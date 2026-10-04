// [sequence.reqmts]/9, 12, 38, 42, 59, 63, 111: "Each iterator in the range [i, j) is
// dereferenced exactly once" for X(i, j), a.insert(p, i, j) and a.assign(i, j), and "each
// iterator in the range rg is dereferenced exactly once" for X(from_range, rg),
// a.insert_range(p, rg), a.assign_range(rg) and a.append_range(rg). Checked with counting
// single-pass and forward iterators/ranges, inserting into the middle and with enough
// elements to require reallocation.
#include <vector>
#include <string>
#include <ranges>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

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
    a.insert(a.cbegin() + 1, InputIter<T>(arr, &d), InputIter<T>(arr + 25, &d));
    if (!once(25) || count_elems(a) != 28) return false;
    a.insert(a.cbegin() + 2, ForwardIter<T>(arr, &d), ForwardIter<T>(arr + 25, &d));
    if (!once(25) || count_elems(a) != 53) return false;
    a.insert_range(a.cbegin() + 3, InputRange<T>{arr, arr + 20, &d});
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
  return 0;
}
