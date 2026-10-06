// libycxx core: the ExecutionPolicy overloads of the std:: algorithms
// ([algorithms.parallel.overloads]). They run the sequential algorithm on the calling thread,
// which every standard execution policy permits. They are noexcept: an exception leaving an
// element access function of a parallel algorithm calls terminate ([algorithms.parallel.exceptions]),
// and no temporary storage is ever required (the stable algorithms fall back to in-place versions).
// Each is the synopsis declaration forwarding to the overload without the policy.
#pragma once

#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/algo_nonmod.hpp>
#include <ycxx/core/algo_mutate.hpp>
#include <ycxx/core/algo_sort.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool all_of(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Predicate pred) noexcept {
  return std::all_of(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool any_of(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Predicate pred) noexcept {
  return std::any_of(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool none_of(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Predicate pred) noexcept {
  return std::none_of(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Function>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void for_each(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Function __f) noexcept {
  std::for_each(first, last, __f);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Size, class _Function>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator for_each_n(_ExecutionPolicy&&, _ForwardIterator first, _Size n, _Function __f) noexcept {
  return std::for_each_n(first, n, __f);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator find(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, const _Tp& value) noexcept {
  return std::find(first, last, value);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator find_if(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Predicate pred) noexcept {
  return std::find_if(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator find_if_not(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Predicate pred) noexcept {
  return std::find_if_not(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator1 find_end(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                          _ForwardIterator2 __last2) noexcept {
  return std::find_end(__first1, __last1, __first2, __last2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator1 find_end(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                          _ForwardIterator2 __last2, _BinaryPredicate pred) noexcept {
  return std::find_end(__first1, __last1, __first2, __last2, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator1 find_first_of(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                               _ForwardIterator2 __first2, _ForwardIterator2 __last2) noexcept {
  return std::find_first_of(__first1, __last1, __first2, __last2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator1 find_first_of(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                               _ForwardIterator2 __first2, _ForwardIterator2 __last2, _BinaryPredicate pred) noexcept {
  return std::find_first_of(__first1, __last1, __first2, __last2, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator adjacent_find(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last) noexcept {
  return std::adjacent_find(first, last);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator adjacent_find(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last,
                              _BinaryPredicate pred) noexcept {
  return std::adjacent_find(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
typename iterator_traits<_ForwardIterator>::difference_type count(_ExecutionPolicy&&, _ForwardIterator first,
                                                                 _ForwardIterator last, const _Tp& value) noexcept {
  return std::count(first, last, value);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
typename iterator_traits<_ForwardIterator>::difference_type count_if(_ExecutionPolicy&&, _ForwardIterator first,
                                                                    _ForwardIterator last, _Predicate pred) noexcept {
  return std::count_if(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
pair<_ForwardIterator1, _ForwardIterator2> mismatch(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                                  _ForwardIterator2 __first2) noexcept {
  return std::mismatch(__first1, __last1, __first2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
pair<_ForwardIterator1, _ForwardIterator2> mismatch(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                                  _ForwardIterator2 __first2, _BinaryPredicate pred) noexcept {
  return std::mismatch(__first1, __last1, __first2, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
pair<_ForwardIterator1, _ForwardIterator2> mismatch(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                                  _ForwardIterator2 __first2, _ForwardIterator2 __last2) noexcept {
  return std::mismatch(__first1, __last1, __first2, __last2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
pair<_ForwardIterator1, _ForwardIterator2> mismatch(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                                  _ForwardIterator2 __first2, _ForwardIterator2 __last2,
                                                  _BinaryPredicate pred) noexcept {
  return std::mismatch(__first1, __last1, __first2, __last2, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool equal(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2) noexcept {
  return std::equal(__first1, __last1, __first2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool equal(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
           _BinaryPredicate pred) noexcept {
  return std::equal(__first1, __last1, __first2, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool equal(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
           _ForwardIterator2 __last2) noexcept {
  return std::equal(__first1, __last1, __first2, __last2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool equal(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
           _ForwardIterator2 __last2, _BinaryPredicate pred) noexcept {
  return std::equal(__first1, __last1, __first2, __last2, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator1 search(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                        _ForwardIterator2 __last2) noexcept {
  return std::search(__first1, __last1, __first2, __last2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator1 search(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                        _ForwardIterator2 __last2, _BinaryPredicate pred) noexcept {
  return std::search(__first1, __last1, __first2, __last2, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Size,
          class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator search_n(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Size count,
                         const _Tp& value) noexcept {
  return std::search_n(first, last, count, value);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Size,
          class _Tp = typename iterator_traits<_ForwardIterator>::value_type, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator search_n(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Size count, const _Tp& value,
                         _BinaryPredicate pred) noexcept {
  return std::search_n(first, last, count, value, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 copy(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                      _ForwardIterator2 result) noexcept {
  return std::copy(first, last, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _Size, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 copy_n(_ExecutionPolicy&&, _ForwardIterator1 first, _Size n, _ForwardIterator2 result) noexcept {
  return std::copy_n(first, n, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 copy_if(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last, _ForwardIterator2 result,
                         _Predicate pred) noexcept {
  return std::copy_if(first, last, result, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 move(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                      _ForwardIterator2 result) noexcept {
  return std::move(first, last, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 swap_ranges(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                             _ForwardIterator2 __first2) noexcept {
  return std::swap_ranges(__first1, __last1, __first2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _UnaryOperation>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 transform(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 result,
                           _UnaryOperation op) noexcept {
  return std::transform(__first1, __last1, result, op);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator,
          class _BinaryOperation>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator transform(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                          _ForwardIterator result, _BinaryOperation __binary_op) noexcept {
  return std::transform(__first1, __last1, __first2, result, __binary_op);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void replace(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, const _Tp& __old_value,
             const _Tp& __new_value) noexcept {
  std::replace(first, last, __old_value, __new_value);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate,
          class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void replace_if(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Predicate pred,
                const _Tp& __new_value) noexcept {
  std::replace_if(first, last, pred, __new_value);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Tp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 replace_copy(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last, _ForwardIterator2 result,
                              const _Tp& __old_value, const _Tp& __new_value) noexcept {
  return std::replace_copy(first, last, result, __old_value, __new_value);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Predicate,
          class _Tp = typename iterator_traits<_ForwardIterator2>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 replace_copy_if(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                 _ForwardIterator2 result, _Predicate pred, const _Tp& __new_value) noexcept {
  return std::replace_copy_if(first, last, result, pred, __new_value);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void fill(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, const _Tp& value) noexcept {
  std::fill(first, last, value);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Size,
          class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator fill_n(_ExecutionPolicy&&, _ForwardIterator first, _Size n, const _Tp& value) noexcept {
  return std::fill_n(first, n, value);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Generator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void generate(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Generator __gen) noexcept {
  std::generate(first, last, __gen);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Size, class _Generator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator generate_n(_ExecutionPolicy&&, _ForwardIterator first, _Size n, _Generator __gen) noexcept {
  return std::generate_n(first, n, __gen);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator remove(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, const _Tp& value) noexcept {
  return std::remove(first, last, value);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator remove_if(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Predicate pred) noexcept {
  return std::remove_if(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2,
          class _Tp = typename iterator_traits<_ForwardIterator1>::value_type>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 remove_copy(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last, _ForwardIterator2 result,
                             const _Tp& value) noexcept {
  return std::remove_copy(first, last, result, value);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 remove_copy_if(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                                _ForwardIterator2 result, _Predicate pred) noexcept {
  return std::remove_copy_if(first, last, result, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator unique(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last) noexcept {
  return std::unique(first, last);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator unique(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _BinaryPredicate pred) noexcept {
  return std::unique(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 unique_copy(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last,
                             _ForwardIterator2 result) noexcept {
  return std::unique_copy(first, last, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 unique_copy(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 last, _ForwardIterator2 result,
                             _BinaryPredicate pred) noexcept {
  return std::unique_copy(first, last, result, pred);
}

template <class _ExecutionPolicy, class _BidirectionalIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void reverse(_ExecutionPolicy&&, _BidirectionalIterator first, _BidirectionalIterator last) noexcept {
  std::reverse(first, last);
}

template <class _ExecutionPolicy, class _BidirectionalIterator, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator reverse_copy(_ExecutionPolicy&&, _BidirectionalIterator first, _BidirectionalIterator last,
                             _ForwardIterator result) noexcept {
  return std::reverse_copy(first, last, result);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator rotate(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator __middle,
                       _ForwardIterator last) noexcept {
  return std::rotate(first, __middle, last);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator2 rotate_copy(_ExecutionPolicy&&, _ForwardIterator1 first, _ForwardIterator1 __middle, _ForwardIterator1 last,
                             _ForwardIterator2 result) noexcept {
  return std::rotate_copy(first, __middle, last, result);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator shift_left(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last,
                           typename iterator_traits<_ForwardIterator>::difference_type n) noexcept {
  return std::shift_left(first, last, n);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator shift_right(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last,
                            typename iterator_traits<_ForwardIterator>::difference_type n) noexcept {
  return std::shift_right(first, last, n);
}

template <class _ExecutionPolicy, class _RandomAccessIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void sort(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator last) noexcept {
  std::sort(first, last);
}

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void sort(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) noexcept {
  std::sort(first, last, comp);
}

template <class _ExecutionPolicy, class _RandomAccessIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void stable_sort(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator last) noexcept {
  std::stable_sort(first, last);
}

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void stable_sort(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) noexcept {
  std::stable_sort(first, last, comp);
}

template <class _ExecutionPolicy, class _RandomAccessIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void partial_sort(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator __middle,
                  _RandomAccessIterator last) noexcept {
  std::partial_sort(first, __middle, last);
}

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void partial_sort(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator __middle, _RandomAccessIterator last,
                  _Compare comp) noexcept {
  std::partial_sort(first, __middle, last, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _RandomAccessIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_RandomAccessIterator partial_sort_copy(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last,
                                       _RandomAccessIterator __result_first, _RandomAccessIterator __result_last) noexcept {
  return std::partial_sort_copy(first, last, __result_first, __result_last);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _RandomAccessIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_RandomAccessIterator partial_sort_copy(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last,
                                       _RandomAccessIterator __result_first, _RandomAccessIterator __result_last,
                                       _Compare comp) noexcept {
  return std::partial_sort_copy(first, last, __result_first, __result_last, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool is_sorted(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last) noexcept {
  return std::is_sorted(first, last);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool is_sorted(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Compare comp) noexcept {
  return std::is_sorted(first, last, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator is_sorted_until(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last) noexcept {
  return std::is_sorted_until(first, last);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator is_sorted_until(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Compare comp) noexcept {
  return std::is_sorted_until(first, last, comp);
}

template <class _ExecutionPolicy, class _RandomAccessIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void nth_element(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator __nth,
                 _RandomAccessIterator last) noexcept {
  std::nth_element(first, __nth, last);
}

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void nth_element(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator __nth, _RandomAccessIterator last,
                 _Compare comp) noexcept {
  std::nth_element(first, __nth, last, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool is_partitioned(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Predicate pred) noexcept {
  return std::is_partitioned(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator partition(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Predicate pred) noexcept {
  return std::partition(first, last, pred);
}

template <class _ExecutionPolicy, class _BidirectionalIterator, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_BidirectionalIterator stable_partition(_ExecutionPolicy&&, _BidirectionalIterator first, _BidirectionalIterator last,
                                       _Predicate pred) noexcept {
  return std::stable_partition(first, last, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _ForwardIterator1, class _ForwardIterator2, class _Predicate>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
pair<_ForwardIterator1, _ForwardIterator2> partition_copy(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last,
                                                        _ForwardIterator1 __out_true, _ForwardIterator2 __out_false,
                                                        _Predicate pred) noexcept {
  return std::partition_copy(first, last, __out_true, __out_false, pred);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator merge(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                      _ForwardIterator2 __last2, _ForwardIterator result) noexcept {
  return std::merge(__first1, __last1, __first2, __last2, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator merge(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                      _ForwardIterator2 __last2, _ForwardIterator result, _Compare comp) noexcept {
  return std::merge(__first1, __last1, __first2, __last2, result, comp);
}

template <class _ExecutionPolicy, class _BidirectionalIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void inplace_merge(_ExecutionPolicy&&, _BidirectionalIterator first, _BidirectionalIterator __middle,
                   _BidirectionalIterator last) noexcept {
  std::inplace_merge(first, __middle, last);
}

template <class _ExecutionPolicy, class _BidirectionalIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void inplace_merge(_ExecutionPolicy&&, _BidirectionalIterator first, _BidirectionalIterator __middle,
                   _BidirectionalIterator last, _Compare comp) noexcept {
  std::inplace_merge(first, __middle, last, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool includes(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
              _ForwardIterator2 __last2) noexcept {
  return std::includes(__first1, __last1, __first2, __last2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool includes(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
              _ForwardIterator2 __last2, _Compare comp) noexcept {
  return std::includes(__first1, __last1, __first2, __last2, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator set_union(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                          _ForwardIterator2 __last2, _ForwardIterator result) noexcept {
  return std::set_union(__first1, __last1, __first2, __last2, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator set_union(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                          _ForwardIterator2 __last2, _ForwardIterator result, _Compare comp) noexcept {
  return std::set_union(__first1, __last1, __first2, __last2, result, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator set_intersection(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                 _ForwardIterator2 __first2, _ForwardIterator2 __last2, _ForwardIterator result) noexcept {
  return std::set_intersection(__first1, __last1, __first2, __last2, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator set_intersection(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                 _ForwardIterator2 __first2, _ForwardIterator2 __last2, _ForwardIterator result,
                                 _Compare comp) noexcept {
  return std::set_intersection(__first1, __last1, __first2, __last2, result, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator set_difference(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                               _ForwardIterator2 __first2, _ForwardIterator2 __last2, _ForwardIterator result) noexcept {
  return std::set_difference(__first1, __last1, __first2, __last2, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator set_difference(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                               _ForwardIterator2 __first2, _ForwardIterator2 __last2, _ForwardIterator result,
                               _Compare comp) noexcept {
  return std::set_difference(__first1, __last1, __first2, __last2, result, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator set_symmetric_difference(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                         _ForwardIterator2 __first2, _ForwardIterator2 __last2,
                                         _ForwardIterator result) noexcept {
  return std::set_symmetric_difference(__first1, __last1, __first2, __last2, result);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator set_symmetric_difference(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                         _ForwardIterator2 __first2, _ForwardIterator2 __last2, _ForwardIterator result,
                                         _Compare comp) noexcept {
  return std::set_symmetric_difference(__first1, __last1, __first2, __last2, result, comp);
}

template <class _ExecutionPolicy, class _RandomAccessIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool is_heap(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator last) noexcept {
  return std::is_heap(first, last);
}

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool is_heap(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) noexcept {
  return std::is_heap(first, last, comp);
}

template <class _ExecutionPolicy, class _RandomAccessIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_RandomAccessIterator is_heap_until(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator last) noexcept {
  return std::is_heap_until(first, last);
}

template <class _ExecutionPolicy, class _RandomAccessIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_RandomAccessIterator is_heap_until(_ExecutionPolicy&&, _RandomAccessIterator first, _RandomAccessIterator last,
                                   _Compare comp) noexcept {
  return std::is_heap_until(first, last, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator min_element(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last) noexcept {
  return std::min_element(first, last);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator min_element(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Compare comp) noexcept {
  return std::min_element(first, last, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator max_element(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last) noexcept {
  return std::max_element(first, last);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_ForwardIterator max_element(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last, _Compare comp) noexcept {
  return std::max_element(first, last, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
pair<_ForwardIterator, _ForwardIterator> minmax_element(_ExecutionPolicy&&, _ForwardIterator first,
                                                      _ForwardIterator last) noexcept {
  return std::minmax_element(first, last);
}

template <class _ExecutionPolicy, class _ForwardIterator, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
pair<_ForwardIterator, _ForwardIterator> minmax_element(_ExecutionPolicy&&, _ForwardIterator first, _ForwardIterator last,
                                                      _Compare comp) noexcept {
  return std::minmax_element(first, last, comp);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool lexicographical_compare(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                             _ForwardIterator2 __first2, _ForwardIterator2 __last2) noexcept {
  return std::lexicographical_compare(__first1, __last1, __first2, __last2);
}

template <class _ExecutionPolicy, class _ForwardIterator1, class _ForwardIterator2, class _Compare>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
bool lexicographical_compare(_ExecutionPolicy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1,
                             _ForwardIterator2 __first2, _ForwardIterator2 __last2, _Compare comp) noexcept {
  return std::lexicographical_compare(__first1, __last1, __first2, __last2, comp);
}

} // namespace std
