// Generic run-time check, instantiated per node-based associative / unordered associative
// container with a stateful allocator: the allocator semantics of node handles.
//   [container.node.overview]/2: "If a node handle is not empty, then it contains an
//     allocator that is equal to the allocator of the container when the element was
//     extracted. If a node handle is empty, it contains no allocator."
//   [container.node.cons]/1: the move constructor moves ptr_ and alloc_ and leaves nh empty;
//     /3: move assignment destroys the owned element with ator-traits::destroy and
//     deallocates the node, takes nh.ptr_, and "If !alloc_ is true or
//     ator-traits::propagate_on_container_move_assignment::value is true, move assigns
//     nh.alloc_ to alloc_"; nh is left empty; /5: throws nothing.
//   [container.node.dtor]: "If ptr_ != nullptr, destroys the value_type subobject ... by
//     calling ator-traits::destroy, then deallocates ptr_".
//   [container.node.observers]: get_allocator() returns *alloc_; operator bool / empty().
//   [container.node.modifiers]/1-2: swap exchanges ptr_ and, "If !alloc_ is true, or
//     !nh.alloc_ is true, or ator-traits::propagate_on_container_swap::value is true",
//     the allocators; noexcept(propagate_on_container_swap || is_always_equal).
//   [associative.reqmts.general]/84-99, [unord.req.general]: extract and insert(nh) move
//     the node without constructing, destroying, allocating or deallocating anything.
// X's allocator is NAlloc<value_type, POCMA, POCS>.
#pragma once
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace reqs::node_handle_alloc {

struct Counts {
  int allocs = 0, deallocs = 0, constructs = 0, destroys = 0;
  friend bool operator==(const Counts&, const Counts&) = default;
};
inline Counts counts;

template <class T, bool POCMA, bool POCS>
struct NAlloc {
  using value_type = T;
  using propagate_on_container_move_assignment = std::bool_constant<POCMA>;
  using propagate_on_container_swap = std::bool_constant<POCS>;
  template <class U>
  struct rebind {
    using other = NAlloc<U, POCMA, POCS>;
  };
  int id = 0;
  NAlloc() = default;
  explicit NAlloc(int i) : id(i) {}
  template <class U>
  NAlloc(const NAlloc<U, POCMA, POCS>& o) noexcept : id(o.id) {}
  T* allocate(std::size_t n) {
    ++counts.allocs;
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) {
    ++counts.deallocs;
    std::allocator<T>{}.deallocate(p, n);
  }
  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ++counts.constructs;
    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
  }
  template <class U>
  void destroy(U* p) {
    ++counts.destroys;
    p->~U();
  }
  template <class U>
  friend bool operator==(const NAlloc& a, const NAlloc<U, POCMA, POCS>& b) noexcept {
    return a.id == b.id;
  }
};

template <class X>
concept is_map = requires { typename X::mapped_type; };

template <class X>
X make(int id, std::initializer_list<int> ks) {
  X x{typename X::allocator_type(id)};
  for (int k : ks) {
    if constexpr (is_map<X>) x.emplace(k, k + 100);
    else x.emplace(k);
  }
  return x;
}

template <class N>
int key_in(const N& nh) {
  if constexpr (requires { nh.key(); }) return nh.key();
  else return nh.value();
}

template <class X>
bool test() {
  using N = typename X::node_type;
  using A = typename X::allocator_type;
  constexpr bool pocma = std::allocator_traits<A>::propagate_on_container_move_assignment::value;
  constexpr bool pocs = std::allocator_traits<A>::propagate_on_container_swap::value;
  static_assert(std::is_same_v<typename N::allocator_type, A>);
  static_assert(std::is_nothrow_default_constructible_v<N> && std::is_nothrow_move_constructible_v<N>);
  static_assert(!std::is_copy_constructible_v<N> && !std::is_copy_assignable_v<N>);
  static_assert(!std::is_convertible_v<N, bool> && std::is_constructible_v<bool, N>);
  static_assert(noexcept(std::declval<const N&>().empty()) && noexcept(static_cast<bool>(std::declval<const N&>())));
  static_assert(noexcept(std::declval<N&>().swap(std::declval<N&>())) == pocs);  // is_always_equal is false
  static_assert(noexcept(swap(std::declval<N&>(), std::declval<N&>())) == pocs);

  X a = make<X>(7, {1, 2, 3, 4});
  X b = make<X>(8, {10, 20});
  // extract and insert(nh) neither construct, destroy, allocate nor deallocate
  Counts before = counts;
  N h1 = a.extract(a.find(1));
  N h2 = a.extract(2);
  if (!(counts == before)) return false;
  if (h1.empty() || !h1 || h1.get_allocator().id != 7 || key_in(h1) != 1) return false;
  N hb = b.extract(10);
  if (hb.get_allocator().id != 8) return false;

  // move construction: the allocator goes along, the source is empty
  N m(std::move(h2));
  if (!h2.empty() || static_cast<bool>(h2) || m.empty() || m.get_allocator().id != 7 || key_in(m) != 2) return false;
  // move assignment into an empty handle: takes the allocator whatever POCMA says
  N e;
  e = std::move(m);
  if (!m.empty() || e.empty() || e.get_allocator().id != 7 || key_in(e) != 2) return false;
  if (!(counts == before)) return false;
  // move assignment into a non-empty handle with an equal allocator: the old element is
  // destroyed and its node deallocated through the allocator
  N h3 = a.extract(3);
  h3 = std::move(e);
  if (!e.empty() || key_in(h3) != 2 || h3.get_allocator().id != 7) return false;
  if (counts.destroys != before.destroys + 1 || counts.deallocs != before.deallocs + 1) return false;
  if (counts.constructs != before.constructs || counts.allocs != before.allocs) return false;
  before = counts;
  if constexpr (pocma) {  // unequal allocators: allowed only when propagating; alloc_ follows
    h3 = std::move(hb);
    if (!hb.empty() || key_in(h3) != 10 || h3.get_allocator().id != 8) return false;
    if (counts.destroys != before.destroys + 1 || counts.deallocs != before.deallocs + 1) return false;
    b.insert(std::move(h3));  // back into a container with an equal allocator
    if (!h3.empty() || !b.contains(10)) return false;
  } else {
    b.insert(std::move(hb));
  }
  before = counts;
  // swap with an empty handle: the allocator moves too, whatever POCS says
  N empty;
  empty.swap(h1);
  if (!h1.empty() || empty.empty() || empty.get_allocator().id != 7 || key_in(empty) != 1) return false;
  swap(empty, h1);
  if (!empty.empty() || h1.get_allocator().id != 7) return false;
  // swap of two non-empty handles with equal allocators
  N h4 = a.extract(4);
  h1.swap(h4);
  if (key_in(h1) != 4 || key_in(h4) != 1 || h1.get_allocator().id != 7 || h4.get_allocator().id != 7) return false;
  if constexpr (pocs) {  // unequal allocators: allowed only when propagating; swapped
    N h20 = b.extract(20);
    h1.swap(h20);
    if (key_in(h1) != 20 || h1.get_allocator().id != 8 || h20.get_allocator().id != 7) return false;
    h1.swap(h20);
    b.insert(std::move(h20));
  }
  if (!(counts == before)) return false;
  // insert back; then the destructor of a non-empty handle destroys and deallocates
  a.insert(std::move(h4));
  a.insert(a.end(), std::move(h1));
  if (a.size() != 2 || !a.contains(1) || !a.contains(4) || !(counts == before)) return false;
  {
    N doomed = a.extract(1);
    N dead;  // an empty handle destroys nothing
  }
  if (counts.destroys != before.destroys + 1 || counts.deallocs != before.deallocs + 1) return false;
  return a.size() == 1 && b.size() == 2;
}

}  // namespace reqs::node_handle_alloc
