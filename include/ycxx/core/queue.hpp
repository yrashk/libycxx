// libycxx core: the container adaptors queue ([queue]) and priority_queue ([priority.queue]).
// priority_queue's default container, vector, is only named here (declared below without its
// default argument, which vector's definition supplies), so this header does not depend on
// vector's definition; <queue> includes <vector>. The formatter specializations
// ([container.adaptors.format]) are in ycxx/core/format_adaptors.hpp, which <queue> includes.
#pragma once

#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/algo_sort.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/deque.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, class _Allocator>
class vector;

// ---- [queue] ----
template <class _Tp, class _Container = deque<_Tp>>
class queue {
  static_assert(is_same_v<_Tp, typename _Container::value_type>,
                "std::queue: T must be Container::value_type ([container.adaptors.general])");

public:
  using value_type = typename _Container::value_type;
  using reference = typename _Container::reference;
  using const_reference = typename _Container::const_reference;
  using size_type = typename _Container::size_type;
  using container_type = _Container;

protected:
  _Container c;

public:
  // Constrained (an extension), so that is_default_constructible reflects Container's.
  constexpr queue()
    requires is_default_constructible_v<_Container>
      : queue(_Container()) {}
  constexpr explicit queue(const _Container& __cont) : c(__cont) {}
  constexpr explicit queue(_Container&& __cont) : c(static_cast<_Container&&>(__cont)) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr queue(_InputIterator first, _InputIterator last)
      : c(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last)) {}
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr queue(from_range_t, _Rp&& __rg) : c(__ycxx::__detail::__range_to<_Container>(static_cast<_Rp&&>(__rg))) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr explicit queue(const _Alloc& a) : c(a) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr queue(const _Container& __cont, const _Alloc& a) : c(__cont, a) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr queue(_Container&& __cont, const _Alloc& a) : c(static_cast<_Container&&>(__cont), a) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr queue(const queue& __q, const _Alloc& a) : c(__q.c, a) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr queue(queue&& __q, const _Alloc& a) : c(static_cast<_Container&&>(__q.c), a) {}
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && uses_allocator_v<_Container, _Alloc>
  constexpr queue(_InputIterator first, _InputIterator last, const _Alloc& __alloc)
      : c(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last), __alloc) {}
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp, class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr queue(from_range_t, _Rp&& __rg, const _Alloc& a)
      : c(__ycxx::__detail::__range_to<_Container>(static_cast<_Rp&&>(__rg), a)) {}

  [[nodiscard]] constexpr bool empty() const { return c.empty(); }
  constexpr size_type size() const { return c.size(); }
  constexpr reference front() { return c.front(); }
  constexpr const_reference front() const { return c.front(); }
  constexpr reference back() { return c.back(); }
  constexpr const_reference back() const { return c.back(); }
  constexpr void push(const value_type& __x) { c.push_back(__x); }
  constexpr void push(value_type&& __x) { c.push_back(static_cast<value_type&&>(__x)); }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void push_range(_Rp&& __rg) {
    if constexpr (requires { c.append_range(static_cast<_Rp&&>(__rg)); })
      c.append_range(static_cast<_Rp&&>(__rg));
    else
      ranges::copy(__rg, std::back_inserter(c));
  }
  template <class... _Args>
  constexpr decltype(auto) emplace(_Args&&... __args) {
    return c.emplace_back(static_cast<_Args&&>(__args)...);
  }
  constexpr void pop() { c.pop_front(); }
  constexpr void swap(queue& __q) noexcept(is_nothrow_swappable_v<_Container>) {
    ::__ycxx::__detail::__swap_adl::__do_swap(c, __q.c);
  }

  template <class _T1, class _C1>
  friend constexpr bool operator==(const queue<_T1, _C1>&, const queue<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator!=(const queue<_T1, _C1>&, const queue<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator<(const queue<_T1, _C1>&, const queue<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator>(const queue<_T1, _C1>&, const queue<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator<=(const queue<_T1, _C1>&, const queue<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator>=(const queue<_T1, _C1>&, const queue<_T1, _C1>&);
  template <class _T1, three_way_comparable _C1>
  friend constexpr compare_three_way_result_t<_C1> operator<=>(const queue<_T1, _C1>&, const queue<_T1, _C1>&);
};

template <class _Container>
  requires(!__ycxx::__detail::__qualifies_as_allocator<_Container>)
queue(_Container) -> queue<typename _Container::value_type, _Container>;
template <class _InputIterator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
queue(_InputIterator, _InputIterator) -> queue<__ycxx::__detail::__iter_value_type<_InputIterator>>;
template <ranges::input_range _Rp>
queue(from_range_t, _Rp&&) -> queue<ranges::range_value_t<_Rp>>;
template <class _Container, class _Allocator>
  requires(!__ycxx::__detail::__qualifies_as_allocator<_Container>) && uses_allocator_v<_Container, _Allocator>
queue(_Container, _Allocator) -> queue<typename _Container::value_type, _Container>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
queue(_InputIterator, _InputIterator, _Allocator)
    -> queue<__ycxx::__detail::__iter_value_type<_InputIterator>, deque<__ycxx::__detail::__iter_value_type<_InputIterator>, _Allocator>>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
queue(from_range_t, _Rp&&, _Allocator) -> queue<ranges::range_value_t<_Rp>, deque<ranges::range_value_t<_Rp>, _Allocator>>;

template <class _Tp, class _Container, class _Alloc>
struct uses_allocator<queue<_Tp, _Container>, _Alloc> : uses_allocator<_Container, _Alloc>::type {};

// ---- [queue.ops] ----
template <class _Tp, class _Container>
constexpr bool operator==(const queue<_Tp, _Container>& __x, const queue<_Tp, _Container>& y) {
  return __x.c == y.c;
}
template <class _Tp, class _Container>
constexpr bool operator!=(const queue<_Tp, _Container>& __x, const queue<_Tp, _Container>& y) {
  return __x.c != y.c;
}
template <class _Tp, class _Container>
constexpr bool operator<(const queue<_Tp, _Container>& __x, const queue<_Tp, _Container>& y) {
  return __x.c < y.c;
}
template <class _Tp, class _Container>
constexpr bool operator>(const queue<_Tp, _Container>& __x, const queue<_Tp, _Container>& y) {
  return __x.c > y.c;
}
template <class _Tp, class _Container>
constexpr bool operator<=(const queue<_Tp, _Container>& __x, const queue<_Tp, _Container>& y) {
  return __x.c <= y.c;
}
template <class _Tp, class _Container>
constexpr bool operator>=(const queue<_Tp, _Container>& __x, const queue<_Tp, _Container>& y) {
  return __x.c >= y.c;
}
template <class _Tp, three_way_comparable _Container>
constexpr compare_three_way_result_t<_Container> operator<=>(const queue<_Tp, _Container>& __x,
                                                            const queue<_Tp, _Container>& y) {
  return __x.c <=> y.c;
}

template <class _Tp, class _Container>
  requires is_swappable_v<_Container>
constexpr void swap(queue<_Tp, _Container>& __x, queue<_Tp, _Container>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [priority.queue] ----
template <class _Tp, class _Container = vector<_Tp, allocator<_Tp>>, class _Compare = less<typename _Container::value_type>>
class priority_queue {
  static_assert(is_same_v<_Tp, typename _Container::value_type>,
                "std::priority_queue: T must be Container::value_type ([container.adaptors.general])");

public:
  using value_type = typename _Container::value_type;
  using reference = typename _Container::reference;
  using const_reference = typename _Container::const_reference;
  using size_type = typename _Container::size_type;
  using container_type = _Container;
  using value_compare = _Compare;

protected:
  _Container c;
  _Compare comp;

private:
  constexpr void __heapify() { std::make_heap(c.begin(), c.end(), comp); }
  constexpr void __clear_moved_from() {
    if constexpr (requires { c.clear(); })
      c.clear();
  }

public:
  // Constrained (an extension), so that is_default_constructible reflects the members'.
  constexpr priority_queue()
    requires is_default_constructible_v<_Container> && is_default_constructible_v<_Compare>
      : priority_queue(_Compare()) {}
  constexpr explicit priority_queue(const _Compare& __x)
    requires is_default_constructible_v<_Container>
      : priority_queue(__x, _Container()) {}
  // A moved-from priority_queue is left empty when the container has clear() (an extension; the
  // draft does not require clear() of the container): its container's moved-from contents need
  // not form a heap, and top()/pop() rely on that invariant.
  constexpr priority_queue(const priority_queue&) = default;
  constexpr priority_queue(priority_queue&& __q) noexcept(is_nothrow_move_constructible_v<_Container> &&
                                                        is_nothrow_move_constructible_v<_Compare>)
      : c(static_cast<_Container&&>(__q.c)), comp(static_cast<_Compare&&>(__q.comp)) {
    __q.__clear_moved_from();
  }
  constexpr priority_queue& operator=(const priority_queue&) = default;
  constexpr priority_queue& operator=(priority_queue&& __q) noexcept(is_nothrow_move_assignable_v<_Container> &&
                                                                   is_nothrow_move_assignable_v<_Compare>) {
    if (this != __builtin_addressof(__q)) {
      c = static_cast<_Container&&>(__q.c);
      comp = static_cast<_Compare&&>(__q.comp);
      __q.__clear_moved_from();
    }
    return *this;
  }
  constexpr priority_queue(const _Compare& __x, const _Container& y) : c(y), comp(__x) { __heapify(); }
  constexpr priority_queue(const _Compare& __x, _Container&& y) : c(static_cast<_Container&&>(y)), comp(__x) { __heapify(); }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr priority_queue(_InputIterator first, _InputIterator last, const _Compare& __x = _Compare())
      : c(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last)), comp(__x) {
    __heapify();
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr priority_queue(_InputIterator first, _InputIterator last, const _Compare& __x, const _Container& y)
      : c(y), comp(__x) {
    c.insert(c.end(), static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    __heapify();
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr priority_queue(_InputIterator first, _InputIterator last, const _Compare& __x, _Container&& y)
      : c(static_cast<_Container&&>(y)), comp(__x) {
    c.insert(c.end(), static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    __heapify();
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr priority_queue(from_range_t, _Rp&& __rg, const _Compare& __x = _Compare())
      : c(__ycxx::__detail::__range_to<_Container>(static_cast<_Rp&&>(__rg))), comp(__x) {
    __heapify();
  }
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr explicit priority_queue(const _Alloc& a) : c(a), comp() {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(const _Compare& compare, const _Alloc& a) : c(a), comp(compare) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(const _Compare& compare, const _Container& __cont, const _Alloc& a)
      : c(__cont, a), comp(compare) {
    __heapify();
  }
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(const _Compare& compare, _Container&& __cont, const _Alloc& a)
      : c(static_cast<_Container&&>(__cont), a), comp(compare) {
    __heapify();
  }
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(const priority_queue& __q, const _Alloc& a) : c(__q.c, a), comp(__q.comp) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(priority_queue&& __q, const _Alloc& a)
      : c(static_cast<_Container&&>(__q.c), a), comp(static_cast<_Compare&&>(__q.comp)) {
    __q.__clear_moved_from();
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(_InputIterator first, _InputIterator last, const _Alloc& a)
      : c(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last), a), comp() {
    __heapify();
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(_InputIterator first, _InputIterator last, const _Compare& compare, const _Alloc& a)
      : c(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last), a), comp(compare) {
    __heapify();
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(_InputIterator first, _InputIterator last, const _Compare& compare, const _Container& __cont,
                           const _Alloc& a)
      : c(__cont, a), comp(compare) {
    c.insert(c.end(), static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    __heapify();
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(_InputIterator first, _InputIterator last, const _Compare& compare, _Container&& __cont,
                           const _Alloc& a)
      : c(static_cast<_Container&&>(__cont), a), comp(compare) {
    c.insert(c.end(), static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    __heapify();
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp, class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(from_range_t, _Rp&& __rg, const _Compare& compare, const _Alloc& a)
      : c(__ycxx::__detail::__range_to<_Container>(static_cast<_Rp&&>(__rg), a)), comp(compare) {
    __heapify();
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp, class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr priority_queue(from_range_t, _Rp&& __rg, const _Alloc& a)
      : c(__ycxx::__detail::__range_to<_Container>(static_cast<_Rp&&>(__rg), a)), comp() {
    __heapify();
  }

  [[nodiscard]] constexpr bool empty() const { return c.empty(); }
  constexpr size_type size() const { return c.size(); }
  constexpr const_reference top() const { return c.front(); }
  constexpr void push(const value_type& __x) {
    c.push_back(__x);
    std::push_heap(c.begin(), c.end(), comp);
  }
  constexpr void push(value_type&& __x) {
    c.push_back(static_cast<value_type&&>(__x));
    std::push_heap(c.begin(), c.end(), comp);
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void push_range(_Rp&& __rg) {
    if constexpr (requires { c.append_range(static_cast<_Rp&&>(__rg)); })
      c.append_range(static_cast<_Rp&&>(__rg));
    else
      ranges::copy(__rg, std::back_inserter(c));
    __heapify();
  }
  template <class... _Args>
  constexpr void emplace(_Args&&... __args) {
    c.emplace_back(static_cast<_Args&&>(__args)...);
    std::push_heap(c.begin(), c.end(), comp);
  }
  constexpr void pop() {
    std::pop_heap(c.begin(), c.end(), comp);
    c.pop_back();
  }
  constexpr void swap(priority_queue& __q) noexcept(is_nothrow_swappable_v<_Container> &&
                                                  is_nothrow_swappable_v<_Compare>) {
    ::__ycxx::__detail::__swap_adl::__do_swap(c, __q.c);
    ::__ycxx::__detail::__swap_adl::__do_swap(comp, __q.comp);
  }
};

template <class _Compare, class _Container>
  requires(!__ycxx::__detail::__qualifies_as_allocator<_Compare>) && (!__ycxx::__detail::__qualifies_as_allocator<_Container>)
priority_queue(_Compare, _Container) -> priority_queue<typename _Container::value_type, _Container, _Compare>;
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_value_type<_InputIterator>>,
          class _Container = vector<__ycxx::__detail::__iter_value_type<_InputIterator>,
                                   allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
           (!__ycxx::__detail::__qualifies_as_allocator<_Compare>) && (!__ycxx::__detail::__qualifies_as_allocator<_Container>)
priority_queue(_InputIterator, _InputIterator, _Compare = _Compare(), _Container = _Container())
    -> priority_queue<__ycxx::__detail::__iter_value_type<_InputIterator>, _Container, _Compare>;
template <ranges::input_range _Rp, class _Compare = less<ranges::range_value_t<_Rp>>>
  requires(!__ycxx::__detail::__qualifies_as_allocator<_Compare>)
priority_queue(from_range_t, _Rp&&, _Compare = _Compare())
    -> priority_queue<ranges::range_value_t<_Rp>,
                      vector<ranges::range_value_t<_Rp>, allocator<ranges::range_value_t<_Rp>>>, _Compare>;
template <class _Compare, class _Container, class _Allocator>
  requires(!__ycxx::__detail::__qualifies_as_allocator<_Compare>) && (!__ycxx::__detail::__qualifies_as_allocator<_Container>) &&
          uses_allocator_v<_Container, _Allocator>
priority_queue(_Compare, _Container, _Allocator) -> priority_queue<typename _Container::value_type, _Container, _Compare>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
priority_queue(_InputIterator, _InputIterator, _Allocator)
    -> priority_queue<__ycxx::__detail::__iter_value_type<_InputIterator>,
                      vector<__ycxx::__detail::__iter_value_type<_InputIterator>, _Allocator>,
                      less<__ycxx::__detail::__iter_value_type<_InputIterator>>>;
template <class _InputIterator, class _Compare, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
           (!__ycxx::__detail::__qualifies_as_allocator<_Compare>) && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
priority_queue(_InputIterator, _InputIterator, _Compare, _Allocator)
    -> priority_queue<__ycxx::__detail::__iter_value_type<_InputIterator>,
                      vector<__ycxx::__detail::__iter_value_type<_InputIterator>, _Allocator>, _Compare>;
template <class _InputIterator, class _Compare, class _Container, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
           (!__ycxx::__detail::__qualifies_as_allocator<_Compare>) && (!__ycxx::__detail::__qualifies_as_allocator<_Container>) &&
           uses_allocator_v<_Container, _Allocator>
priority_queue(_InputIterator, _InputIterator, _Compare, _Container, _Allocator)
    -> priority_queue<typename _Container::value_type, _Container, _Compare>;
template <ranges::input_range _Rp, class _Compare, class _Allocator>
  requires(!__ycxx::__detail::__qualifies_as_allocator<_Compare>) && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
priority_queue(from_range_t, _Rp&&, _Compare, _Allocator)
    -> priority_queue<ranges::range_value_t<_Rp>, vector<ranges::range_value_t<_Rp>, _Allocator>, _Compare>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
priority_queue(from_range_t, _Rp&&, _Allocator)
    -> priority_queue<ranges::range_value_t<_Rp>, vector<ranges::range_value_t<_Rp>, _Allocator>>;

template <class _Tp, class _Container, class _Compare, class _Alloc>
struct uses_allocator<priority_queue<_Tp, _Container, _Compare>, _Alloc> : uses_allocator<_Container, _Alloc>::type {};

template <class _Tp, class _Container, class _Compare>
  requires is_swappable_v<_Container> && is_swappable_v<_Compare>
constexpr void swap(priority_queue<_Tp, _Container, _Compare>& __x,
                    priority_queue<_Tp, _Container, _Compare>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

} // namespace std
