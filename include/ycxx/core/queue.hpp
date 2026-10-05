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

namespace [[gnu::visibility("hidden")]] std {

template <class T, class Allocator>
class vector;

// ---- [queue] ----
template <class T, class Container = deque<T>>
class queue {
  static_assert(is_same_v<T, typename Container::value_type>,
                "std::queue: T must be Container::value_type ([container.adaptors.general])");

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
  constexpr queue()
    requires is_default_constructible_v<Container>
      : queue(Container()) {}
  constexpr explicit queue(const Container& cont) : c(cont) {}
  constexpr explicit queue(Container&& cont) : c(static_cast<Container&&>(cont)) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr queue(InputIterator first, InputIterator last)
      : c(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last)) {}
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr queue(from_range_t, R&& rg) : c(ycxx::detail::range_to<Container>(static_cast<R&&>(rg))) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr explicit queue(const Alloc& a) : c(a) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr queue(const Container& cont, const Alloc& a) : c(cont, a) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr queue(Container&& cont, const Alloc& a) : c(static_cast<Container&&>(cont), a) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr queue(const queue& q, const Alloc& a) : c(q.c, a) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr queue(queue&& q, const Alloc& a) : c(static_cast<Container&&>(q.c), a) {}
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && uses_allocator_v<Container, Alloc>
  constexpr queue(InputIterator first, InputIterator last, const Alloc& alloc)
      : c(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last), alloc) {}
  template <ycxx::detail::container_compatible_range<T> R, class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr queue(from_range_t, R&& rg, const Alloc& a)
      : c(ycxx::detail::range_to<Container>(static_cast<R&&>(rg), a)) {}

  [[nodiscard]] constexpr bool empty() const { return c.empty(); }
  constexpr size_type size() const { return c.size(); }
  constexpr reference front() { return c.front(); }
  constexpr const_reference front() const { return c.front(); }
  constexpr reference back() { return c.back(); }
  constexpr const_reference back() const { return c.back(); }
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
  constexpr void pop() { c.pop_front(); }
  constexpr void swap(queue& q) noexcept(is_nothrow_swappable_v<Container>) {
    ::ycxx::detail::swap_adl::do_swap(c, q.c);
  }

  template <class T1, class C1>
  friend constexpr bool operator==(const queue<T1, C1>&, const queue<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator!=(const queue<T1, C1>&, const queue<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator<(const queue<T1, C1>&, const queue<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator>(const queue<T1, C1>&, const queue<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator<=(const queue<T1, C1>&, const queue<T1, C1>&);
  template <class T1, class C1>
  friend constexpr bool operator>=(const queue<T1, C1>&, const queue<T1, C1>&);
  template <class T1, three_way_comparable C1>
  friend constexpr compare_three_way_result_t<C1> operator<=>(const queue<T1, C1>&, const queue<T1, C1>&);
};

template <class Container>
  requires(!ycxx::detail::qualifies_as_allocator<Container>)
queue(Container) -> queue<typename Container::value_type, Container>;
template <class InputIterator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
queue(InputIterator, InputIterator) -> queue<ycxx::detail::iter_value_type<InputIterator>>;
template <ranges::input_range R>
queue(from_range_t, R&&) -> queue<ranges::range_value_t<R>>;
template <class Container, class Allocator>
  requires(!ycxx::detail::qualifies_as_allocator<Container>) && uses_allocator_v<Container, Allocator>
queue(Container, Allocator) -> queue<typename Container::value_type, Container>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
queue(InputIterator, InputIterator, Allocator)
    -> queue<ycxx::detail::iter_value_type<InputIterator>, deque<ycxx::detail::iter_value_type<InputIterator>, Allocator>>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
queue(from_range_t, R&&, Allocator) -> queue<ranges::range_value_t<R>, deque<ranges::range_value_t<R>, Allocator>>;

template <class T, class Container, class Alloc>
struct uses_allocator<queue<T, Container>, Alloc> : uses_allocator<Container, Alloc>::type {};

// ---- [queue.ops] ----
template <class T, class Container>
constexpr bool operator==(const queue<T, Container>& x, const queue<T, Container>& y) {
  return x.c == y.c;
}
template <class T, class Container>
constexpr bool operator!=(const queue<T, Container>& x, const queue<T, Container>& y) {
  return x.c != y.c;
}
template <class T, class Container>
constexpr bool operator<(const queue<T, Container>& x, const queue<T, Container>& y) {
  return x.c < y.c;
}
template <class T, class Container>
constexpr bool operator>(const queue<T, Container>& x, const queue<T, Container>& y) {
  return x.c > y.c;
}
template <class T, class Container>
constexpr bool operator<=(const queue<T, Container>& x, const queue<T, Container>& y) {
  return x.c <= y.c;
}
template <class T, class Container>
constexpr bool operator>=(const queue<T, Container>& x, const queue<T, Container>& y) {
  return x.c >= y.c;
}
template <class T, three_way_comparable Container>
constexpr compare_three_way_result_t<Container> operator<=>(const queue<T, Container>& x,
                                                            const queue<T, Container>& y) {
  return x.c <=> y.c;
}

template <class T, class Container>
  requires is_swappable_v<Container>
constexpr void swap(queue<T, Container>& x, queue<T, Container>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [priority.queue] ----
template <class T, class Container = vector<T, allocator<T>>, class Compare = less<typename Container::value_type>>
class priority_queue {
  static_assert(is_same_v<T, typename Container::value_type>,
                "std::priority_queue: T must be Container::value_type ([container.adaptors.general])");

public:
  using value_type = typename Container::value_type;
  using reference = typename Container::reference;
  using const_reference = typename Container::const_reference;
  using size_type = typename Container::size_type;
  using container_type = Container;
  using value_compare = Compare;

protected:
  Container c;
  Compare comp;

private:
  constexpr void heapify() { std::make_heap(c.begin(), c.end(), comp); }
  constexpr void clear_moved_from() {
    if constexpr (requires { c.clear(); })
      c.clear();
  }

public:
  // Constrained (an extension), so that is_default_constructible reflects the members'.
  constexpr priority_queue()
    requires is_default_constructible_v<Container> && is_default_constructible_v<Compare>
      : priority_queue(Compare()) {}
  constexpr explicit priority_queue(const Compare& x)
    requires is_default_constructible_v<Container>
      : priority_queue(x, Container()) {}
  // A moved-from priority_queue is left empty when the container has clear() (an extension; the
  // draft does not require clear() of the container): its container's moved-from contents need
  // not form a heap, and top()/pop() rely on that invariant.
  constexpr priority_queue(const priority_queue&) = default;
  constexpr priority_queue(priority_queue&& q) noexcept(is_nothrow_move_constructible_v<Container> &&
                                                        is_nothrow_move_constructible_v<Compare>)
      : c(static_cast<Container&&>(q.c)), comp(static_cast<Compare&&>(q.comp)) {
    q.clear_moved_from();
  }
  constexpr priority_queue& operator=(const priority_queue&) = default;
  constexpr priority_queue& operator=(priority_queue&& q) noexcept(is_nothrow_move_assignable_v<Container> &&
                                                                   is_nothrow_move_assignable_v<Compare>) {
    if (this != __builtin_addressof(q)) {
      c = static_cast<Container&&>(q.c);
      comp = static_cast<Compare&&>(q.comp);
      q.clear_moved_from();
    }
    return *this;
  }
  constexpr priority_queue(const Compare& x, const Container& y) : c(y), comp(x) { heapify(); }
  constexpr priority_queue(const Compare& x, Container&& y) : c(static_cast<Container&&>(y)), comp(x) { heapify(); }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr priority_queue(InputIterator first, InputIterator last, const Compare& x = Compare())
      : c(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last)), comp(x) {
    heapify();
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr priority_queue(InputIterator first, InputIterator last, const Compare& x, const Container& y)
      : c(y), comp(x) {
    c.insert(c.end(), static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    heapify();
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr priority_queue(InputIterator first, InputIterator last, const Compare& x, Container&& y)
      : c(static_cast<Container&&>(y)), comp(x) {
    c.insert(c.end(), static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    heapify();
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr priority_queue(from_range_t, R&& rg, const Compare& x = Compare())
      : c(ycxx::detail::range_to<Container>(static_cast<R&&>(rg))), comp(x) {
    heapify();
  }
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr explicit priority_queue(const Alloc& a) : c(a), comp() {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr priority_queue(const Compare& compare, const Alloc& a) : c(a), comp(compare) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr priority_queue(const Compare& compare, const Container& cont, const Alloc& a)
      : c(cont, a), comp(compare) {
    heapify();
  }
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr priority_queue(const Compare& compare, Container&& cont, const Alloc& a)
      : c(static_cast<Container&&>(cont), a), comp(compare) {
    heapify();
  }
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr priority_queue(const priority_queue& q, const Alloc& a) : c(q.c, a), comp(q.comp) {}
  template <class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr priority_queue(priority_queue&& q, const Alloc& a)
      : c(static_cast<Container&&>(q.c), a), comp(static_cast<Compare&&>(q.comp)) {
    q.clear_moved_from();
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && uses_allocator_v<Container, Alloc>
  constexpr priority_queue(InputIterator first, InputIterator last, const Alloc& a)
      : c(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last), a), comp() {
    heapify();
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && uses_allocator_v<Container, Alloc>
  constexpr priority_queue(InputIterator first, InputIterator last, const Compare& compare, const Alloc& a)
      : c(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last), a), comp(compare) {
    heapify();
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && uses_allocator_v<Container, Alloc>
  constexpr priority_queue(InputIterator first, InputIterator last, const Compare& compare, const Container& cont,
                           const Alloc& a)
      : c(cont, a), comp(compare) {
    c.insert(c.end(), static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    heapify();
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && uses_allocator_v<Container, Alloc>
  constexpr priority_queue(InputIterator first, InputIterator last, const Compare& compare, Container&& cont,
                           const Alloc& a)
      : c(static_cast<Container&&>(cont), a), comp(compare) {
    c.insert(c.end(), static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    heapify();
  }
  template <ycxx::detail::container_compatible_range<T> R, class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr priority_queue(from_range_t, R&& rg, const Compare& compare, const Alloc& a)
      : c(ycxx::detail::range_to<Container>(static_cast<R&&>(rg), a)), comp(compare) {
    heapify();
  }
  template <ycxx::detail::container_compatible_range<T> R, class Alloc>
    requires uses_allocator_v<Container, Alloc>
  constexpr priority_queue(from_range_t, R&& rg, const Alloc& a)
      : c(ycxx::detail::range_to<Container>(static_cast<R&&>(rg), a)), comp() {
    heapify();
  }

  [[nodiscard]] constexpr bool empty() const { return c.empty(); }
  constexpr size_type size() const { return c.size(); }
  constexpr const_reference top() const { return c.front(); }
  constexpr void push(const value_type& x) {
    c.push_back(x);
    std::push_heap(c.begin(), c.end(), comp);
  }
  constexpr void push(value_type&& x) {
    c.push_back(static_cast<value_type&&>(x));
    std::push_heap(c.begin(), c.end(), comp);
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void push_range(R&& rg) {
    if constexpr (requires { c.append_range(static_cast<R&&>(rg)); })
      c.append_range(static_cast<R&&>(rg));
    else
      ranges::copy(rg, std::back_inserter(c));
    heapify();
  }
  template <class... Args>
  constexpr void emplace(Args&&... args) {
    c.emplace_back(static_cast<Args&&>(args)...);
    std::push_heap(c.begin(), c.end(), comp);
  }
  constexpr void pop() {
    std::pop_heap(c.begin(), c.end(), comp);
    c.pop_back();
  }
  constexpr void swap(priority_queue& q) noexcept(is_nothrow_swappable_v<Container> &&
                                                  is_nothrow_swappable_v<Compare>) {
    ::ycxx::detail::swap_adl::do_swap(c, q.c);
    ::ycxx::detail::swap_adl::do_swap(comp, q.comp);
  }
};

template <class Compare, class Container>
  requires(!ycxx::detail::qualifies_as_allocator<Compare>) && (!ycxx::detail::qualifies_as_allocator<Container>)
priority_queue(Compare, Container) -> priority_queue<typename Container::value_type, Container, Compare>;
template <class InputIterator, class Compare = less<ycxx::detail::iter_value_type<InputIterator>>,
          class Container = vector<ycxx::detail::iter_value_type<InputIterator>,
                                   allocator<ycxx::detail::iter_value_type<InputIterator>>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
           (!ycxx::detail::qualifies_as_allocator<Compare>) && (!ycxx::detail::qualifies_as_allocator<Container>)
priority_queue(InputIterator, InputIterator, Compare = Compare(), Container = Container())
    -> priority_queue<ycxx::detail::iter_value_type<InputIterator>, Container, Compare>;
template <ranges::input_range R, class Compare = less<ranges::range_value_t<R>>>
  requires(!ycxx::detail::qualifies_as_allocator<Compare>)
priority_queue(from_range_t, R&&, Compare = Compare())
    -> priority_queue<ranges::range_value_t<R>,
                      vector<ranges::range_value_t<R>, allocator<ranges::range_value_t<R>>>, Compare>;
template <class Compare, class Container, class Allocator>
  requires(!ycxx::detail::qualifies_as_allocator<Compare>) && (!ycxx::detail::qualifies_as_allocator<Container>) &&
          uses_allocator_v<Container, Allocator>
priority_queue(Compare, Container, Allocator) -> priority_queue<typename Container::value_type, Container, Compare>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
priority_queue(InputIterator, InputIterator, Allocator)
    -> priority_queue<ycxx::detail::iter_value_type<InputIterator>,
                      vector<ycxx::detail::iter_value_type<InputIterator>, Allocator>,
                      less<ycxx::detail::iter_value_type<InputIterator>>>;
template <class InputIterator, class Compare, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
           (!ycxx::detail::qualifies_as_allocator<Compare>) && ycxx::detail::qualifies_as_allocator<Allocator>
priority_queue(InputIterator, InputIterator, Compare, Allocator)
    -> priority_queue<ycxx::detail::iter_value_type<InputIterator>,
                      vector<ycxx::detail::iter_value_type<InputIterator>, Allocator>, Compare>;
template <class InputIterator, class Compare, class Container, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
           (!ycxx::detail::qualifies_as_allocator<Compare>) && (!ycxx::detail::qualifies_as_allocator<Container>) &&
           uses_allocator_v<Container, Allocator>
priority_queue(InputIterator, InputIterator, Compare, Container, Allocator)
    -> priority_queue<typename Container::value_type, Container, Compare>;
template <ranges::input_range R, class Compare, class Allocator>
  requires(!ycxx::detail::qualifies_as_allocator<Compare>) && ycxx::detail::qualifies_as_allocator<Allocator>
priority_queue(from_range_t, R&&, Compare, Allocator)
    -> priority_queue<ranges::range_value_t<R>, vector<ranges::range_value_t<R>, Allocator>, Compare>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
priority_queue(from_range_t, R&&, Allocator)
    -> priority_queue<ranges::range_value_t<R>, vector<ranges::range_value_t<R>, Allocator>>;

template <class T, class Container, class Compare, class Alloc>
struct uses_allocator<priority_queue<T, Container, Compare>, Alloc> : uses_allocator<Container, Alloc>::type {};

template <class T, class Container, class Compare>
  requires is_swappable_v<Container> && is_swappable_v<Compare>
constexpr void swap(priority_queue<T, Container, Compare>& x,
                    priority_queue<T, Container, Compare>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

} // namespace std
