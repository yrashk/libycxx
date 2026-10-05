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

namespace std {

template <class T, class Container = deque<T>>
class stack {
  static_assert(is_same_v<T, typename Container::value_type>,
                "std::stack: T must be Container::value_type ([container.adaptors.general])");

public:
  using value_type = typename Container::value_type;
  using reference = typename Container::reference;
  using const_reference = typename Container::const_reference;
  using size_type = typename Container::size_type;
  using container_type = Container;

protected:
  Container c;

public:
  // Constrained (an extension), so that is_default_constructible reflects Container's.
  constexpr stack()
    requires is_default_constructible_v<Container>
      : stack(Container()) {}
  constexpr explicit stack(const Container& cont) : c(cont) {}
  constexpr explicit stack(Container&& cont) : c(static_cast<Container&&>(cont)) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr stack(InputIterator first, InputIterator last)
      : c(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last)) {}
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr stack(from_range_t, R&& rg) : c(ycxx::detail::range_to<Container>(static_cast<R&&>(rg))) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr explicit stack(const Alloc& a) : c(a) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr stack(const Container& cont, const Alloc& a) : c(cont, a) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr stack(Container&& cont, const Alloc& a) : c(static_cast<Container&&>(cont), a) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr stack(const stack& s, const Alloc& a) : c(s.c, a) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr stack(stack&& s, const Alloc& a) : c(static_cast<Container&&>(s.c), a) {}
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && uses_allocator_v<Container, Alloc>
  constexpr stack(InputIterator first, InputIterator last, const Alloc& alloc)
      : c(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last), alloc) {}
  template <ycxx::detail::container_compatible_range<T> R, class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr stack(from_range_t, R&& rg, const Alloc& a)
      : c(ycxx::detail::range_to<Container>(static_cast<R&&>(rg), a)) {}

  [[nodiscard]] constexpr bool empty() const { return c.empty(); }
  constexpr size_type size() const { return c.size(); }
  constexpr reference top() { return c.back(); }
  constexpr const_reference top() const { return c.back(); }
  constexpr void push(const value_type& x) { c.push_back(x); }
  constexpr void push(value_type&& x) { c.push_back(static_cast<value_type&&>(x)); }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void push_range(R&& rg) {
    if constexpr (requires { c.append_range(static_cast<R&&>(rg)); })
      c.append_range(static_cast<R&&>(rg));
    else
      ranges::copy(rg, std::back_inserter(c));
  }
  template <class... Args>
  constexpr decltype(auto) emplace(Args&&... args) {
    return c.emplace_back(static_cast<Args&&>(args)...);
  }
  constexpr void pop() { c.pop_back(); }
  constexpr void swap(stack& s) noexcept(is_nothrow_swappable_v<Container>) {
    ::ycxx::detail::swap_adl::do_swap(c, s.c);
  }

  template <class T1, class C1>
  friend constexpr bool operator==(const stack<T1, C1>&, const stack<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator!=(const stack<T1, C1>&, const stack<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator<(const stack<T1, C1>&, const stack<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator>(const stack<T1, C1>&, const stack<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator<=(const stack<T1, C1>&, const stack<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator>=(const stack<T1, C1>&, const stack<T1, C1>&);
  template <class T1, three_way_comparable C1>
  friend constexpr compare_three_way_result_t<C1> operator<=>(const stack<T1, C1>&, const stack<T1, C1>&);
};

template <class Container>
  requires(!ycxx::detail::qualifies_as_allocator<Container>)
stack(Container) -> stack<typename Container::value_type, Container>;
template <class InputIterator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
stack(InputIterator, InputIterator) -> stack<ycxx::detail::iter_value_type<InputIterator>>;
template <ranges::input_range R>
stack(from_range_t, R&&) -> stack<ranges::range_value_t<R>>;
template <class Container, class Allocator>
  requires(!ycxx::detail::qualifies_as_allocator<Container>) && uses_allocator_v<Container, Allocator>
stack(Container, Allocator) -> stack<typename Container::value_type, Container>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
stack(InputIterator, InputIterator, Allocator)
    -> stack<ycxx::detail::iter_value_type<InputIterator>, deque<ycxx::detail::iter_value_type<InputIterator>, Allocator>>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
stack(from_range_t, R&&, Allocator) -> stack<ranges::range_value_t<R>, deque<ranges::range_value_t<R>, Allocator>>;

template <class T, class Container, class Alloc>
struct uses_allocator<stack<T, Container>, Alloc> : uses_allocator<Container, Alloc>::type {};

// ---- [stack.ops] ----
template <class T, class Container>
constexpr bool operator==(const stack<T, Container>& x, const stack<T, Container>& y) {
  return x.c == y.c;
}
template <class T, class Container>
constexpr bool operator!=(const stack<T, Container>& x, const stack<T, Container>& y) {
  return x.c != y.c;
}
template <class T, class Container>
constexpr bool operator<(const stack<T, Container>& x, const stack<T, Container>& y) {
  return x.c < y.c;
}
template <class T, class Container>
constexpr bool operator>(const stack<T, Container>& x, const stack<T, Container>& y) {
  return x.c > y.c;
}
template <class T, class Container>
constexpr bool operator<=(const stack<T, Container>& x, const stack<T, Container>& y) {
  return x.c <= y.c;
}
template <class T, class Container>
constexpr bool operator>=(const stack<T, Container>& x, const stack<T, Container>& y) {
  return x.c >= y.c;
}
template <class T, three_way_comparable Container>
constexpr compare_three_way_result_t<Container> operator<=>(const stack<T, Container>& x,
                                                            const stack<T, Container>& y) {
  return x.c <=> y.c;
}

template <class T, class Container>
  requires is_swappable_v<Container>
constexpr void swap(stack<T, Container>& x, stack<T, Container>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

} // namespace std
