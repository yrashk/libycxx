// libycxx core: the ExecutionPolicy overloads of the specialized memory algorithms
// ([memory.syn], [specialized.algorithms]). As in algo_parallel.hpp, each runs the sequential
// algorithm on the calling thread and is noexcept: an exception leaving an element access
// function (here, an element's constructor) calls terminate ([algorithms.parallel.exceptions]).
#pragma once

#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/uninitialized.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _ExecutionPolicy, class _NoThrowForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void uninitialized_default_construct(_ExecutionPolicy&&, _NoThrowForwardIterator first,
                                     _NoThrowForwardIterator last) noexcept {
  std::uninitialized_default_construct(first, last);
}

template <class _ExecutionPolicy, class _NoThrowForwardIterator, class _Size>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_NoThrowForwardIterator uninitialized_default_construct_n(_ExecutionPolicy&&, _NoThrowForwardIterator first,
                                                         _Size n) noexcept {
  return std::uninitialized_default_construct_n(first, n);
}

template <class _ExecutionPolicy, class _NoThrowForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void uninitialized_value_construct(_ExecutionPolicy&&, _NoThrowForwardIterator first,
                                   _NoThrowForwardIterator last) noexcept {
  std::uninitialized_value_construct(first, last);
}

template <class _ExecutionPolicy, class _NoThrowForwardIterator, class _Size>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_NoThrowForwardIterator uninitialized_value_construct_n(_ExecutionPolicy&&, _NoThrowForwardIterator first,
                                                       _Size n) noexcept {
  return std::uninitialized_value_construct_n(first, n);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _NoThrowForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_NoThrowForwardIterator uninitialized_copy(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last,
                                          _NoThrowForwardIterator result) noexcept {
  return std::uninitialized_copy(first, last, result);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Size, class _NoThrowForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_NoThrowForwardIterator uninitialized_copy_n(_ExecutionPolicy&&, _ForwardIterator first, _Size n,
                                            _NoThrowForwardIterator result) noexcept {
  return std::uninitialized_copy_n(first, n, result);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _NoThrowForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_NoThrowForwardIterator uninitialized_move(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last,
                                          _NoThrowForwardIterator result) noexcept {
  return std::uninitialized_move(first, last, result);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Size, class _NoThrowForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
pair<_ForwardIterator, _NoThrowForwardIterator> uninitialized_move_n(_ExecutionPolicy&&, _ForwardIterator first, _Size n,
                                                                   _NoThrowForwardIterator result) noexcept {
  return std::uninitialized_move_n(first, n, result);
}

template <class _ExecutionPolicy, class _NoThrowForwardIterator,
          class _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void uninitialized_fill(_ExecutionPolicy&&, _NoThrowForwardIterator first, _NoThrowForwardIterator last,
                        const _Tp& __x) noexcept {
  std::uninitialized_fill(first, last, __x);
}

template <class _ExecutionPolicy, class _NoThrowForwardIterator, class _Size,
          class _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_NoThrowForwardIterator uninitialized_fill_n(_ExecutionPolicy&&, _NoThrowForwardIterator first, _Size n,
                                            const _Tp& __x) noexcept {
  return std::uninitialized_fill_n(first, n, __x);
}

template <class _ExecutionPolicy, class _NoThrowForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void destroy(_ExecutionPolicy&&, _NoThrowForwardIterator first, _NoThrowForwardIterator last) noexcept {
  std::destroy(first, last);
}

template <class _ExecutionPolicy, class _NoThrowForwardIterator, class _Size>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_NoThrowForwardIterator destroy_n(_ExecutionPolicy&&, _NoThrowForwardIterator first, _Size n) noexcept {
  return std::destroy_n(first, n);
}

}} // namespace std
