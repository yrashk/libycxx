// The numeric algorithms assign through the output with the expressions the draft gives, which
// select the assignment operator (and constructor) by overload resolution, also for a
// trivially copyable type:
//   [numeric.iota] iota: "assigns *i = value and increments value as if by ++value" (value is a
//     non-const lvalue of type T); ranges::iota: "*first = as_const(value);" (a const lvalue);
//   [partial.sum]: "creates an accumulator acc whose type is InputIterator's value type,
//     initializes it with *first, and assigns the result to *result. For every iterator i in
//     [first + 1, last) in order, acc is then modified by acc = std::move(acc) + *i ... and the
//     result is assigned to *(result + (i - first))";
//   [adjacent.difference]: acc "initializes it with *first, and assigns the result to *result";
//     for later i, "creates an object val whose type is T, initializes it with *i, computes
//     binary_op(val, std::move(acc)), assigns the result to *(result + (i - first)), and move
//     assigns from val to acc".
// S: for a non-const lvalue S the constructor and assignment templates taking U& (U = S) are
// better matches than the defaulted copy operations; they add 1000. Rvalues use the defaulted
// (trivial) operations. So iota's outputs are marked once, ranges::iota's are not;
// partial_sum's outputs carry two marks (initialization of acc from *first, assignment from
// acc); adjacent_difference's first output carries two marks and the differences none.
#include <concepts>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <type_traits>
#include <vector>
#include "check.hpp"

struct S {
  using difference_type = int;  // weakly_incrementable, for ranges::iota
  int v;
  S(int x = 0) : v(x) {}
  S(const S&) = default;
  S& operator=(const S&) = default;
  template <class U>
    requires std::same_as<U, S>
  S(U& o) : v(o.v + 1000) {}
  template <class U>
    requires std::same_as<U, S>
  S& operator=(U& o) {
    v = o.v + 1000;
    return *this;
  }
  S& operator++() {
    ++v;
    return *this;
  }
  S operator++(int) {
    S old(v);
    ++v;
    return old;
  }
  friend S operator+(const S& a, const S& b) { return S(a.v + b.v); }
  friend S operator-(const S& a, const S& b) { return S(a.v - b.v); }
};
static_assert(std::is_trivially_copyable_v<S> && std::weakly_incrementable<S>);

constexpr int N = 12;

template <class C>
void run() {
  C out(N);
  std::iota(out.begin(), out.end(), S(5));
  for (int i = 0; i < N; ++i) CHECK(out[static_cast<std::size_t>(i)].v == 5 + i + 1000);
  auto r = std::ranges::iota(out.begin(), out.end(), S(5));
  CHECK(r.out == out.end() && r.value.v == 5 + N);
  for (int i = 0; i < N; ++i) CHECK(out[static_cast<std::size_t>(i)].v == 5 + i);

  C src;
  for (int i = 0; i < N; ++i) src.push_back(S(i * i));
  auto e = std::partial_sum(src.begin(), src.end(), out.begin());
  CHECK(e == out.end());
  int prefix = 0;
  for (int i = 0; i < N; ++i) {
    prefix += i * i;
    CHECK(out[static_cast<std::size_t>(i)].v == prefix + 2000);
  }
  e = std::partial_sum(src.begin(), src.end(), out.begin(), [](S a, const S& b) { return S(a.v + b.v); });
  prefix = 0;
  for (int i = 0; i < N; ++i) {
    prefix += i * i;
    CHECK(out[static_cast<std::size_t>(i)].v == prefix + 2000);
  }
  e = std::adjacent_difference(src.begin(), src.end(), out.begin());
  CHECK(e == out.end());
  CHECK(out[0].v == 0 + 2000);
  for (int i = 1; i < N; ++i) CHECK(out[static_cast<std::size_t>(i)].v == i * i - (i - 1) * (i - 1));
}

int main() {
  run<std::vector<S>>();
  S arr_src[N];
  for (int i = 0; i < N; ++i) arr_src[i] = S(i);  // prvalue: trivial
  S arr_out[N];
  std::partial_sum(arr_src, arr_src + N, arr_out);
  CHECK(arr_out[N - 1].v == N * (N - 1) / 2 + 2000);
  std::iota(arr_out, arr_out + N, S(0));
  CHECK(arr_out[3].v == 1003);
}
