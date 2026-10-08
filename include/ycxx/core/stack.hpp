// libycxx core: the container adaptor stack ([stack]). The formatter specialization
// ([container.adaptors.format]) is in ycxx/core/format_adaptors.hpp, which <stack> includes.
#pragma once

#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/deque.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp, class _Container = deque<_Tp>>
class stack {
  static_assert(is_same_v<_Tp, typename _Container::value_type>,
                "std::stack: T must be Container::value_type ([container.adaptors.general])");

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
  constexpr stack()
    requires is_default_constructible_v<_Container>
      : stack(_Container()) {}
  constexpr explicit stack(const _Container& __cont) : c(__cont) {}
  constexpr explicit stack(_Container&& __cont) : c(static_cast<_Container&&>(__cont)) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr stack(_InputIterator first, _InputIterator last)
      : c(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last)) {}
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr stack(from_range_t, _Rp&& __rg) : c(__ycxx::__detail::__range_to<_Container>(static_cast<_Rp&&>(__rg))) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr explicit stack(const _Alloc& a) : c(a) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr stack(const _Container& __cont, const _Alloc& a) : c(__cont, a) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr stack(_Container&& __cont, const _Alloc& a) : c(static_cast<_Container&&>(__cont), a) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr stack(const stack& s, const _Alloc& a) : c(s.c, a) {}
  template <class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr stack(stack&& s, const _Alloc& a) : c(static_cast<_Container&&>(s.c), a) {}
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && uses_allocator_v<_Container, _Alloc>
  constexpr stack(_InputIterator first, _InputIterator last, const _Alloc& __alloc)
      : c(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last), __alloc) {}
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp, class _Alloc>
    requires uses_allocator_v<_Container, _Alloc>
  constexpr stack(from_range_t, _Rp&& __rg, const _Alloc& a)
      : c(__ycxx::__detail::__range_to<_Container>(static_cast<_Rp&&>(__rg), a)) {}

  [[nodiscard]] constexpr bool empty() const { return c.empty(); }
  constexpr size_type size() const { return c.size(); }
  constexpr reference top() { return c.back(); }
  constexpr const_reference top() const { return c.back(); }
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
  constexpr void pop() { c.pop_back(); }
  constexpr void swap(stack& s) noexcept(is_nothrow_swappable_v<_Container>) {
    ::__ycxx::__detail::__swap_adl::__do_swap(c, s.c);
  }

  template <class _T1, class _C1>
  friend constexpr bool operator==(const stack<_T1, _C1>&, const stack<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator!=(const stack<_T1, _C1>&, const stack<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator<(const stack<_T1, _C1>&, const stack<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator>(const stack<_T1, _C1>&, const stack<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator<=(const stack<_T1, _C1>&, const stack<_T1, _C1>&);
  template <class _T1, class _C1>
  friend constexpr bool operator>=(const stack<_T1, _C1>&, const stack<_T1, _C1>&);
  template <class _T1, three_way_comparable _C1>
  friend constexpr compare_three_way_result_t<_C1> operator<=>(const stack<_T1, _C1>&, const stack<_T1, _C1>&);
};

template <class _Container>
  requires(!__ycxx::__detail::__qualifies_as_allocator<_Container>)
stack(_Container) -> stack<typename _Container::value_type, _Container>;
template <class _InputIterator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
stack(_InputIterator, _InputIterator) -> stack<__ycxx::__detail::__iter_value_type<_InputIterator>>;
template <ranges::input_range _Rp>
stack(from_range_t, _Rp&&) -> stack<ranges::range_value_t<_Rp>>;
template <class _Container, class _Allocator>
  requires(!__ycxx::__detail::__qualifies_as_allocator<_Container>) && uses_allocator_v<_Container, _Allocator>
stack(_Container, _Allocator) -> stack<typename _Container::value_type, _Container>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
stack(_InputIterator, _InputIterator, _Allocator)
    -> stack<__ycxx::__detail::__iter_value_type<_InputIterator>, deque<__ycxx::__detail::__iter_value_type<_InputIterator>, _Allocator>>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
stack(from_range_t, _Rp&&, _Allocator) -> stack<ranges::range_value_t<_Rp>, deque<ranges::range_value_t<_Rp>, _Allocator>>;

template <class _Tp, class _Container, class _Alloc>
struct uses_allocator<stack<_Tp, _Container>, _Alloc> : uses_allocator<_Container, _Alloc>::type {};

// ---- [stack.ops] ----
template <class _Tp, class _Container>
constexpr bool operator==(const stack<_Tp, _Container>& __x, const stack<_Tp, _Container>& y) {
  return __x.c == y.c;
}
template <class _Tp, class _Container>
constexpr bool operator!=(const stack<_Tp, _Container>& __x, const stack<_Tp, _Container>& y) {
  return __x.c != y.c;
}
template <class _Tp, class _Container>
constexpr bool operator<(const stack<_Tp, _Container>& __x, const stack<_Tp, _Container>& y) {
  return __x.c < y.c;
}
template <class _Tp, class _Container>
constexpr bool operator>(const stack<_Tp, _Container>& __x, const stack<_Tp, _Container>& y) {
  return __x.c > y.c;
}
template <class _Tp, class _Container>
constexpr bool operator<=(const stack<_Tp, _Container>& __x, const stack<_Tp, _Container>& y) {
  return __x.c <= y.c;
}
template <class _Tp, class _Container>
constexpr bool operator>=(const stack<_Tp, _Container>& __x, const stack<_Tp, _Container>& y) {
  return __x.c >= y.c;
}
template <class _Tp, three_way_comparable _Container>
constexpr compare_three_way_result_t<_Container> operator<=>(const stack<_Tp, _Container>& __x,
                                                            const stack<_Tp, _Container>& y) {
  return __x.c <=> y.c;
}

template <class _Tp, class _Container>
  requires is_swappable_v<_Container>
constexpr void swap(stack<_Tp, _Container>& __x, stack<_Tp, _Container>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

}} // namespace std
