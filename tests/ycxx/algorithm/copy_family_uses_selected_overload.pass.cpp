// The copying algorithms beyond copy/copy_n/copy_if/copy_backward (those:
// algorithm/copy_uses_selected_overload) assign *i to the output, which selects the assignment
// operator by overload resolution, also for a trivially copyable type:
//   [alg.reverse] reverse_copy: "*(result + N - 1 - i) = *(first + i)";
//   [alg.rotate] rotate_copy: "*(result + i) = *(first + (i + (middle - first)) % N)";
//   [alg.replace] replace_copy(_if): "Assigns through every iterator i ... new_value if E(i)
//     ... otherwise *(first + (i - result))" (Mandates: *first and new_value are writable to
//     result, [iterator.requirements.general]: *o = E);
//   [alg.remove] remove_copy(_if), [alg.unique] unique_copy, [alg.partitions] partition_copy,
//     [partial.sort.copy]: copy the elements, with *first writable to the output (Mandates);
//   [alg.merge] merge and [alg.set.operations] set_union, set_intersection, set_difference,
//     set_symmetric_difference copy elements of the input ranges.
// [alg.shift] shift_left/shift_right "moves the element" with "At most (last - first) - n
// assignments", one move assignment per moved element.
// S: for a non-const lvalue S the assignment template taking U& (U = S) is a better match
// than the defaulted copy assignment; it adds 1000. M: for an rvalue M the template taking U&&
// beats the defaulted copy assignment; it adds 2000. Both types are trivially copyable, so a
// byte copy would leave the values unmarked.
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <deque>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <vector>
#include "check.hpp"

struct S {
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
  friend bool operator==(const S& a, const S& b) { return a.v % 1000 == b.v % 1000; }
  friend auto operator<=>(const S& a, const S& b) { return a.v % 1000 <=> b.v % 1000; }
};
static_assert(std::is_trivially_copyable_v<S>);
static_assert(!std::is_trivially_assignable_v<S&, S&>);

struct M {
  int v;
  M(int x = 0) : v(x) {}
  M(const M&) = default;
  M& operator=(const M&) = default;
  template <class U>
    requires std::same_as<U, M>
  M& operator=(U&& o) {
    v = o.v + 2000;
    return *this;
  }
};
static_assert(std::is_trivially_copyable_v<M>);
static_assert(!std::is_trivially_assignable_v<M&, M&&>);

constexpr int N = 40;

// every written element is a copy made by the template: value 1000 + residue
template <class It>
void expect_marked(It first, It last, std::size_t n) {
  CHECK(static_cast<std::size_t>(std::distance(first, last)) == n);
  for (; first != last; ++first) CHECK(first->v >= 1000 && first->v < 2000);
}

template <class Src>
void run() {
  Src src;
  for (int i = 0; i < N; ++i) src.push_back(S(i));
  Src sorted2;  // odd values 1..2N-1
  for (int i = 0; i < N; ++i) sorted2.push_back(S(2 * i + 1));
  std::vector<S> out(3 * N, S(-1));
  auto o = out.begin();

  expect_marked(o, std::reverse_copy(src.begin(), src.end(), o), N);
  expect_marked(o, std::ranges::reverse_copy(src, o).out, N);
  for (int mid : {0, 1, 17, N}) {
    auto m = std::next(src.begin(), mid);
    expect_marked(o, std::rotate_copy(src.begin(), m, src.end(), o), N);
    expect_marked(o, std::ranges::rotate_copy(src, m, o).out, N);
  }
  // new_value for replace_copy is const: copied unmarked; the others are copies of *first
  const S nv(500);
  auto e = std::replace_copy(src.begin(), src.end(), o, S(3), nv);
  CHECK(e == o + N);
  for (int i = 0; i < N; ++i) CHECK(out[static_cast<std::size_t>(i)].v == (i == 3 ? 500 : 1000 + i));
  e = std::replace_copy_if(src.begin(), src.end(), o, [](const S& s) { return s.v % 2 == 0; }, nv);
  for (int i = 0; i < N; ++i) CHECK(out[static_cast<std::size_t>(i)].v == (i % 2 == 0 ? 500 : 1000 + i));
  auto re = std::ranges::replace_copy(src, o, S(3), nv);
  for (int i = 0; i < N; ++i) CHECK(out[static_cast<std::size_t>(i)].v == (i == 3 ? 500 : 1000 + i));
  CHECK(re.out == o + N);

  expect_marked(o, std::remove_copy(src.begin(), src.end(), o, S(5)), N - 1);
  expect_marked(o, std::remove_copy_if(src.begin(), src.end(), o, [](const S& s) { return s.v % 3 == 0; }),
                N - (N + 2) / 3);
  expect_marked(o, std::ranges::remove_copy(src, o, S(5)).out, N - 1);
  expect_marked(o, std::unique_copy(src.begin(), src.end(), o), N);
  expect_marked(o, std::ranges::unique_copy(src, o).out, N);
  {
    std::vector<S> t(N), f(N);
    auto [et, ef] = std::partition_copy(src.begin(), src.end(), t.begin(), f.begin(),
                                        [](const S& s) { return s.v % 2 == 0; });
    expect_marked(t.begin(), et, N / 2);
    expect_marked(f.begin(), ef, N / 2);
    auto r = std::ranges::partition_copy(src, t.begin(), f.begin(), [](const S& s) { return s.v < 10; });
    expect_marked(t.begin(), r.out1, 10);
    expect_marked(f.begin(), r.out2, N - 10);
  }
  {
    std::vector<S> small(7);
    auto pe = std::partial_sort_copy(src.begin(), src.end(), small.begin(), small.end());
    expect_marked(small.begin(), pe, 7);
    auto big = std::partial_sort_copy(src.begin(), src.end(), out.begin(), out.end());
    expect_marked(out.begin(), big, N);
    auto rr = std::ranges::partial_sort_copy(src, small);
    expect_marked(small.begin(), rr.out, 7);
  }
  // merge and the set operations: src (0..N-1) and sorted2 (odd 1..2N-1), both sorted by residue
  expect_marked(o, std::merge(src.begin(), src.end(), sorted2.begin(), sorted2.end(), o), 2 * N);
  expect_marked(o, std::ranges::merge(src, sorted2, o).out, 2 * N);
  auto u = std::set_union(src.begin(), src.end(), sorted2.begin(), sorted2.end(), o);
  expect_marked(o, u, static_cast<std::size_t>(N + N / 2));
  expect_marked(o, std::ranges::set_union(src, sorted2, o).out, static_cast<std::size_t>(N + N / 2));
  expect_marked(o, std::set_intersection(src.begin(), src.end(), sorted2.begin(), sorted2.end(), o), N / 2);
  expect_marked(o, std::ranges::set_intersection(src, sorted2, o).out, N / 2);
  expect_marked(o, std::set_difference(src.begin(), src.end(), sorted2.begin(), sorted2.end(), o), N / 2);
  expect_marked(o, std::ranges::set_difference(src, sorted2, o).out, N / 2);
  expect_marked(o, std::set_symmetric_difference(src.begin(), src.end(), sorted2.begin(), sorted2.end(), o),
                static_cast<std::size_t>(N));
  expect_marked(o, std::ranges::set_symmetric_difference(src, sorted2, o).out, static_cast<std::size_t>(N));
}

template <class C>
void shifts() {
  for (int n : {1, 2, 9, 39}) {
    C c;
    for (int i = 0; i < N; ++i) c.push_back(M(i));
    auto nl = std::shift_left(c.begin(), c.end(), n);
    CHECK(nl == c.end() - n);
    for (int i = 0; i < N - n; ++i) CHECK(c[static_cast<std::size_t>(i)].v == i + n + 2000);
    for (int i = 0; i < N; ++i) c[static_cast<std::size_t>(i)].v = i;
    auto nf = std::shift_right(c.begin(), c.end(), n);
    CHECK(nf == c.begin() + n);
    for (int i = n; i < N; ++i) CHECK(c[static_cast<std::size_t>(i)].v == i - n + 2000);
    for (int i = 0; i < N; ++i) c[static_cast<std::size_t>(i)].v = i;
    auto rl = std::ranges::shift_left(c, n);
    CHECK(rl.end() == c.end() - n);
    for (int i = 0; i < N - n; ++i) CHECK(c[static_cast<std::size_t>(i)].v == i + n + 2000);
    for (int i = 0; i < N; ++i) c[static_cast<std::size_t>(i)].v = i;
    auto rr = std::ranges::shift_right(c, n);
    CHECK(rr.begin() == c.begin() + n);
    for (int i = n; i < N; ++i) CHECK(c[static_cast<std::size_t>(i)].v == i - n + 2000);
  }
}

int main() {
  run<std::vector<S>>();
  run<std::deque<S>>();
  shifts<std::vector<M>>();
  shifts<std::deque<M>>();
}
