// libycxx core: the ExecutionPolicy overloads of the <numeric> algorithms
// ([algorithms.parallel.overloads]), run sequentially; see algo_parallel.hpp.
#pragma once

#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/numeric.hpp>

namespace std {

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
typename iterator_traits<ForwardIterator>::value_type reduce(ExecutionPolicy&&, ForwardIterator first,
                                                             ForwardIterator last) noexcept {
  return std::reduce(first, last);
}

template <class ExecutionPolicy, class ForwardIterator, class T>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
T reduce(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, T init) noexcept {
  return std::reduce(first, last, std::move(init));
}

template <class ExecutionPolicy, class ForwardIterator, class T, class BinaryOperation>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
T reduce(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, T init, BinaryOperation binary_op) noexcept {
  return std::reduce(first, last, std::move(init), binary_op);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class T>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
T transform_reduce(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                   T init) noexcept {
  return std::transform_reduce(first1, last1, first2, std::move(init));
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class T, class BinaryOperation1,
          class BinaryOperation2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
T transform_reduce(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, T init,
                   BinaryOperation1 binary_op1, BinaryOperation2 binary_op2) noexcept {
  return std::transform_reduce(first1, last1, first2, std::move(init), binary_op1, binary_op2);
}

template <class ExecutionPolicy, class ForwardIterator, class T, class BinaryOperation, class UnaryOperation>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
T transform_reduce(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, T init, BinaryOperation binary_op,
                   UnaryOperation unary_op) noexcept {
  return std::transform_reduce(first, last, std::move(init), binary_op, unary_op);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class T>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 exclusive_scan(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                ForwardIterator2 result, T init) noexcept {
  return std::exclusive_scan(first, last, result, std::move(init));
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class T, class BinaryOperation>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 exclusive_scan(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                ForwardIterator2 result, T init, BinaryOperation binary_op) noexcept {
  return std::exclusive_scan(first, last, result, std::move(init), binary_op);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 inclusive_scan(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                ForwardIterator2 result) noexcept {
  return std::inclusive_scan(first, last, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryOperation>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 inclusive_scan(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                ForwardIterator2 result, BinaryOperation binary_op) noexcept {
  return std::inclusive_scan(first, last, result, binary_op);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryOperation, class T>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 inclusive_scan(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                ForwardIterator2 result, BinaryOperation binary_op, T init) noexcept {
  return std::inclusive_scan(first, last, result, binary_op, std::move(init));
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class T, class BinaryOperation,
          class UnaryOperation>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 transform_exclusive_scan(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                          ForwardIterator2 result, T init, BinaryOperation binary_op,
                                          UnaryOperation unary_op) noexcept {
  return std::transform_exclusive_scan(first, last, result, std::move(init), binary_op, unary_op);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryOperation,
          class UnaryOperation>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 transform_inclusive_scan(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                          ForwardIterator2 result, BinaryOperation binary_op,
                                          UnaryOperation unary_op) noexcept {
  return std::transform_inclusive_scan(first, last, result, binary_op, unary_op);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryOperation,
          class UnaryOperation, class T>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 transform_inclusive_scan(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                          ForwardIterator2 result, BinaryOperation binary_op, UnaryOperation unary_op,
                                          T init) noexcept {
  return std::transform_inclusive_scan(first, last, result, binary_op, unary_op, std::move(init));
}

// [adjacent.difference]/5: unlike the overloads without a policy (an accumulator and a copy of
// each element, binary_op(val, std::move(acc))), these pass the input elements themselves and
// make no T.
template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryOperation>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 adjacent_difference(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                     ForwardIterator2 result, BinaryOperation binary_op) noexcept {
  if (first == last)
    return result;
  *result = *first;
  ++result;
  for (ForwardIterator1 prev = first; ++first != last; prev = first, (void)++result)
    *result = binary_op(*first, *prev);
  return result;
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 adjacent_difference(ExecutionPolicy&& exec, ForwardIterator1 first, ForwardIterator1 last,
                                     ForwardIterator2 result) noexcept {
  return std::adjacent_difference(static_cast<ExecutionPolicy&&>(exec), first, last, result, minus<>());
}

} // namespace std
