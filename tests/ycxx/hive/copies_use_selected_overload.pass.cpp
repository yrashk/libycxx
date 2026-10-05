// [hive.cons], [hive.modifiers]: hive(first, last), hive(from_range, rg), insert(first, last),
// insert_range(rg), assign(first, last), assign_range(rg) are EmplaceConstructible from *i
// ("Inserts copies of elements"), emplace(args) constructs "with std::forward<Args>(args)...";
// [range.utility.conv.to] ranges::to. S is trivially copyable, but for a non-const lvalue S
// its constructor and assignment templates taking U& (U = S) are better matches than the
// defaulted copy operations; they add 1000, so every element copied from the source carries a
// mark and a byte copy does not (as containers/copies_use_selected_overload for the other
// containers).
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <hive>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
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
  friend bool operator==(const S& a, const S& b) { return a.v == b.v; }
  friend auto operator<=>(const S& a, const S& b) { return a.v <=> b.v; }
};
static_assert(std::is_trivially_copyable_v<S>);
static_assert(!std::is_trivially_constructible_v<S, S&>);
static_assert(std::is_trivially_constructible_v<S, S&&>);

constexpr int N = 24;

// the values of the elements copied from src[0, n): each marked at least once, residues 0..n-1
template <class R>
void expect_marked(const R& r, int n) {
  std::vector<int> res;
  for (const S& s : r)
    if (s.v >= 0) {  // pre-existing elements are negative
      CHECK(s.v >= 1000);
      res.push_back(s.v % 1000);
    }
  CHECK(static_cast<int>(res.size()) == n);
  std::sort(res.begin(), res.end());
  for (int i = 0; i < n; ++i) CHECK(res[static_cast<std::size_t>(i)] == i);
}

struct Src {
  S a[N];
  Src() {
    for (int i = 0; i < N; ++i) a[i].v = i;
  }
  auto range(int n) { return std::ranges::subrange(a, a + n); }
};

template <class C>
void sequence_common(Src& src, int n) {
  auto rg = src.range(n);
  expect_marked(C(src.a, src.a + n), n);
  expect_marked(C(std::from_range, rg), n);
  expect_marked(std::ranges::to<C>(rg), n);
  C b(5, S(-1));
  b.assign(src.a, src.a + n);
  expect_marked(b, n);
  C b2(30, S(-1));
  b2.assign_range(rg);
  expect_marked(b2, n);
}

static void hive(Src& src) {
  using C = std::hive<S>;
  for (int n : {1, 5, N}) {
    sequence_common<C>(src, n);
    auto rg = src.range(n);
    C c(4, S(-3));
    c.insert(src.a, src.a + n);
    expect_marked(c, n);
    C c2(4, S(-3));
    c2.insert_range(rg);
    expect_marked(c2, n);
    C c3;
    for (int i = 0; i < n; ++i) c3.emplace(src.a[i]);
    expect_marked(c3, n);
    C c4(src.a, src.a + n);
    c4.erase(c4.begin());  // leaves a hole, reused by the next insertions
    c4.insert(src.a, src.a + n);
    std::vector<S> rest(c4.begin(), c4.end());  // (copies from const S&: unchanged)
    CHECK(rest.size() == static_cast<std::size_t>(2 * n - 1));
    for (const S& s : rest) CHECK(s.v >= 1000);
  }
}

int main() {
  Src src;
  hive(src);
}
