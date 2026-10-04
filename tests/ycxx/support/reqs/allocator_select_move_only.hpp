// Generic checks, instantiated per allocator-aware sequence container, for two parts of
// [container.reqmts]/64 and [container.alloc.reqmts] that support/reqs/allocator_aware.hpp
// does not reach (its allocator's select_on_container_copy_construction returns a plain
// copy, and its element types are copyable):
//
// copy_select<C>(): "Copy constructors for these container types obtain an allocator by
// calling allocator_traits<allocator_type>::select_on_container_copy_construction on the
// allocator belonging to the container being copied." With an allocator whose
// select_on_container_copy_construction returns a *different* allocator (id + 100): X u(t)
// and X u = t use it; X u(t, m) uses m (/14); X u(rv) moves the allocator (/16); a = t keeps
// a's allocator when propagate_on_container_copy_assignment is false (/64); the copy
// constructor of the elements is used only (no allocator replacement on elements).
//
// move_only<C>(): with unequal allocators and propagate_on_container_move_assignment false,
// "a = rv" needs only "T is Cpp17MoveInsertable into X and Cpp17MoveAssignable" (/26) and
// leaves a equal to rv's former value (/28) with a's allocator; "X u(rv, m)" needs only
// Cpp17MoveInsertable (/18) and gives rv's elements with allocator m (/19). So a move-only T
// must compile and work, both when a is shorter and when it is longer than rv. With
// propagate_on_container_move_assignment true, a takes rv's allocator (/64).
//
// C<A> names the container type for allocator A (rebound to the element type by the test).
#pragma once
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "move_only_elem.hpp"

namespace reqs::allocator_select_move_only {

template <class T, bool MoveProp>
struct SelAlloc {
  using value_type = T;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::bool_constant<MoveProp>;
  using propagate_on_container_swap = std::false_type;
  using is_always_equal = std::false_type;
  template <class U>
  struct rebind {
    using other = SelAlloc<U, MoveProp>;
  };
  int id = 0;
  constexpr SelAlloc() noexcept = default;
  constexpr explicit SelAlloc(int i) noexcept : id(i) {}
  template <class U>
  constexpr SelAlloc(const SelAlloc<U, MoveProp>& o) noexcept : id(o.id) {}
  constexpr T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  constexpr void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  constexpr SelAlloc select_on_container_copy_construction() const { return SelAlloc(id + 100); }
  template <class U>
  friend constexpr bool operator==(const SelAlloc& a, const SelAlloc<U, MoveProp>& b) noexcept {
    return a.id == b.id;
  }
};

template <template <class> class C, class T>
using X_of = C<SelAlloc<T, false>>;

template <class X>
constexpr X filled(int id, std::initializer_list<int> vs) {
  X x{typename X::allocator_type(id)};
  for (int v : vs) append_to(x, v);
  return x;
}

template <template <class> class C, class T>
constexpr bool copy_select() {
  using X = X_of<C, T>;
  using A = typename X::allocator_type;
  static_assert(std::is_same_v<typename X::value_type, T>);
  const X t = filled<X>(1, {0, 1, 1, 0, 1, 0, 0, 1, 1, 1});
  X u(t);
  if (u.get_allocator().id != 101 || !values_are(u, {0, 1, 1, 0, 1, 0, 0, 1, 1, 1})) return false;
  X v = t;
  if (v.get_allocator().id != 101) return false;
  X w(t, A(5));
  if (w.get_allocator().id != 5 || !values_are(w, {0, 1, 1, 0, 1, 0, 0, 1, 1, 1})) return false;
  X m(std::move(v));  // the allocator is moved, not selected
  if (m.get_allocator().id != 101 || !values_are(m, {0, 1, 1, 0, 1, 0, 0, 1, 1, 1})) return false;
  X a = filled<X>(7, {1, 0});  // values usable for every T, bool included
  a = t;
  if (a.get_allocator().id != 7 || !values_are(a, {0, 1, 1, 0, 1, 0, 0, 1, 1, 1})) return false;
  X copy_of_copy(u);  // selection applies to the copied container's current allocator
  return copy_of_copy.get_allocator().id == 201;
}

template <template <class> class C, bool MoveProp>
constexpr bool move_only_case() {
  using X = C<SelAlloc<MOElem, MoveProp>>;
  using A = typename X::allocator_type;
  {
    X a = filled<X>(1, {1, 2});
    X rv = filled<X>(2, {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21});
    a = std::move(rv);  // a shorter than rv
    if (!values_are(a, {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21})) return false;
    if (a.get_allocator().id != (MoveProp ? 2 : 1)) return false;
  }
  {
    X a = filled<X>(1, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13});
    X rv = filled<X>(3, {30, 31});
    a = std::move(rv);  // a longer than rv
    if (!values_are(a, {30, 31}) || a.get_allocator().id != (MoveProp ? 3 : 1)) return false;
    X empty{A(4)};
    a = std::move(empty);
    if (!(a.begin() == a.end())) return false;
  }
  {
    X rv = filled<X>(1, {5, 6, 7});
    X u(std::move(rv), A(9));  // unequal: element-wise moves
    if (!values_are(u, {5, 6, 7}) || u.get_allocator().id != 9) return false;
    X same(std::move(u), A(9));  // equal allocators
    if (!values_are(same, {5, 6, 7}) || same.get_allocator().id != 9) return false;
    X moved(std::move(same));
    if (!values_are(moved, {5, 6, 7}) || moved.get_allocator().id != 9) return false;
  }
  return true;
}

template <template <class> class C>
constexpr bool move_only() {
  return move_only_case<C, false>() && move_only_case<C, true>();
}

}  // namespace reqs::allocator_select_move_only
