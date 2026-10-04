// Generic check, instantiated per sequence container X = C<T, FancyAlloc<T>>: the container
// works with an allocator whose pointer is a class type (support/fancy_ptr.hpp).
// [container.reqmts]/64: "all containers defined in this Clause obtain memory using an
// allocator", Note 2: "containers and iterators do not store references to allocated
// elements other than through the allocator's pointer type, i.e., as objects of type P or
// pointer_traits<P>::template rebind<unspecified>, where P is
// allocator_traits<allocator_type>::pointer." [allocator.requirements.general]/2-5 allows P
// to be any type meeting Cpp17NullablePointer, Cpp17RandomAccessIterator and
// contiguous_iterator. The member types pointer / const_pointer of vector, deque, list and
// forward_list are allocator_traits<Allocator>::pointer / const_pointer ([vector.overview],
// [deque.overview], [list.overview], [forward.list.overview]); vector<bool>'s are
// implementation-defined ([vector.bool.pspc]). [vector.data]: data() returns T*.
// Exercised: construction forms, copy / move / assignment / swap, insertion and erasure at
// every position with growth, resize, clear, reverse iteration, erase_if, and the list /
// forward_list operations, the deque front operations and the vector capacity members.
#pragma once
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "fancy_ptr.hpp"

namespace reqs::fancy_alloc_seq {

template <class X>
constexpr bool is_forward_list = requires(X& x) { x.before_begin(); };

template <class X, class V>
constexpr void insert_one(X& x, int k, V&& v) {
  if constexpr (is_forward_list<X>) {
    auto p = x.cbefore_begin();
    for (int i = 0; i < k; ++i) ++p;
    x.insert_after(p, std::forward<V>(v));
  } else {
    x.insert(cnth(x, k), std::forward<V>(v));
  }
}

template <class X>
constexpr void erase_one(X& x, int k) {
  if constexpr (is_forward_list<X>) {
    auto p = x.cbefore_begin();
    for (int i = 0; i < k; ++i) ++p;
    x.erase_after(p);
  } else {
    x.erase(cnth(x, k));
  }
}

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  static_assert(std::is_same_v<typename X::allocator_type, FancyAlloc<T>>);
  if constexpr (!std::is_same_v<T, bool>) {
    static_assert(std::is_same_v<typename X::pointer, FancyPtr<T>>);
    static_assert(std::is_same_v<typename X::const_pointer, FancyPtr<const T>>);
  }
  X a = make<X>({0, 1, 2, 3, 4});
  X b(a);
  X c(std::move(b));
  X d(a, FancyAlloc<T>());
  X e({val<T>(7), val<T>(8)});
  X f(3, val<T>(9));
  if (!holds(c, {0, 1, 2, 3, 4}) || !holds(d, {0, 1, 2, 3, 4}) || !holds(e, {7, 8}) || !holds(f, {9, 9, 9}))
    return false;
  b = a;
  b = std::move(f);
  f = {val<T>(1)};
  b.swap(e);
  if (!holds(b, {7, 8}) || !holds(e, {9, 9, 9}) || !holds(f, {1})) return false;

  // growth at the front, the middle and the end
  for (int i = 10; i < 60; ++i) insert_one(a, 0, val<T>(i));
  for (int i = 60; i < 70; ++i) insert_one(a, 25, val<T>(i));
  if (count_elems(a) != 65) return false;
  for (int i = 0; i < 30; ++i) erase_one(a, 10);
  if (count_elems(a) != 35) return false;
  insert_one(a, 2, val<T>(70));
  if (!(*nth(a, 2) == val<T>(70))) return false;
  a.resize(100);
  a.resize(50, val<T>(71));
  a.resize(3);
  if (count_elems(a) != 3 || !(*nth(a, 2) == val<T>(70))) return false;
  std::erase_if(a, [](const T& x) { return x == val<T>(70); });
  if (!std::is_same_v<T, bool> && count_elems(a) != 2) return false;  // val<bool> is not distinct
  a.assign(4, val<T>(72));
  if (!holds(a, {72, 72, 72, 72})) return false;

  if constexpr (requires { a.rbegin(); }) {
    X r = make<X>({1, 2, 3});
    auto it = r.rbegin();
    if (!(*it == val<T>(3)) || !(*++it == val<T>(2))) return false;
  }
  if constexpr (requires { a.push_front(val<T>(0)); }) {
    X q;
    for (int i = 0; i < 40; ++i) q.push_front(val<T>(i));
    for (int i = 0; i < 39; ++i) q.pop_front();
    if (!holds(q, {0})) return false;
  }
  if constexpr (requires { a.reserve(1); }) {
    X v = make<X>({1, 2});
    v.reserve(100);
    if (v.capacity() < 100 || !holds(v, {1, 2})) return false;
    v.shrink_to_fit();
    if (!holds(v, {1, 2})) return false;
  }
  if constexpr (requires { a.data(); }) {
    static_assert(std::is_same_v<decltype(a.data()), T*>);
    static_assert(std::is_same_v<decltype(std::as_const(a).data()), const T*>);
    if (a.data() != std::addressof(*a.begin())) return false;
  }
  if constexpr (requires { a.sort(); }) {
    X s = make<X>({5, 1, 4, 2, 3, 2});
    s.sort();
    s.unique();
    if (!holds(s, {1, 2, 3, 4, 5})) return false;
    X t = make<X>({0, 6});
    s.merge(t);
    s.reverse();
    s.remove(val<T>(6));
    if (!holds(s, {5, 4, 3, 2, 1, 0}) || !(t.begin() == t.end())) return false;
    X u = make<X>({8, 9});
    if constexpr (is_forward_list<X>)
      s.splice_after(s.cbefore_begin(), u);
    else
      s.splice(s.cbegin(), u);
    if (!holds(s, {8, 9, 5, 4, 3, 2, 1, 0}) || !(u.begin() == u.end())) return false;
  }
  a.clear();
  return a.begin() == a.end();
}

}  // namespace reqs::fancy_alloc_seq
