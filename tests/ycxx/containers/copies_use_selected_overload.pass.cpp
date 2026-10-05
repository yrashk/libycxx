// Containers copy elements from a range or an argument with the constructor (assignment)
// that overload resolution selects for the source expression, also when the element type is
// trivially copyable (vector, deque and list range members: vector/range_ops_use_selected_overload):
//   [sequence.reqmts]: X(i, j), X(from_range, rg), a.insert(p, i, j), a.insert_range(p, rg),
//     a.append_range(rg), a.prepend_range(rg), a.assign(i, j), a.assign_range(rg) are
//     EmplaceConstructible from *i (and assignable from *i for assign); a.emplace(p, args),
//     a.emplace_back(args), a.emplace_front(args) construct "with std::forward<Args>(args)...";
//     [range.utility.conv.to] ranges::to.
//   inplace_vector ([inplace.vector.modifiers]: try_emplace_back, unchecked_emplace_back "with
//     std::forward<Args>(args)..."), forward_list
//     ([forward.list.modifiers]: insert_after(p, first, last), insert_range_after,
//     emplace_after, prepend_range); hive: hive/copies_use_selected_overload.
//   flat_set / flat_multiset ([flat.set.modifiers]/5: insert(first, last) adds elements "as if by
//     c.insert(c.end(), first, last)"; /10: insert_range by "ranges::for_each(rg, [&](value_type
//     e) { c.insert(c.end(), std::move(e)); })"; emplace "initializes an object t of type
//     value_type with std::forward<Args>(args)...").
//   flat_map ([flat.map.modifiers]: insert(first, last) "for (; first != last; ++first) {
//     value_type value = *first; ...}", insert_range likewise; value_type is pair<key_type,
//     mapped_type>, initialized from a pair<S, S>& by pair(pair<U1, U2>&), [pairs.pair]).
//   set / map node construction ([associative.reqmts] emplace).
// S is trivially copyable, but for a non-const lvalue S its constructor and assignment
// templates taking U& (U = S) are better matches than the defaulted copy operations; they add
// 1000. Moves (rvalues) use the defaulted operations and keep the value. So every element
// copied from the source carries a mark; a byte copy of the source does not. Copies of
// existing elements are not checked.
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <deque>
#include <flat_map>
#include <flat_set>
#include <forward_list>
#include <inplace_vector>
#include <iterator>
#include <list>
#include <map>
#include <ranges>
#include <set>
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

template <class C>
void sequence(Src& src) {
  for (int n : {1, 5, N}) {
    sequence_common<C>(src, n);
    auto rg = src.range(n);
    for (int at : {0, 1, 3}) {
      C c(3, S(-5));
      c.insert(std::next(c.begin(), at), src.a, src.a + n);
      expect_marked(c, n);
      C c2(3, S(-5));
      c2.insert_range(std::next(c2.begin(), at), rg);
      expect_marked(c2, n);
      C c3(3, S(-5));
      for (int i = 0; i < n; ++i) c3.emplace(std::next(c3.begin(), at), src.a[i]);
      expect_marked(c3, n);
    }
    C d(2, S(-7));
    d.append_range(rg);
    expect_marked(d, n);
    C e;
    for (int i = 0; i < n; ++i) e.emplace_back(src.a[i]);
    expect_marked(e, n);
    if constexpr (requires { e.prepend_range(rg); }) {
      C f(2, S(-7));
      f.prepend_range(rg);
      expect_marked(f, n);
      C g;
      for (int i = 0; i < n; ++i) g.emplace_front(src.a[i]);
      expect_marked(g, n);
    }
  }
}

static void inplace(Src& src) {
  using C = std::inplace_vector<S, 64>;
  sequence<C>(src);
  for (int n : {1, 5, N}) {
    C a;
    for (int i = 0; i < n; ++i) CHECK(a.try_emplace_back(src.a[i]).has_value());
    expect_marked(a, n);
    C b;
    for (int i = 0; i < n; ++i) b.unchecked_emplace_back(src.a[i]);
    expect_marked(b, n);
  }
}

static void forward(Src& src) {
  using C = std::forward_list<S>;
  for (int n : {1, 5, N}) {
    sequence_common<C>(src, n);
    auto rg = src.range(n);
    C c(2, S(-3));
    c.insert_after(std::next(c.begin()), src.a, src.a + n);
    expect_marked(c, n);
    C c2(2, S(-3));
    c2.insert_range_after(c2.before_begin(), rg);
    expect_marked(c2, n);
    C c3;
    for (int i = 0; i < n; ++i) c3.emplace_after(c3.before_begin(), src.a[i]);
    expect_marked(c3, n);
    C c4(2, S(-3));
    c4.prepend_range(rg);
    expect_marked(c4, n);
    C c5;
    for (int i = 0; i < n; ++i) c5.emplace_front(src.a[i]);
    expect_marked(c5, n);
  }
}

template <class C>
void flat_set_like(Src& src) {
  for (int n : {1, 5, N}) {
    auto rg = src.range(n);
    expect_marked(C(src.a, src.a + n), n);
    expect_marked(C(std::from_range, rg), n);
    C a{S(-1), S(-2)};
    a.insert(src.a, src.a + n);
    expect_marked(a, n);
    C b{S(-1), S(-2)};
    b.insert_range(rg);
    expect_marked(b, n);
    C c;
    for (int i = 0; i < n; ++i) c.emplace(src.a[i]);
    expect_marked(c, n);
    C d;
    for (int i = 0; i < n; ++i) d.insert(src.a[i]);  // insert(const value_type&): trivial copy
    for (const S& s : d) CHECK(s.v < 1000);
  }
}

template <class C>
void map_like(Src& src) {
  std::vector<std::pair<S, S>> pairs;
  for (int i = 0; i < N; ++i) pairs.emplace_back(S(i), S(i));
  for (int n : {1, 5, N}) {
    auto rg = std::ranges::subrange(pairs.data(), pairs.data() + n);
    auto keys = [](const C& m) {
      std::vector<S> k;
      for (const auto& [key, value] : m) {
        k.push_back(key);
        CHECK(value.v >= 1000 && value.v % 1000 == key.v % 1000);
      }
      return k;
    };
    expect_marked(keys(C(pairs.data(), pairs.data() + n)), n);
    expect_marked(keys(C(std::from_range, rg)), n);
    C a;
    a.insert(pairs.data(), pairs.data() + n);
    expect_marked(keys(a), n);
    C b;
    b.insert_range(rg);
    expect_marked(keys(b), n);
    C c;
    for (int i = 0; i < n; ++i) c.emplace(src.a[i], src.a[i]);
    expect_marked(keys(c), n);
  }
}

int main() {
  Src src;
  sequence<std::vector<S>>(src);
  sequence<std::deque<S>>(src);
  sequence<std::list<S>>(src);
  inplace(src);
  forward(src);
  flat_set_like<std::flat_set<S>>(src);
  flat_set_like<std::flat_multiset<S>>(src);
  flat_set_like<std::set<S>>(src);
  map_like<std::flat_map<S, S>>(src);
  map_like<std::flat_multimap<S, S>>(src);
  map_like<std::map<S, S>>(src);
}
