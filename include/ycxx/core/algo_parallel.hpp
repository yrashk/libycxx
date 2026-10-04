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

namespace std {

template <class ExecutionPolicy, class ForwardIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool all_of(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Predicate pred) noexcept {
  return std::all_of(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool any_of(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Predicate pred) noexcept {
  return std::any_of(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool none_of(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Predicate pred) noexcept {
  return std::none_of(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator, class Function>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void for_each(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Function f) noexcept {
  std::for_each(first, last, f);
}

template <class ExecutionPolicy, class ForwardIterator, class Size, class Function>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator for_each_n(ExecutionPolicy&&, ForwardIterator first, Size n, Function f) noexcept {
  return std::for_each_n(first, n, f);
}

template <class ExecutionPolicy, class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator find(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, const T& value) noexcept {
  return std::find(first, last, value);
}

template <class ExecutionPolicy, class ForwardIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator find_if(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Predicate pred) noexcept {
  return std::find_if(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator find_if_not(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Predicate pred) noexcept {
  return std::find_if_not(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator1 find_end(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                          ForwardIterator2 last2) noexcept {
  return std::find_end(first1, last1, first2, last2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator1 find_end(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                          ForwardIterator2 last2, BinaryPredicate pred) noexcept {
  return std::find_end(first1, last1, first2, last2, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator1 find_first_of(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                               ForwardIterator2 first2, ForwardIterator2 last2) noexcept {
  return std::find_first_of(first1, last1, first2, last2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator1 find_first_of(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                               ForwardIterator2 first2, ForwardIterator2 last2, BinaryPredicate pred) noexcept {
  return std::find_first_of(first1, last1, first2, last2, pred);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator adjacent_find(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last) noexcept {
  return std::adjacent_find(first, last);
}

template <class ExecutionPolicy, class ForwardIterator, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator adjacent_find(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last,
                              BinaryPredicate pred) noexcept {
  return std::adjacent_find(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
typename iterator_traits<ForwardIterator>::difference_type count(ExecutionPolicy&&, ForwardIterator first,
                                                                 ForwardIterator last, const T& value) noexcept {
  return std::count(first, last, value);
}

template <class ExecutionPolicy, class ForwardIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
typename iterator_traits<ForwardIterator>::difference_type count_if(ExecutionPolicy&&, ForwardIterator first,
                                                                    ForwardIterator last, Predicate pred) noexcept {
  return std::count_if(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
pair<ForwardIterator1, ForwardIterator2> mismatch(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                                                  ForwardIterator2 first2) noexcept {
  return std::mismatch(first1, last1, first2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
pair<ForwardIterator1, ForwardIterator2> mismatch(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                                                  ForwardIterator2 first2, BinaryPredicate pred) noexcept {
  return std::mismatch(first1, last1, first2, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
pair<ForwardIterator1, ForwardIterator2> mismatch(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                                                  ForwardIterator2 first2, ForwardIterator2 last2) noexcept {
  return std::mismatch(first1, last1, first2, last2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
pair<ForwardIterator1, ForwardIterator2> mismatch(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                                                  ForwardIterator2 first2, ForwardIterator2 last2,
                                                  BinaryPredicate pred) noexcept {
  return std::mismatch(first1, last1, first2, last2, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool equal(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2) noexcept {
  return std::equal(first1, last1, first2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool equal(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
           BinaryPredicate pred) noexcept {
  return std::equal(first1, last1, first2, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool equal(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
           ForwardIterator2 last2) noexcept {
  return std::equal(first1, last1, first2, last2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool equal(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
           ForwardIterator2 last2, BinaryPredicate pred) noexcept {
  return std::equal(first1, last1, first2, last2, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator1 search(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                        ForwardIterator2 last2) noexcept {
  return std::search(first1, last1, first2, last2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator1 search(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                        ForwardIterator2 last2, BinaryPredicate pred) noexcept {
  return std::search(first1, last1, first2, last2, pred);
}

template <class ExecutionPolicy, class ForwardIterator, class Size,
          class T = typename iterator_traits<ForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator search_n(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Size count,
                         const T& value) noexcept {
  return std::search_n(first, last, count, value);
}

template <class ExecutionPolicy, class ForwardIterator, class Size,
          class T = typename iterator_traits<ForwardIterator>::value_type, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator search_n(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Size count, const T& value,
                         BinaryPredicate pred) noexcept {
  return std::search_n(first, last, count, value, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 copy(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                      ForwardIterator2 result) noexcept {
  return std::copy(first, last, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class Size, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 copy_n(ExecutionPolicy&&, ForwardIterator1 first, Size n, ForwardIterator2 result) noexcept {
  return std::copy_n(first, n, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 copy_if(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last, ForwardIterator2 result,
                         Predicate pred) noexcept {
  return std::copy_if(first, last, result, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 move(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                      ForwardIterator2 result) noexcept {
  return std::move(first, last, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 swap_ranges(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                             ForwardIterator2 first2) noexcept {
  return std::swap_ranges(first1, last1, first2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class UnaryOperation>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 transform(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 result,
                           UnaryOperation op) noexcept {
  return std::transform(first1, last1, result, op);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator,
          class BinaryOperation>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator transform(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                          ForwardIterator result, BinaryOperation binary_op) noexcept {
  return std::transform(first1, last1, first2, result, binary_op);
}

template <class ExecutionPolicy, class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void replace(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, const T& old_value,
             const T& new_value) noexcept {
  std::replace(first, last, old_value, new_value);
}

template <class ExecutionPolicy, class ForwardIterator, class Predicate,
          class T = typename iterator_traits<ForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void replace_if(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Predicate pred,
                const T& new_value) noexcept {
  std::replace_if(first, last, pred, new_value);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class T>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 replace_copy(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last, ForwardIterator2 result,
                              const T& old_value, const T& new_value) noexcept {
  return std::replace_copy(first, last, result, old_value, new_value);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class Predicate,
          class T = typename iterator_traits<ForwardIterator2>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 replace_copy_if(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                 ForwardIterator2 result, Predicate pred, const T& new_value) noexcept {
  return std::replace_copy_if(first, last, result, pred, new_value);
}

template <class ExecutionPolicy, class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void fill(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, const T& value) noexcept {
  std::fill(first, last, value);
}

template <class ExecutionPolicy, class ForwardIterator, class Size,
          class T = typename iterator_traits<ForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator fill_n(ExecutionPolicy&&, ForwardIterator first, Size n, const T& value) noexcept {
  return std::fill_n(first, n, value);
}

template <class ExecutionPolicy, class ForwardIterator, class Generator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void generate(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Generator gen) noexcept {
  std::generate(first, last, gen);
}

template <class ExecutionPolicy, class ForwardIterator, class Size, class Generator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator generate_n(ExecutionPolicy&&, ForwardIterator first, Size n, Generator gen) noexcept {
  return std::generate_n(first, n, gen);
}

template <class ExecutionPolicy, class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator remove(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, const T& value) noexcept {
  return std::remove(first, last, value);
}

template <class ExecutionPolicy, class ForwardIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator remove_if(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Predicate pred) noexcept {
  return std::remove_if(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2,
          class T = typename iterator_traits<ForwardIterator1>::value_type>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 remove_copy(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last, ForwardIterator2 result,
                             const T& value) noexcept {
  return std::remove_copy(first, last, result, value);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 remove_copy_if(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                                ForwardIterator2 result, Predicate pred) noexcept {
  return std::remove_copy_if(first, last, result, pred);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator unique(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last) noexcept {
  return std::unique(first, last);
}

template <class ExecutionPolicy, class ForwardIterator, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator unique(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, BinaryPredicate pred) noexcept {
  return std::unique(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 unique_copy(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last,
                             ForwardIterator2 result) noexcept {
  return std::unique_copy(first, last, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 unique_copy(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 last, ForwardIterator2 result,
                             BinaryPredicate pred) noexcept {
  return std::unique_copy(first, last, result, pred);
}

template <class ExecutionPolicy, class BidirectionalIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void reverse(ExecutionPolicy&&, BidirectionalIterator first, BidirectionalIterator last) noexcept {
  std::reverse(first, last);
}

template <class ExecutionPolicy, class BidirectionalIterator, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator reverse_copy(ExecutionPolicy&&, BidirectionalIterator first, BidirectionalIterator last,
                             ForwardIterator result) noexcept {
  return std::reverse_copy(first, last, result);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator rotate(ExecutionPolicy&&, ForwardIterator first, ForwardIterator middle,
                       ForwardIterator last) noexcept {
  return std::rotate(first, middle, last);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator2 rotate_copy(ExecutionPolicy&&, ForwardIterator1 first, ForwardIterator1 middle, ForwardIterator1 last,
                             ForwardIterator2 result) noexcept {
  return std::rotate_copy(first, middle, last, result);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator shift_left(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last,
                           typename iterator_traits<ForwardIterator>::difference_type n) noexcept {
  return std::shift_left(first, last, n);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator shift_right(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last,
                            typename iterator_traits<ForwardIterator>::difference_type n) noexcept {
  return std::shift_right(first, last, n);
}

template <class ExecutionPolicy, class RandomAccessIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void sort(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator last) noexcept {
  std::sort(first, last);
}

template <class ExecutionPolicy, class RandomAccessIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void sort(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator last, Compare comp) noexcept {
  std::sort(first, last, comp);
}

template <class ExecutionPolicy, class RandomAccessIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void stable_sort(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator last) noexcept {
  std::stable_sort(first, last);
}

template <class ExecutionPolicy, class RandomAccessIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void stable_sort(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator last, Compare comp) noexcept {
  std::stable_sort(first, last, comp);
}

template <class ExecutionPolicy, class RandomAccessIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void partial_sort(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator middle,
                  RandomAccessIterator last) noexcept {
  std::partial_sort(first, middle, last);
}

template <class ExecutionPolicy, class RandomAccessIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void partial_sort(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last,
                  Compare comp) noexcept {
  std::partial_sort(first, middle, last, comp);
}

template <class ExecutionPolicy, class ForwardIterator, class RandomAccessIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
RandomAccessIterator partial_sort_copy(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last,
                                       RandomAccessIterator result_first, RandomAccessIterator result_last) noexcept {
  return std::partial_sort_copy(first, last, result_first, result_last);
}

template <class ExecutionPolicy, class ForwardIterator, class RandomAccessIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
RandomAccessIterator partial_sort_copy(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last,
                                       RandomAccessIterator result_first, RandomAccessIterator result_last,
                                       Compare comp) noexcept {
  return std::partial_sort_copy(first, last, result_first, result_last, comp);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool is_sorted(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last) noexcept {
  return std::is_sorted(first, last);
}

template <class ExecutionPolicy, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool is_sorted(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Compare comp) noexcept {
  return std::is_sorted(first, last, comp);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator is_sorted_until(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last) noexcept {
  return std::is_sorted_until(first, last);
}

template <class ExecutionPolicy, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator is_sorted_until(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Compare comp) noexcept {
  return std::is_sorted_until(first, last, comp);
}

template <class ExecutionPolicy, class RandomAccessIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void nth_element(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator nth,
                 RandomAccessIterator last) noexcept {
  std::nth_element(first, nth, last);
}

template <class ExecutionPolicy, class RandomAccessIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void nth_element(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last,
                 Compare comp) noexcept {
  std::nth_element(first, nth, last, comp);
}

template <class ExecutionPolicy, class ForwardIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool is_partitioned(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Predicate pred) noexcept {
  return std::is_partitioned(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator partition(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Predicate pred) noexcept {
  return std::partition(first, last, pred);
}

template <class ExecutionPolicy, class BidirectionalIterator, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
BidirectionalIterator stable_partition(ExecutionPolicy&&, BidirectionalIterator first, BidirectionalIterator last,
                                       Predicate pred) noexcept {
  return std::stable_partition(first, last, pred);
}

template <class ExecutionPolicy, class ForwardIterator, class ForwardIterator1, class ForwardIterator2, class Predicate>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
pair<ForwardIterator1, ForwardIterator2> partition_copy(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last,
                                                        ForwardIterator1 out_true, ForwardIterator2 out_false,
                                                        Predicate pred) noexcept {
  return std::partition_copy(first, last, out_true, out_false, pred);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator merge(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                      ForwardIterator2 last2, ForwardIterator result) noexcept {
  return std::merge(first1, last1, first2, last2, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator merge(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                      ForwardIterator2 last2, ForwardIterator result, Compare comp) noexcept {
  return std::merge(first1, last1, first2, last2, result, comp);
}

template <class ExecutionPolicy, class BidirectionalIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void inplace_merge(ExecutionPolicy&&, BidirectionalIterator first, BidirectionalIterator middle,
                   BidirectionalIterator last) noexcept {
  std::inplace_merge(first, middle, last);
}

template <class ExecutionPolicy, class BidirectionalIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void inplace_merge(ExecutionPolicy&&, BidirectionalIterator first, BidirectionalIterator middle,
                   BidirectionalIterator last, Compare comp) noexcept {
  std::inplace_merge(first, middle, last, comp);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool includes(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
              ForwardIterator2 last2) noexcept {
  return std::includes(first1, last1, first2, last2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool includes(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
              ForwardIterator2 last2, Compare comp) noexcept {
  return std::includes(first1, last1, first2, last2, comp);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator set_union(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                          ForwardIterator2 last2, ForwardIterator result) noexcept {
  return std::set_union(first1, last1, first2, last2, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator set_union(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                          ForwardIterator2 last2, ForwardIterator result, Compare comp) noexcept {
  return std::set_union(first1, last1, first2, last2, result, comp);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator set_intersection(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                                 ForwardIterator2 first2, ForwardIterator2 last2, ForwardIterator result) noexcept {
  return std::set_intersection(first1, last1, first2, last2, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator set_intersection(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                                 ForwardIterator2 first2, ForwardIterator2 last2, ForwardIterator result,
                                 Compare comp) noexcept {
  return std::set_intersection(first1, last1, first2, last2, result, comp);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator set_difference(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                               ForwardIterator2 first2, ForwardIterator2 last2, ForwardIterator result) noexcept {
  return std::set_difference(first1, last1, first2, last2, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator set_difference(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                               ForwardIterator2 first2, ForwardIterator2 last2, ForwardIterator result,
                               Compare comp) noexcept {
  return std::set_difference(first1, last1, first2, last2, result, comp);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator set_symmetric_difference(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                                         ForwardIterator2 first2, ForwardIterator2 last2,
                                         ForwardIterator result) noexcept {
  return std::set_symmetric_difference(first1, last1, first2, last2, result);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator set_symmetric_difference(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                                         ForwardIterator2 first2, ForwardIterator2 last2, ForwardIterator result,
                                         Compare comp) noexcept {
  return std::set_symmetric_difference(first1, last1, first2, last2, result, comp);
}

template <class ExecutionPolicy, class RandomAccessIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool is_heap(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator last) noexcept {
  return std::is_heap(first, last);
}

template <class ExecutionPolicy, class RandomAccessIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool is_heap(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator last, Compare comp) noexcept {
  return std::is_heap(first, last, comp);
}

template <class ExecutionPolicy, class RandomAccessIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
RandomAccessIterator is_heap_until(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator last) noexcept {
  return std::is_heap_until(first, last);
}

template <class ExecutionPolicy, class RandomAccessIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
RandomAccessIterator is_heap_until(ExecutionPolicy&&, RandomAccessIterator first, RandomAccessIterator last,
                                   Compare comp) noexcept {
  return std::is_heap_until(first, last, comp);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator min_element(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last) noexcept {
  return std::min_element(first, last);
}

template <class ExecutionPolicy, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator min_element(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Compare comp) noexcept {
  return std::min_element(first, last, comp);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator max_element(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last) noexcept {
  return std::max_element(first, last);
}

template <class ExecutionPolicy, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
ForwardIterator max_element(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last, Compare comp) noexcept {
  return std::max_element(first, last, comp);
}

template <class ExecutionPolicy, class ForwardIterator>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
pair<ForwardIterator, ForwardIterator> minmax_element(ExecutionPolicy&&, ForwardIterator first,
                                                      ForwardIterator last) noexcept {
  return std::minmax_element(first, last);
}

template <class ExecutionPolicy, class ForwardIterator, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
pair<ForwardIterator, ForwardIterator> minmax_element(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last,
                                                      Compare comp) noexcept {
  return std::minmax_element(first, last, comp);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool lexicographical_compare(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                             ForwardIterator2 first2, ForwardIterator2 last2) noexcept {
  return std::lexicographical_compare(first1, last1, first2, last2);
}

template <class ExecutionPolicy, class ForwardIterator1, class ForwardIterator2, class Compare>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
bool lexicographical_compare(ExecutionPolicy&&, ForwardIterator1 first1, ForwardIterator1 last1,
                             ForwardIterator2 first2, ForwardIterator2 last2, Compare comp) noexcept {
  return std::lexicographical_compare(first1, last1, first2, last2, comp);
}

} // namespace std
