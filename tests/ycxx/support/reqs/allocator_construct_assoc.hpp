// Generic run-time check, instantiated per node-based associative / unordered associative
// container: every key and mapped object (every value_type subobject) is constructed through
// allocator_traits<A>::construct and destroyed through allocator_traits<A>::destroy, with an
// allocator equal to get_allocator() (rebound to the node type).
// [container.alloc.reqmts]/2: the element requirements are stated only in terms of
// allocator_traits<A>::construct(m, p, args) (Cpp17DefaultInsertable, Cpp17MoveInsertable,
// Cpp17CopyInsertable, Cpp17EmplaceConstructible) and allocator_traits<A>::destroy(m, p)
// (Cpp17Erasable); Note 2: "A container calls allocator_traits<A>::construct(m, p, args) to
// construct an element at p using args, with m == get_allocator()". [container.reqmts]/64.
// [associative.reqmts.general]/47-56, [unord.req.general]: a_uniq.emplace(args) "Inserts a
// value_type object t constructed with std::forward<Args>(args)..." with the precondition
// "value_type is Cpp17EmplaceConstructible into X from args" -- so t, even when it is not
// kept, is constructed only through the allocator. [map.modifiers]/7-31,
// [unord.map.modifiers]: try_emplace / insert_or_assign construct value_type with
// piecewise_construct (Cpp17EmplaceConstructible); [map.access], [unord.map.elem]:
// operator[]. Reuses Tracked / ConstructAlloc / counters of
// support/reqs/allocator_construct_seq.hpp: Tracked counts the constructions and
// destructions that happen outside ConstructAlloc::construct / destroy.
#pragma once
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
// The sequence harness names std::erase (qualified, so it must be declared where the template
// is defined); the associative containers declare only erase_if, so bring in a declaration.
#include <vector>
#include "reqs/allocator_construct_seq.hpp"
#include "check.hpp"  // dprintf

namespace reqs::allocator_construct_assoc {

using reqs::allocator_construct_seq::ConstructAlloc;
using reqs::allocator_construct_seq::counters;
using reqs::allocator_construct_seq::Counters;
using reqs::allocator_construct_seq::Tracked;
using reqs::allocator_construct_seq::Val;

struct TrackedHash {
  std::size_t operator()(const Tracked& t) const { return static_cast<std::size_t>(t.v) * 2654435761u; }
};

template <class X>
concept is_map = requires { typename X::mapped_type; };
template <class X>
concept is_unordered = requires { typename X::hasher; };
template <class X>
concept is_multi = std::is_same_v<decltype(std::declval<X&>().insert(std::declval<const typename X::value_type&>())),
                                  typename X::iterator>;

template <class X>
const Tracked& key_of(const typename X::value_type& v) {
  if constexpr (is_map<X>) return v.first;
  else return v;
}

// The source values: pair<Val, Val> for maps, Val for sets (converted to value_type inside
// allocator construct calls).
template <class X>
auto src(int k) {
  if constexpr (is_map<X>) return std::pair<Val, Val>{Val{k}, Val{k + 100}};
  else return Val{k};
}

// X(args..., a): the unordered containers take a bucket count before the allocator.
template <class X, class... Args>
X with_alloc(const typename X::allocator_type& a, Args&&... args) {
  if constexpr (is_unordered<X>) return X(std::forward<Args>(args)..., 0, a);
  else return X(std::forward<Args>(args)..., a);
}

template <class X>
bool operations() {
  using A = typename X::allocator_type;
  using S = decltype(src<X>(0));
  S arr[5] = {src<X>(1), src<X>(2), src<X>(3), src<X>(4), src<X>(5)};
  X keys = with_alloc<X>(A(1), arr, arr + 5);  // elements whose keys / values the test passes as lvalues
  auto el = [&](int k) -> const typename X::value_type& {  // the element with key k (1..5)
    for (auto it = keys.begin(); it != keys.end(); ++it)
      if (key_of<X>(*it).v == k) return *it;
    return *keys.begin();
  };
  auto key = [&](int k) -> const Tracked& { return key_of<X>(el(k)); };

  X a(A(1));
  X fr = with_alloc<X>(A(1), std::from_range, arr);
  counters.expected_outside += is_map<X> ? 4 : 2;
  X il([&] {
    using IL = std::initializer_list<typename X::value_type>;
    if constexpr (is_map<X>) return with_alloc<X>(A(1), IL{{1, 101}, {2, 102}});  // pair(U1&&, U2&&)
    else return with_alloc<X>(A(1), IL{1, 2});
  }());
  X cp(keys);
  X cpa(keys, A(2));
  X mv(std::move(cp));
  X mva(std::move(cpa), A(3));  // unequal allocator: element-wise
  if (fr.size() != 5 || il.size() != 2 || mv.size() != 5 || mva.size() != 5) return false;
  a = keys;
  a = il;
  a = std::move(mva);  // unequal, not propagating: element-wise
  if (a.size() != 5) return false;

  // insertion: copies and moves of existing elements, emplacement from Val, duplicates
  X b(A(1));
  b.insert(el(1));
  typename X::value_type moved_from = el(2);  // made by the test
  counters.expected_outside += is_map<X> ? 2 : 1;
  b.insert(std::move(moved_from));
  b.insert(b.end(), el(3));
  if constexpr (is_map<X>) {
    b.emplace(Val{6}, Val{106});
    b.emplace(Val{6}, Val{206});  // duplicate for unique keys: t is still built by the allocator
    b.emplace_hint(b.begin(), Val{7}, Val{107});
    b.insert(std::pair<Val, Val>{Val{8}, Val{108}});  // insert(P&&): emplace
    b.emplace(std::piecewise_construct, std::forward_as_tuple(9), std::forward_as_tuple(109));
  } else {
    b.emplace(Val{6});
    b.emplace(Val{6});
    b.emplace_hint(b.begin(), Val{7});
    b.emplace(8);
    b.emplace(9);
  }
  b.insert(arr, arr + 3);
  b.insert_range(arr);
  if constexpr (is_map<X> && !is_multi<X>) {
    b.try_emplace(key(4), Val{104});
    b.try_emplace(key(4), Val{999});  // exists: nothing constructed
    b.try_emplace(b.end(), key(5), Val{105});
    Tracked k10 = Val{10}, m10 = Val{110};  // made by the test
    counters.expected_outside += 2;
    b.try_emplace(std::move(k10), Val{110});
    b.insert_or_assign(key(1), el(2).second);  // assigns
    b.insert_or_assign(b.end(), el(3).first, std::move(m10));
    b[key(1)] = Val{101};  // existing
    b[key(4)];
    Tracked k11 = Val{11};
    counters.expected_outside += 1;
    b[std::move(k11)];  // new: key moved in, mapped value-initialized
  }
  if (!b.contains(key(1)) || b.count(key(3)) == 0) return false;

  // erasure, node handles, merge
  b.erase(key(1));
  b.erase(b.begin());
  b.erase(b.begin(), std::next(b.begin(), 2));
  auto nh = b.extract(b.begin());
  X c(A(1));
  c.insert(std::move(nh));
  c.insert(c.end(), b.extract(b.begin()));
  b.merge(c);
  c.merge(fr);
  std::erase_if(b, [](const typename X::value_type& v) { return key_of<X>(v).v == 6; });
  if constexpr (is_unordered<X>) {
    b.rehash(100);
    b.reserve(5);
  }
  X d = with_alloc<X>(A(1), arr, arr + 2);
  b.swap(d);  // equal allocators
  b.clear();
  return b.empty();
}

template <class X>
bool test() {
  counters = Counters();
  bool ok = operations<X>();
  if (!ok) return false;
  if (counters.outside_constructs != counters.expected_outside) {
    dprintf(2, "allocator_construct_assoc: %d constructions outside the allocator (expected %d)\n",
            counters.outside_constructs, counters.expected_outside);
    return false;
  }
  if (counters.outside_destroys != counters.expected_outside) {
    dprintf(2, "allocator_construct_assoc: %d destructions outside the allocator (expected %d)\n",
            counters.outside_destroys, counters.expected_outside);
    return false;
  }
  return counters.live == 0 && counters.constructs > 50 && counters.constructs == counters.destroys;
}

}  // namespace reqs::allocator_construct_assoc
