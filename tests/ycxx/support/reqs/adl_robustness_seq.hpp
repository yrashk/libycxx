// Generic check, instantiated per sequence container: the container works with element
// types whose associated namespaces contain poisoned function templates
// (support/adl_poison.hpp).
// [contents]/3: "Whenever an unqualified name other than swap, make_error_code,
// make_error_condition, from_stream, or submdspan_mapping is used in the specification of a
// declaration D in [library] ... its meaning is established as-if by performing unqualified
// name lookup in the context of D", so argument-dependent lookup must not find evil::move,
// evil::copy, ::addressof, ... from inside the library. The element requirements
// ([container.alloc.reqmts]/2, [utility.arg.requirements]) and the iterator requirements
// ([iterator.requirements]) do not include a unary & or a comma operator, so evil::Val
// (both deleted) and evil::Iter (both deleted) must be usable.
// (Elements of type Holder<Incomplete>*, as in algorithm/adl_incomplete_holder, are not used:
// any operator expression on the container's iterators in the test itself, e.g. it != end(),
// performs ADL over the iterator type, whose associated entities include Holder<Incomplete>
// for any iterator design, including a plain pointer -- [basic.lookup.argdep]/3.)
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
    a.reserve(100);  // <= inplace_vector capacity used by the tests
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

}  // namespace reqs::adl_robustness_seq
