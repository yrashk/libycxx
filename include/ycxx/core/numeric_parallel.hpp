// libycxx core: the ExecutionPolicy overloads of the <numeric> algorithms
// ([algorithms.parallel.overloads]), run sequentially; see algo_parallel.hpp.
#pragma once

#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/numeric.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
typename iterator_traits<_ForwardIterator>::value_type reduce(_ExecutionPolicy&&, _ForwardIterator first,
                                                             _ForwardIterator last) noexcept {
  return std::reduce(first, last);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Tp reduce(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Tp init) noexcept {
  return std::reduce(first, last, std::move(init));
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp, class _BinaryOperation>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Tp reduce(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Tp init, _BinaryOperation __binary_op) noexcept {
  return std::reduce(first, last, std::move(init), __binary_op);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Tp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Tp transform_reduce(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                   _Tp init) noexcept {
  return std::transform_reduce(__first1, __last1, __first2, std::move(init));
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Tp, class _BinaryOperation1,
          class _BinaryOperation2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Tp transform_reduce(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2, _Tp init,
                   _BinaryOperation1 __binary_op1, _BinaryOperation2 __binary_op2) noexcept {
  return std::transform_reduce(__first1, __last1, __first2, std::move(init), __binary_op1, __binary_op2);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp, class _BinaryOperation, class _UnaryOperation>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Tp transform_reduce(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Tp init, _BinaryOperation __binary_op,
                   _UnaryOperation __unary_op) noexcept {
  return std::transform_reduce(first, last, std::move(init), __binary_op, __unary_op);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Tp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 exclusive_scan(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                _ForwardIterator2 result, _Tp init) noexcept {
  return std::exclusive_scan(first, last, result, std::move(init));
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Tp, class _BinaryOperation>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 exclusive_scan(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                _ForwardIterator2 result, _Tp init, _BinaryOperation __binary_op) noexcept {
  return std::exclusive_scan(first, last, result, std::move(init), __binary_op);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 inclusive_scan(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                _ForwardIterator2 result) noexcept {
  return std::inclusive_scan(first, last, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryOperation>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 inclusive_scan(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                _ForwardIterator2 result, _BinaryOperation __binary_op) noexcept {
  return std::inclusive_scan(first, last, result, __binary_op);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryOperation, class _Tp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 inclusive_scan(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                _ForwardIterator2 result, _BinaryOperation __binary_op, _Tp init) noexcept {
  return std::inclusive_scan(first, last, result, __binary_op, std::move(init));
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Tp, class _BinaryOperation,
          class _UnaryOperation>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 transform_exclusive_scan(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                          _ForwardIterator2 result, _Tp init, _BinaryOperation __binary_op,
                                          _UnaryOperation __unary_op) noexcept {
  return std::transform_exclusive_scan(first, last, result, std::move(init), __binary_op, __unary_op);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryOperation,
          class _UnaryOperation>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 transform_inclusive_scan(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                          _ForwardIterator2 result, _BinaryOperation __binary_op,
                                          _UnaryOperation __unary_op) noexcept {
  return std::transform_inclusive_scan(first, last, result, __binary_op, __unary_op);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryOperation,
          class _UnaryOperation, class _Tp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 transform_inclusive_scan(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                          _ForwardIterator2 result, _BinaryOperation __binary_op, _UnaryOperation __unary_op,
                                          _Tp init) noexcept {
  return std::transform_inclusive_scan(first, last, result, __binary_op, __unary_op, std::move(init));
}

// [adjacent.difference]/5: unlike the overloads without a policy (an accumulator and a copy of
// each element, binary_op(val, std::move(acc))), these pass the input elements themselves and
// make no T.
template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryOperation>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 adjacent_difference(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                     _ForwardIterator2 result, _BinaryOperation __binary_op) noexcept {
  if (first == last)
    return result;
  *result = *first;
  ++result;
  for (_ForwardIterator1 prev = first; ++first != last; prev = first, (void)++result)
    *result = __binary_op(*first, *prev);
  return result;
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 adjacent_difference(_ExecutionPolicy&& __exec, _ForwardIterator1 first, _ForwardIterator1 last,
                                     _ForwardIterator2 result) noexcept {
  return std::adjacent_difference(static_cast<_ExecutionPolicy&&>(__exec), first, last, result, minus<>());
}

} // namespace std
