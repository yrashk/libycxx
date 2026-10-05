// libycxx core: the ExecutionPolicy overloads of the specialized memory algorithms
// ([memory.syn], [specialized.algorithms]). As in algo_parallel.hpp, each runs the sequential
// algorithm on the calling thread and is noexcept: an exception leaving an element access
// function (here, an element's constructor) calls terminate ([algorithms.parallel.exceptions]).
#pragma once

#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/uninitialized.hpp>

namespace [[gnu::visibility("hidden")]] std {

template <class ExecutionPolicy, class NoThrowForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void uninitialized_default_construct(ExecutionPolicy&&, NoThrowForwardIterator first,
                                     NoThrowForwardIterator last) noexcept {
  std::uninitialized_default_construct(first, last);
}

template <class ExecutionPolicy, class NoThrowForwardIterator, class Size>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
NoThrowForwardIterator uninitialized_default_construct_n(ExecutionPolicy&&, NoThrowForwardIterator first,
                                                         Size n) noexcept {
  return std::uninitialized_default_construct_n(first, n);
}

template <class ExecutionPolicy, class NoThrowForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void uninitialized_value_construct(ExecutionPolicy&&, NoThrowForwardIterator first,
                                   NoThrowForwardIterator last) noexcept {
  std::uninitialized_value_construct(first, last);
}

template <class ExecutionPolicy, class NoThrowForwardIterator, class Size>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
NoThrowForwardIterator uninitialized_value_construct_n(ExecutionPolicy&&, NoThrowForwardIterator first,
                                                       Size n) noexcept {
  return std::uninitialized_value_construct_n(first, n);
}

template <class ExecutionPolicy, class ForwardIterator, class NoThrowForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
NoThrowForwardIterator uninitialized_copy(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last,
                                          NoThrowForwardIterator result) noexcept {
  return std::uninitialized_copy(first, last, result);
}

template <class ExecutionPolicy, class ForwardIterator, class Size, class NoThrowForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
NoThrowForwardIterator uninitialized_copy_n(ExecutionPolicy&&, ForwardIterator first, Size n,
                                            NoThrowForwardIterator result) noexcept {
  return std::uninitialized_copy_n(first, n, result);
}

template <class ExecutionPolicy, class ForwardIterator, class NoThrowForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
NoThrowForwardIterator uninitialized_move(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last,
                                          NoThrowForwardIterator result) noexcept {
  return std::uninitialized_move(first, last, result);
}

template <class ExecutionPolicy, class ForwardIterator, class Size, class NoThrowForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
pair<ForwardIterator, NoThrowForwardIterator> uninitialized_move_n(ExecutionPolicy&&, ForwardIterator first, Size n,
                                                                   NoThrowForwardIterator result) noexcept {
  return std::uninitialized_move_n(first, n, result);
}

template <class ExecutionPolicy, class NoThrowForwardIterator,
          class T = typename iterator_traits<NoThrowForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void uninitialized_fill(ExecutionPolicy&&, NoThrowForwardIterator first, NoThrowForwardIterator last,
                        const T& x) noexcept {
  std::uninitialized_fill(first, last, x);
}

template <class ExecutionPolicy, class NoThrowForwardIterator, class Size,
          class T = typename iterator_traits<NoThrowForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
NoThrowForwardIterator uninitialized_fill_n(ExecutionPolicy&&, NoThrowForwardIterator first, Size n,
                                            const T& x) noexcept {
  return std::uninitialized_fill_n(first, n, x);
}

template <class ExecutionPolicy, class NoThrowForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void destroy(ExecutionPolicy&&, NoThrowForwardIterator first, NoThrowForwardIterator last) noexcept {
  std::destroy(first, last);
}

template <class ExecutionPolicy, class NoThrowForwardIterator, class Size>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
NoThrowForwardIterator destroy_n(ExecutionPolicy&&, NoThrowForwardIterator first, Size n) noexcept {
  return std::destroy_n(first, n);
}

} // namespace std
