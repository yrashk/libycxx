// Generic check, instantiated per sequence container: the container works with element
// types whose associated namespaces contain poisoned function templates
// (support/adl_poison.hpp) and with pointers to an instantiation that must not be completed.
// [contents]/3: "Whenever an unqualified name other than swap, make_error_code,
// make_error_condition, from_stream, or submdspan_mapping is used in the specification of a
// declaration D in [library] ... its meaning is established as-if by performing unqualified
// name lookup in the context of D", so argument-dependent lookup must not find evil::move,
// evil::copy, ::addressof, ... from inside the library. The element requirements
// ([container.alloc.reqmts]/2, [utility.arg.requirements]) and the iterator requirements
// ([iterator.requirements]) do not include a unary & or a comma operator, so evil::Val
// (both deleted) and evil::Iter (both deleted) must be usable.
// Holder<Incomplete>* elements: performing ADL for a call with such an argument requires
// completing Holder<Incomplete>, which is ill-formed ([basic.lookup.argdep]/3, [temp.inst]).
#pragma once
#include <compare>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <ranges>
#include <utility>
#include "adl_poison.hpp"

namespace reqs::adl_robustness_seq {

template <class X>
constexpr bool is_forward_list = requires(X& x) { x.before_begin(); };

template <class X>
constexpr std::ptrdiff_t length(const X& x) {
  std::ptrdiff_t n = 0;
  for (auto it = x.begin(); it != x.end(); ++it) ++n;
  return n;
}

// Inserts v before element k (insert_after the (k-1)-th for forward_list).
template <class X, class V>
constexpr void insert_one(X& x, int k, const V& v) {
  if constexpr (is_forward_list<X>) {
    auto p = x.cbefore_begin();
    for (int i = 0; i < k; ++i) ++p;
    x.insert_after(p, v);
  } else {
    auto p = x.cbegin();
    for (int i = 0; i < k; ++i) ++p;
    x.insert(p, v);
  }
}

// V is evil::Val or GVal: constructible from int, ==, <=>.
template <class X>
bool values() {
  using V = typename X::value_type;
  using It = evil::Iter<V>;
  V arr[6] = {V(5), V(3), V(1), V(4), V(1), V(2)};
  auto as_ints = [](const X& x, std::initializer_list<int> il) {
    auto it = x.begin();
    for (int i : il) {
      if (it == x.end() || !(*it == V(i))) return false;
      ++it;
    }
    return it == x.end();
  };

  X a;
  X b(3);
  X c(2, V(7));
  X d(It(arr), It(arr + 6));
  X e{V(1), V(2)};
  X f(std::from_range, arr);
  if (!as_ints(c, {7, 7}) || !as_ints(d, {5, 3, 1, 4, 1, 2}) || !(d == f) || length(b) != 3) return false;
  X g(d);
  X h(std::move(g));
  a = h;
  a = std::move(h);
  a = {V(9), V(8)};
  a.assign(It(arr), It(arr + 3));
  a.assign(4, V(6));
  a.assign({V(1)});
  a.assign_range(arr);
  if (!(a == d) || a != d || a < d || !(a <= d) || (a <=> d) != 0) return false;
  a.swap(e);
  std::swap(a, e);
  std::ranges::swap(a, e);
  if (!as_ints(a, {1, 2})) return false;

  insert_one(a, 1, V(10));
  if constexpr (is_forward_list<X>) {
    a.emplace_after(a.cbefore_begin(), 11);
    a.insert_after(a.cbefore_begin(), 2, V(12));
    a.insert_after(a.cbefore_begin(), It(arr), It(arr + 2));
    a.insert_after(a.cbefore_begin(), {V(13)});
    a.insert_range_after(a.cbefore_begin(), arr);
    a.erase_after(a.cbefore_begin());
    a.erase_after(a.cbefore_begin(), std::next(a.cbegin(), 3));
  } else {
    a.emplace(a.cbegin(), 11);
    a.insert(a.cbegin(), 2, V(12));
    a.insert(a.cbegin(), It(arr), It(arr + 2));
    a.insert(a.cbegin(), {V(13)});
    a.insert_range(a.cbegin(), arr);
    a.erase(a.cbegin());
    a.erase(a.cbegin(), std::next(a.cbegin(), 3));
  }
  if constexpr (requires { a.push_back(V(1)); }) {
    a.push_back(V(20));
    a.emplace_back(21);
    a.append_range(arr);
    a.pop_back();
  }
  if constexpr (requires { a.push_front(V(1)); }) {
    a.push_front(V(22));
    a.emplace_front(23);
    a.prepend_range(arr);
    a.pop_front();
  }
  if constexpr (requires { a.reserve(1); }) {
    a.reserve(200);
    a.shrink_to_fit();
  }
  if constexpr (requires { a[0]; }) {
    if (!(a[0] == a.at(0))) return false;
  }
  if constexpr (requires { a.data(); }) {
    if (a.data() != std::to_address(a.begin())) return false;
  }
  if constexpr (requires { a.rbegin(); }) {
    if (!(*a.rbegin() == *std::prev(std::next(a.begin(), length(a))))) return false;
  }
  a.resize(40);
  a.resize(45, V(3));
  a.resize(10);
  if (length(a) != 10) return false;
  std::erase(a, V(3));
  std::erase_if(a, [](const V& v) { return v == V(0); });

  if constexpr (requires { a.sort(); }) {  // list, forward_list operations
    X s(std::from_range, arr);
    s.sort();
    if (!as_ints(s, {1, 1, 2, 3, 4, 5})) return false;
    s.sort(std::greater<>());
    s.reverse();
    s.unique();
    if (!as_ints(s, {1, 2, 3, 4, 5})) return false;
    X t{V(0), V(6)};
    s.merge(t);
    s.merge(X{V(7)}, std::less<>());
    if (!as_ints(s, {0, 1, 2, 3, 4, 5, 6, 7})) return false;
    s.remove(V(7));
    s.remove_if([](const V& v) { return v == V(0); });
    s.unique(std::equal_to<>());
    X u{V(8), V(9)};
    if constexpr (is_forward_list<X>) {
      s.splice_after(s.cbefore_begin(), u);
    } else {
      s.splice(s.cend(), u);
    }
    if (length(s) != 8) return false;
  }
  a.clear();
  return a.begin() == a.end();
}

// value_type is evil::Holder<evil::Incomplete>*.
template <class X>
bool pointers() {
  using P = typename X::value_type;
  // Iterator pairs are plain P*: an evil::Iter<P> argument would make overload resolution check
  // the from_range constructor's constraint, whose ranges::begin performs ADL on the iterator
  // as specified ([range.access.begin]/2.6), completing Holder<Incomplete> regardless of the
  // library.
  alignas(64) static char storage[128];  // distinct, suitably aligned addresses; never dereferenced
  P p[4] = {nullptr, static_cast<P>(static_cast<void*>(storage)), static_cast<P>(static_cast<void*>(storage + 64)),
            nullptr};
  X a(p, p + 4);
  X b(3);
  X c(2, p[1]);
  X d(std::from_range, p);
  X e{p[2], p[1]};
  // No operator expressions on the containers: one would perform ADL for the container type
  // itself ([over.match.oper]/3.2), whose associated entities include Holder<Incomplete>
  // ([basic.lookup.argdep]/3) -- that would be the test's doing, not the library's (and
  // inplace_vector's comparisons are hidden friends, found only that way).
  auto same = [](const X& x, const X& y) {
    auto i = x.begin(), j = y.begin();
    for (; i != x.end() && j != y.end(); ++i, ++j)
      if (*i != *j) return false;
    return i == x.end() && j == y.end();
  };
  if (!same(a, d) || length(b) != 3) return false;
  X g(a);
  X h(std::move(g));
  b = h;
  b = std::move(h);
  b = {p[1]};
  b.assign(p, p + 2);
  b.assign(2, p[2]);
  b.assign_range(p);
  b.swap(c);
  std::swap(b, c);
  insert_one(b, 1, p[2]);
  if constexpr (is_forward_list<X>) {
    b.insert_after(b.cbefore_begin(), p, p + 4);
    b.insert_range_after(b.cbefore_begin(), p);
    b.erase_after(b.cbefore_begin());
  } else {
    b.insert(b.cbegin(), p, p + 4);
    b.insert_range(b.cbegin(), p);
    b.erase(b.cbegin());
  }
  if constexpr (requires { b.push_back(p[0]); }) {
    b.push_back(p[1]);
    b.append_range(p);
    b.pop_back();
  }
  if constexpr (requires { b.push_front(p[0]); }) {
    b.push_front(p[1]);
    b.prepend_range(p);
    b.pop_front();
  }
  if constexpr (requires { b.reserve(1); }) {
    b.reserve(100);
    b.shrink_to_fit();
  }
  b.resize(30);
  b.resize(35, p[2]);
  P const null = nullptr;
  std::erase(b, null);
  std::erase_if(b, [&](P q) { return q == p[2]; });
  for (P q : b)
    if (q != p[1]) return false;
  if constexpr (requires { b.sort(); }) {
    X s(std::from_range, p);
    s.sort(std::less<P>());
    s.unique();
    s.remove(null);
    X t{p[1]};
    s.merge(t, std::less<P>());
    s.reverse();
    if (length(s) != 3) return false;
  }
  b.clear();
  return b.begin() == b.end();
}

}  // namespace reqs::adl_robustness_seq
