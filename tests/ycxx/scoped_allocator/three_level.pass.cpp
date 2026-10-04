// scoped_allocator_adaptor with three allocators, each of a different type:
// [allocator.adaptor.members]/11: select_on_container_copy_construction() returns an adaptor
// "where each allocator a1 within the adaptor is initialized with
// allocator_traits<A1>::select_on_container_copy_construction(a2)" -- per level, so a level
// whose allocator has no such member keeps its state while the others apply their own.
// [scoped.adaptor.operators]/1: == compares outer_allocator() and (recursively)
// inner_allocator(), so adaptors differing only in the innermost allocator are unequal
// (different outer types: equality_rebound).
// [allocator.adaptor.types]/2-4: POCCA/POCMA/POCS are true if true "for any A in the set of
// OuterAlloc and InnerAllocs..."; a container whose allocator is the adaptor therefore
// replaces the whole adaptor (all three levels, including an outer allocator that would not
// propagate on its own) on copy/move assignment and swap ([container.alloc.reqmts]).
// [allocator.adaptor.members]/9: construct passes inner_allocator() by uses-allocator
// construction; for a pair (piecewise or not) each member gets inner_allocator()
// ([allocator.uses.construction]/6-16), and a nested container passes its own inner
// allocator one level further down.
#include <scoped_allocator>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

// Stateful allocator with a tag (so the three levels have distinct types) and selectable
// traits. Mode: 0 = no select_on_container_copy_construction, 1 = it returns id + 100.
template <class T, int Tag, bool Prop = false, int Mode = 0>
struct A {
  using value_type = T;
  using propagate_on_container_copy_assignment = std::bool_constant<Prop>;
  using propagate_on_container_move_assignment = std::bool_constant<Prop>;
  using propagate_on_container_swap = std::bool_constant<Prop>;
  using is_always_equal = std::false_type;
  template <class U>
  struct rebind { using other = A<U, Tag, Prop, Mode>; };
  int id = 0;
  A() = default;
  explicit A(int i) : id(i) {}
  template <class U>
  A(const A<U, Tag, Prop, Mode>& o) : id(o.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  A select_on_container_copy_construction() const
    requires(Mode == 1)
  {
    return A(id + 100);
  }
  template <class U>
  friend bool operator==(const A& a, const A<U, Tag, Prop, Mode>& b) { return a.id == b.id; }
};

template <class T> using A1 = A<T, 1, false, 1>;   // outer: never propagates, selects
template <class T> using A2 = A<T, 2, false, 0>;   // middle: keeps on copy construction
template <class T> using A3 = A<T, 3, true, 1>;    // inner: propagates, selects

using S3 = std::scoped_allocator_adaptor<A1<int>, A2<int>, A3<int>>;
static_assert(S3::propagate_on_container_copy_assignment::value);
static_assert(S3::propagate_on_container_move_assignment::value);
static_assert(S3::propagate_on_container_swap::value);
static_assert(S3::inner_allocator_type::propagate_on_container_swap::value);
static_assert(!std::scoped_allocator_adaptor<A1<int>, A2<int>>::propagate_on_container_swap::value);

static int ids(const S3& s) {
  return s.outer_allocator().id * 10000 + s.inner_allocator().outer_allocator().id * 100 +
         s.inner_allocator().inner_allocator().outer_allocator().id;
}

// Element types: the elements of a VElem get inner_allocator() = adaptor<A2, A3>, so the
// pair's string uses A2 and its vector uses adaptor<A2, A3>, whose strings get A3.
using Str2 = std::basic_string<char, std::char_traits<char>, A2<char>>;
using Str = std::basic_string<char, std::char_traits<char>, A3<char>>;
using Inner = std::scoped_allocator_adaptor<A2<Str>, A3<Str>>;
using VStr = std::vector<Str, Inner>;
using Elem = std::pair<Str2, VStr>;
// The inner allocator types are those of the nested containers' adaptor, as the converting
// constructor of [allocator.adaptor.cnstr]/6 only changes the outer type.
using Outer = std::scoped_allocator_adaptor<A1<Elem>, A2<Elem>, A3<Str>>;
using VElem = std::vector<Elem, Outer>;

static const char* const longs = "a string that is too long for any small-string buffer, 0123456789";

static bool levels(const VElem& v, int outer, int mid, int inner) {
  if (v.get_allocator().outer_allocator().id != outer) return false;
  for (const Elem& e : v) {
    if (e.first.get_allocator().id != mid) return false;  // pair member: inner_allocator()'s outer
    if (e.second.get_allocator().outer_allocator().id != mid) return false;
    if (e.second.get_allocator().inner_allocator().outer_allocator().id != inner) return false;
    for (const Str& s : e.second)
      if (s.get_allocator().id != inner) return false;
  }
  return true;
}

int main() {
  S3 s(A1<int>(1), A2<int>(2), A3<int>(3));
  CHECK(ids(s) == 10203);

  // Per-level selection.
  S3 c = s.select_on_container_copy_construction();
  CHECK(c.outer_allocator().id == 101);
  CHECK(c.inner_allocator().outer_allocator().id == 2);
  CHECK(c.inner_allocator().inner_allocator().outer_allocator().id == 103);
  CHECK(ids(std::allocator_traits<S3>::select_on_container_copy_construction(s)) == ids(c));

  // Equality, recursively down to the innermost allocator.
  CHECK(s == S3(A1<int>(1), A2<int>(2), A3<int>(3)));
  CHECK(s != S3(A1<int>(1), A2<int>(2), A3<int>(4)));
  CHECK(s != S3(A1<int>(1), A2<int>(5), A3<int>(3)));
  CHECK(s != S3(A1<int>(6), A2<int>(2), A3<int>(3)));

  // Construction through a container, piecewise and not.
  VElem v(Outer(A1<Elem>(1), A2<Elem>(2), A3<Str>(3)));
  v.emplace_back(std::piecewise_construct, std::forward_as_tuple(longs), std::forward_as_tuple(3, Str(longs)));
  v.emplace_back(longs, VStr(2, Str(longs)));
  v.emplace_back();
  v.back().second.emplace_back(longs);
  Elem outside(Str2(longs, A2<char>(9)), VStr(1, Str(longs, A3<char>(9)), Inner(A2<Str>(9), A3<Str>(9))));
  v.push_back(outside);
  v.push_back(std::move(outside));
  v.resize(8);
  CHECK(levels(v, 1, 2, 3));
  CHECK(v[0].second.size() == 3 && v[0].second[2] == longs && v[3].second[0] == longs);

  // Copy construction: per-level selection reaches every element.
  VElem copy(v);
  CHECK(levels(copy, 101, 2, 103));
  CHECK(copy.size() == 8 && copy[0].second[1] == longs);

  // Copy assignment: POCCA of the adaptor is true (from A3), so the whole adaptor is copied,
  // including A1's id although A1 alone does not propagate.
  VElem target(Outer(A1<Elem>(7), A2<Elem>(8), A3<Str>(9)));
  target = v;
  CHECK(target.get_allocator().outer_allocator().id == 1);
  CHECK(target.get_allocator().inner_allocator().inner_allocator().outer_allocator().id == 3);
  CHECK(levels(target, 1, 2, 3));

  // Move assignment and swap likewise.
  VElem target2(Outer(A1<Elem>(7), A2<Elem>(8), A3<Str>(9)));
  target2 = std::move(copy);
  CHECK(levels(target2, 101, 2, 103) && target2.size() == 8);
  VElem other(Outer(A1<Elem>(4), A2<Elem>(5), A3<Str>(6)));
  other.emplace_back(longs, VStr());
  swap(other, target);
  CHECK(levels(other, 1, 2, 3) && other.size() == 8);
  CHECK(target.get_allocator().outer_allocator().id == 4 && target.size() == 1);
  CHECK(levels(target, 4, 5, 6));
  return 0;
}
