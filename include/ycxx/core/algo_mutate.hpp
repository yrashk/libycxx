// libycxx core: mutating sequence operations ([alg.modifying.operations]) other than those in
// algo_base.hpp: transform, replace, generate, remove, unique, reverse, rotate, shift, sample
// and shuffle.
#pragma once

#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/urbg.hpp>

namespace ycxx::detail {

template <class Ops, class I, class S, class P>
constexpr I remove_if_impl(I first, S last, P pred) {
  first = ::ycxx::detail::find_if_impl(static_cast<I&&>(first), last, pred);
  if (first == last)
    return first;
  I out = first;
  for (++first; first != last; ++first)
    if (!pred(*first)) {
      *out = Ops::iter_move(first);
      ++out;
    }
  return out;
}

template <class I, class S, class O, class P>
constexpr std::pair<I, O> remove_copy_if_impl(I first, S last, O result, P pred) {
  for (; first != last; ++first)
    if (!pred(*first)) {
      *result = *first;
      ++result;
    }
  return {static_cast<I&&>(first), static_cast<O&&>(result)};
}

// Keeps the first element of every group of consecutive equivalent elements.
template <class Ops, class I, class S, class P>
constexpr I unique_impl(I first, S last, P eq) {
  first = ::ycxx::detail::adjacent_find_impl(static_cast<I&&>(first), last, eq);
  if (first == last)
    return first;
  I out = first; // *out is the last element kept; *++first is known to be its duplicate
  ++first;
  for (++first; first != last; ++first)
    if (!eq(*out, *first))
      *++out = Ops::iter_move(first);
  return ++out;
}

// unique_copy: Kind 0 compares with the last kept input element (forward input), 1 with the
// last element written (readable output of the same value type), 2 with a stored copy.
template <int Kind, class I, class S, class O, class P>
constexpr std::pair<I, O> unique_copy_impl(I first, S last, O result, P eq) {
  if (first == last)
    return {static_cast<I&&>(first), static_cast<O&&>(result)};
  if constexpr (Kind == 0) {
    I kept = first;
    *result = *first;
    ++result;
    while (++first != last)
      if (!eq(*kept, *first)) {
        kept = first;
        *result = *first;
        ++result;
      }
  } else if constexpr (Kind == 1) {
    O written = result;
    *result = *first;
    ++result;
    while (++first != last)
      if (!eq(*written, *first)) {
        written = result;
        *result = *first;
        ++result;
      }
  } else {
    std::iter_value_t<I> kept(*first);
    *result = kept;
    ++result;
    while (++first != last) {
      decltype(auto) x = *first;
      if (!eq(kept, x)) {
        kept = static_cast<decltype(x)&&>(x);
        *result = kept;
        ++result;
      }
    }
  }
  return {static_cast<I&&>(first), static_cast<O&&>(result)};
}

template <class Ops, class I>
constexpr void reverse_impl(I first, I last) {
  if constexpr (std::random_access_iterator<I> || ra_iter<I>) {
    if (first == last)
      return;
    for (--last; first < last; (void)++first, (void)--last)
      Ops::iter_swap(first, last);
  } else {
    while (first != last && first != --last) {
      Ops::iter_swap(first, last);
      ++first;
    }
  }
}

// Returns first + (last - middle). Bidirectional iterators: three reversals; forward iterators:
// block swapping. Either way at most last - first swaps.
template <class Ops, class I>
constexpr I rotate_impl(I first, I middle, I last) {
  if (first == middle)
    return last;
  if (middle == last)
    return first;
  if constexpr (std::bidirectional_iterator<I> || bidi_iter<I>) {
    I ret = first;
    ::ycxx::detail::iter_advance(ret, ::ycxx::detail::range_length(middle, last));
    ::ycxx::detail::reverse_impl<Ops>(first, middle);
    ::ycxx::detail::reverse_impl<Ops>(middle, last);
    ::ycxx::detail::reverse_impl<Ops>(first, last);
    return ret;
  } else {
    I next = middle;
    do {
      Ops::iter_swap(first, next);
      ++first;
      ++next;
      if (first == middle)
        middle = next;
    } while (next != last);
    I ret = first;
    next = middle;
    while (next != last) {
      Ops::iter_swap(first, next);
      ++first;
      ++next;
      if (first == middle)
        middle = next;
      else if (next == last)
        next = middle;
    }
    return ret;
  }
}

// [alg.shift]: returns NEW_LAST.
template <class Ops, class I, class S>
constexpr I shift_left_impl(I first, S last, std::iter_difference_t<I> n) {
  if (n <= 0)
    return ::ycxx::detail::iter_at(first, last);
  I mid = first;
  if constexpr (std::sized_sentinel_for<S, I>) {
    if (n >= last - first)
      return first;
    ::ycxx::detail::iter_advance(mid, n);
  } else {
    for (; n > 0; --n, (void)++mid)
      if (mid == last)
        return first;
  }
  return ::ycxx::detail::move_dispatch<Ops>(static_cast<I&&>(mid), last, static_cast<I&&>(first)).second;
}
// Returns {NEW_FIRST, end}.
template <class Ops, class I, class S>
constexpr std::pair<I, I> shift_right_impl(I first, S last, std::iter_difference_t<I> n) {
  if (n <= 0)
    return {first, ::ycxx::detail::iter_at(first, last)};
  if constexpr (std::bidirectional_iterator<I> || (std::same_as<I, S> && bidi_iter<I>)) {
    I end = ::ycxx::detail::iter_at(first, last);
    if (::ycxx::detail::range_length(first, end) <= n)
      return {end, end};
    I mid = end;
    ::ycxx::detail::iter_advance(mid, -n);
    ::ycxx::detail::move_backward_dispatch<Ops>(first, mid, end);
    ::ycxx::detail::iter_advance(first, n);
    return {first, end};
  } else {
    // Forward iterators: [first, result) is a ring of elements waiting to be placed; each
    // position from result on swaps its element with the oldest waiting one.
    I result = first;
    for (std::iter_difference_t<I> k = n; k > 0; --k, (void)++result)
      if (result == last)
        return {result, result};
    I slot = first;
    I p = result;
    for (; p != last; ++p) {
      Ops::iter_swap(p, slot);
      if (++slot == result)
        slot = first;
    }
    return {result, p};
  }
}

// ---- uniform integers from a URBG ([rand.req.urng]) -------------------------------------------
// A uniformly distributed value in [0, n], by rejection; several draws are combined when the
// generator's range is smaller than n + 1.
template <class G>
unsigned long long uniform_upto(G& g, unsigned long long n) {
  using R = std::remove_cvref_t<decltype(g())>;
  constexpr unsigned long long gmin = static_cast<unsigned long long>(std::remove_reference_t<G>::min());
  constexpr unsigned long long range = static_cast<unsigned long long>(std::remove_reference_t<G>::max()) - gmin;
  auto draw = [&g] { return static_cast<unsigned long long>(static_cast<R>(g())) - gmin; };
  if (n == range)
    return draw();
  if (n < range) {
    // Accept v below the largest multiple of n + 1 that fits in [0, range + 1).
    const unsigned long long m = n + 1;
    const unsigned long long excess = range == ~0ull ? (~0ull % m + 1) % m : (range + 1) % m;
    const unsigned long long limit = range - excess; // values in [0, limit] are accepted
    for (;;) {
      unsigned long long v = draw();
      if (v <= limit)
        return v % m;
    }
  }
  // n > range: v = hi * (range + 1) + lo with hi uniform in [0, n / (range + 1)].
  const unsigned long long base = range + 1;
  for (;;) {
    unsigned long long hi = ::ycxx::detail::uniform_upto(g, n / base);
    unsigned long long lo = draw();
    if (hi <= (n - lo) / base)
      return hi * base + lo;
  }
}

} // namespace ycxx::detail

// =============================================================================================
// std:: forms
// =============================================================================================
namespace std {

// [alg.transform]
template <class InputIterator, class OutputIterator, class UnaryOperation>
constexpr OutputIterator transform(InputIterator first1, InputIterator last1, OutputIterator result,
                                   UnaryOperation op) {
  for (; first1 != last1; (void)++first1, (void)++result)
    *result = op(*first1);
  return result;
}
template <class InputIterator1, class InputIterator2, class OutputIterator, class BinaryOperation>
constexpr OutputIterator transform(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                   OutputIterator result, BinaryOperation binary_op) {
  for (; first1 != last1; (void)++first1, (void)++first2, (void)++result)
    *result = binary_op(*first1, *first2);
  return result;
}

// [alg.replace]
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
constexpr void replace(ForwardIterator first, ForwardIterator last, const T& old_value, const T& new_value) {
  for (; first != last; ++first)
    if (*first == old_value)
      *first = new_value;
}
template <class ForwardIterator, class Predicate, class T = typename iterator_traits<ForwardIterator>::value_type>
constexpr void replace_if(ForwardIterator first, ForwardIterator last, Predicate pred, const T& new_value) {
  for (; first != last; ++first)
    if (pred(*first))
      *first = new_value;
}
template <class InputIterator, class OutputIterator, class T>
constexpr OutputIterator replace_copy(InputIterator first, InputIterator last, OutputIterator result,
                                      const T& old_value, const T& new_value) {
  for (; first != last; (void)++first, (void)++result)
    if (*first == old_value)
      *result = new_value;
    else
      *result = *first;
  return result;
}
template <class InputIterator, class OutputIterator, class Predicate,
          class T = typename iterator_traits<OutputIterator>::value_type>
constexpr OutputIterator replace_copy_if(InputIterator first, InputIterator last, OutputIterator result,
                                         Predicate pred, const T& new_value) {
  for (; first != last; (void)++first, (void)++result)
    if (pred(*first))
      *result = new_value;
    else
      *result = *first;
  return result;
}

// [alg.generate]
template <class ForwardIterator, class Generator>
constexpr void generate(ForwardIterator first, ForwardIterator last, Generator gen) {
  for (; first != last; ++first)
    *first = gen();
}
template <class OutputIterator, class Size, class Generator>
constexpr OutputIterator generate_n(OutputIterator first, Size n, Generator gen) {
  for (auto count = ::ycxx::detail::integral_count(n); count > 0; --count) {
    *first = gen();
    ++first;
  }
  return first;
}

// [alg.remove]
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
[[nodiscard]] constexpr ForwardIterator remove(ForwardIterator first, ForwardIterator last, const T& value) {
  return ::ycxx::detail::remove_if_impl<ycxx::detail::classic_ops>(first, last,
                                                                   ::ycxx::detail::equals_value_plain<T>{value});
}
template <class ForwardIterator, class Predicate>
[[nodiscard]] constexpr ForwardIterator remove_if(ForwardIterator first, ForwardIterator last, Predicate pred) {
  return ::ycxx::detail::remove_if_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(pred));
}
template <class InputIterator, class OutputIterator, class T = typename iterator_traits<InputIterator>::value_type>
constexpr OutputIterator remove_copy(InputIterator first, InputIterator last, OutputIterator result, const T& value) {
  return ::ycxx::detail::remove_copy_if_impl(first, last, result, ::ycxx::detail::equals_value_plain<T>{value}).second;
}
template <class InputIterator, class OutputIterator, class Predicate>
constexpr OutputIterator remove_copy_if(InputIterator first, InputIterator last, OutputIterator result,
                                        Predicate pred) {
  return ::ycxx::detail::remove_copy_if_impl(first, last, result, ::ycxx::detail::ref_pred(pred)).second;
}

// [alg.unique]
template <class ForwardIterator, class BinaryPredicate>
[[nodiscard]] constexpr ForwardIterator unique(ForwardIterator first, ForwardIterator last, BinaryPredicate pred) {
  return ::ycxx::detail::unique_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(pred));
}
template <class ForwardIterator>
[[nodiscard]] constexpr ForwardIterator unique(ForwardIterator first, ForwardIterator last) {
  return std::unique(first, last, equal_to<>{});
}
template <class InputIterator, class OutputIterator, class BinaryPredicate>
constexpr OutputIterator unique_copy(InputIterator first, InputIterator last, OutputIterator result,
                                     BinaryPredicate pred) {
  using VI = typename iterator_traits<InputIterator>::value_type;
  constexpr int kind = ::ycxx::detail::fwd_iter<InputIterator> ? 0
                       : (::ycxx::detail::fwd_iter<OutputIterator> &&
                          requires { requires is_same_v<typename iterator_traits<OutputIterator>::value_type, VI>; })
                           ? 1
                           : 2;
  return ::ycxx::detail::unique_copy_impl<kind>(first, last, result, ::ycxx::detail::ref_pred(pred)).second;
}
template <class InputIterator, class OutputIterator>
constexpr OutputIterator unique_copy(InputIterator first, InputIterator last, OutputIterator result) {
  return std::unique_copy(first, last, result, equal_to<>{});
}

// [alg.reverse]
template <class BidirectionalIterator>
constexpr void reverse(BidirectionalIterator first, BidirectionalIterator last) {
  ::ycxx::detail::reverse_impl<ycxx::detail::classic_ops>(first, last);
}
template <class BidirectionalIterator, class OutputIterator>
constexpr OutputIterator reverse_copy(BidirectionalIterator first, BidirectionalIterator last, OutputIterator result) {
  for (; first != last; ++result)
    *result = *--last;
  return result;
}

// [alg.rotate]
template <class ForwardIterator>
constexpr ForwardIterator rotate(ForwardIterator first, ForwardIterator middle, ForwardIterator last) {
  return ::ycxx::detail::rotate_impl<ycxx::detail::classic_ops>(first, middle, last);
}
template <class ForwardIterator, class OutputIterator>
constexpr OutputIterator rotate_copy(ForwardIterator first, ForwardIterator middle, ForwardIterator last,
                                     OutputIterator result) {
  result = ::ycxx::detail::copy_dispatch(middle, last, result).second;
  return ::ycxx::detail::copy_dispatch(first, middle, result).second;
}

// [alg.random.sample]
template <class PopulationIterator, class SampleIterator, class Distance, class UniformRandomBitGenerator>
SampleIterator sample(PopulationIterator first, PopulationIterator last, SampleIterator out, Distance n,
                      UniformRandomBitGenerator&& g) {
  using D = common_type_t<Distance, typename iterator_traits<PopulationIterator>::difference_type>;
  D want = static_cast<D>(n);
  if (want <= 0)
    return out;
  if constexpr (::ycxx::detail::fwd_iter<PopulationIterator>) {
    // Selection sampling: stable.
    D left = static_cast<D>(::ycxx::detail::range_length(first, last));
    for (; want > 0 && first != last; (void)++first, --left)
      if (static_cast<D>(::ycxx::detail::uniform_upto(g, static_cast<unsigned long long>(left - 1))) < want) {
        *out = *first;
        ++out;
        --want;
      }
    return out;
  } else {
    // Reservoir sampling into the random-access output.
    D k = 0;
    for (; k < want && first != last; (void)++first, ++k)
      out[k] = *first;
    for (; first != last; (void)++first, ++k) {
      D j = static_cast<D>(::ycxx::detail::uniform_upto(g, static_cast<unsigned long long>(k)));
      if (j < want)
        out[j] = *first;
    }
    return out + (k < want ? k : want);
  }
}

// [alg.random.shuffle]
template <class RandomAccessIterator, class UniformRandomBitGenerator>
void shuffle(RandomAccessIterator first, RandomAccessIterator last, UniformRandomBitGenerator&& g) {
  using D = typename iterator_traits<RandomAccessIterator>::difference_type;
  D n = last - first;
  for (D i = 1; i < n; ++i) {
    D j = static_cast<D>(::ycxx::detail::uniform_upto(g, static_cast<unsigned long long>(i)));
    if (j != i) // no self-swap: it would move an element onto itself
      std::iter_swap(first + i, first + j);
  }
}

// [alg.shift]
template <class ForwardIterator>
constexpr ForwardIterator shift_left(ForwardIterator first, ForwardIterator last,
                                     typename iterator_traits<ForwardIterator>::difference_type n) {
  ycxx::detail::precondition(n >= 0, "std::shift_left: negative n");
  return ::ycxx::detail::shift_left_impl<ycxx::detail::classic_ops>(first, last, n);
}
template <class ForwardIterator>
constexpr ForwardIterator shift_right(ForwardIterator first, ForwardIterator last,
                                      typename iterator_traits<ForwardIterator>::difference_type n) {
  ycxx::detail::precondition(n >= 0, "std::shift_right: negative n");
  return ::ycxx::detail::shift_right_impl<ycxx::detail::classic_ops>(first, last, n).first;
}

} // namespace std

// =============================================================================================
// std::ranges:: forms
// =============================================================================================
namespace std::ranges {
template <class I, class O>
using unary_transform_result = in_out_result<I, O>;
template <class I1, class I2, class O>
using binary_transform_result = in_in_out_result<I1, I2, O>;
template <class I, class O>
using replace_copy_result = in_out_result<I, O>;
template <class I, class O>
using replace_copy_if_result = in_out_result<I, O>;
template <class I, class O>
using remove_copy_result = in_out_result<I, O>;
template <class I, class O>
using remove_copy_if_result = in_out_result<I, O>;
template <class I, class O>
using unique_copy_result = in_out_result<I, O>;
template <class I, class O>
using reverse_copy_result = in_out_result<I, O>;
template <class I, class O>
using rotate_copy_result = in_out_result<I, O>;
// reverse_copy_truncated_result, rotate_copy_truncated_result: algo_ranges_parallel.hpp.
} // namespace std::ranges

namespace ycxx::detail::ranges_algo {

using std::ranges::borrowed_iterator_t;
using std::ranges::borrowed_subrange_t;
using std::ranges::iterator_t;

// [alg.transform]
struct transform_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O, std::copy_constructible F,
            class Proj = std::identity>
    requires std::indirectly_writable<O, std::indirect_result_t<F&, std::projected<I, Proj>>>
  constexpr std::ranges::unary_transform_result<I, O> operator()(I first1, S last1, O result, F op,
                                                                Proj proj = {}) const {
    for (; first1 != last1; (void)++first1, (void)++result)
      *result = ::ycxx::detail::invoke(op, ::ycxx::detail::invoke(proj, *first1));
    return {std::move(first1), std::move(result)};
  }
  template <std::ranges::input_range R, std::weakly_incrementable O, std::copy_constructible F,
            class Proj = std::identity>
    requires std::indirectly_writable<O, std::indirect_result_t<F&, std::projected<iterator_t<R>, Proj>>>
  constexpr std::ranges::unary_transform_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result, F op,
                                                                                     Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(op), std::move(proj));
  }
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            std::weakly_incrementable O, std::copy_constructible F, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires std::indirectly_writable<O, std::indirect_result_t<F&, std::projected<I1, Proj1>, std::projected<I2, Proj2>>>
  constexpr std::ranges::binary_transform_result<I1, I2, O> operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                                                                      O result, F binary_op, Proj1 proj1 = {},
                                                                      Proj2 proj2 = {}) const {
    for (; first1 != last1 && first2 != last2; (void)++first1, (void)++first2, (void)++result)
      *result = ::ycxx::detail::invoke(binary_op, ::ycxx::detail::invoke(proj1, *first1),
                                       ::ycxx::detail::invoke(proj2, *first2));
    return {std::move(first1), std::move(first2), std::move(result)};
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, std::weakly_incrementable O,
            std::copy_constructible F, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_writable<O, std::indirect_result_t<F&, std::projected<iterator_t<R1>, Proj1>,
                                                                std::projected<iterator_t<R2>, Proj2>>>
  constexpr std::ranges::binary_transform_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>
  operator()(R1&& r1, R2&& r2, O result, F binary_op, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(result), std::move(binary_op), std::move(proj1), std::move(proj2));
  }
};

// [alg.replace]
struct replace_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            class T1 = std::projected_value_t<I, Proj>, class T2 = std::iter_value_t<I>>
    requires std::indirectly_writable<I, const T2&> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T1*>
  constexpr I operator()(I first, S last, const T1& old_value, const T2& new_value, Proj proj = {}) const {
    for (; first != last; ++first)
      if (::ycxx::detail::invoke(proj, *first) == old_value)
        *first = new_value;
    return first;
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            class T1 = std::projected_value_t<iterator_t<R>, Proj>, class T2 = std::ranges::range_value_t<R>>
    requires std::indirectly_writable<iterator_t<R>, const T2&> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<R>, Proj>, const T1*>
  constexpr borrowed_iterator_t<R> operator()(R&& r, const T1& old_value, const T2& new_value, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), old_value, new_value, std::move(proj));
  }
};
struct replace_if_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity, class T = std::iter_value_t<I>,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires std::indirectly_writable<I, const T&>
  constexpr I operator()(I first, S last, Pred pred, const T& new_value, Proj proj = {}) const {
    for (; first != last; ++first)
      if (::ycxx::detail::invoke(pred, ::ycxx::detail::invoke(proj, *first)))
        *first = new_value;
    return first;
  }
  template <std::ranges::input_range R, class Proj = std::identity, class T = std::ranges::range_value_t<R>,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
    requires std::indirectly_writable<iterator_t<R>, const T&>
  constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, const T& new_value, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), new_value, std::move(proj));
  }
};
struct replace_copy_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class O, class Proj = std::identity,
            class T1 = std::projected_value_t<I, Proj>, class T2 = std::iter_value_t<O>>
    requires std::indirectly_copyable<I, O> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T1*> &&
             std::output_iterator<O, const T2&>
  constexpr std::ranges::replace_copy_result<I, O> operator()(I first, S last, O result, const T1& old_value,
                                                             const T2& new_value, Proj proj = {}) const {
    for (; first != last; (void)++first, (void)++result)
      if (::ycxx::detail::invoke(proj, *first) == old_value)
        *result = new_value;
      else
        *result = *first;
    return {std::move(first), std::move(result)};
  }
  template <std::ranges::input_range R, class O, class Proj = std::identity,
            class T1 = std::projected_value_t<iterator_t<R>, Proj>, class T2 = std::iter_value_t<O>>
    requires std::indirectly_copyable<iterator_t<R>, O> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<R>, Proj>, const T1*> &&
             std::output_iterator<O, const T2&>
  constexpr std::ranges::replace_copy_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result,
                                                                                  const T1& old_value,
                                                                                  const T2& new_value,
                                                                                  Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), old_value, new_value, std::move(proj));
  }
};
struct replace_copy_if_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class O, class T = std::iter_value_t<O>,
            class Proj = std::identity, std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires std::indirectly_copyable<I, O> && std::output_iterator<O, const T&>
  constexpr std::ranges::replace_copy_if_result<I, O> operator()(I first, S last, O result, Pred pred,
                                                                const T& new_value, Proj proj = {}) const {
    for (; first != last; (void)++first, (void)++result)
      if (::ycxx::detail::invoke(pred, ::ycxx::detail::invoke(proj, *first)))
        *result = new_value;
      else
        *result = *first;
    return {std::move(first), std::move(result)};
  }
  template <std::ranges::input_range R, class O, class T = std::iter_value_t<O>, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
    requires std::indirectly_copyable<iterator_t<R>, O> && std::output_iterator<O, const T&>
  constexpr std::ranges::replace_copy_if_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result, Pred pred,
                                                                                     const T& new_value,
                                                                                     Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(pred), new_value,
                   std::move(proj));
  }
};

// [alg.generate]
struct generate_fn {
  template <std::input_or_output_iterator O, std::sentinel_for<O> S, std::copy_constructible F>
    requires std::invocable<F&> && std::indirectly_writable<O, std::invoke_result_t<F&>>
  constexpr O operator()(O first, S last, F gen) const {
    for (; first != last; ++first)
      *first = ::ycxx::detail::invoke(gen);
    return first;
  }
  template <class R, std::copy_constructible F>
    requires std::invocable<F&> && std::ranges::output_range<R, std::invoke_result_t<F&>>
  constexpr borrowed_iterator_t<R> operator()(R&& r, F gen) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(gen));
  }
};
struct generate_n_fn {
  template <std::input_or_output_iterator O, std::copy_constructible F>
    requires std::invocable<F&> && std::indirectly_writable<O, std::invoke_result_t<F&>>
  constexpr O operator()(O first, std::iter_difference_t<O> n, F gen) const {
    for (; n > 0; --n) {
      *first = ::ycxx::detail::invoke(gen);
      ++first;
    }
    return first;
  }
};

// [alg.remove]
struct remove_fn {
  template <std::permutable I, std::sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  constexpr std::ranges::subrange<I> operator()(I first, S last, const T& value, Proj proj = {}) const {
    I end = ::ycxx::detail::remove_if_impl<ranges_ops>(first, last, ::ycxx::detail::equals_value<T, Proj>{value, proj});
    return {std::move(end), ::ycxx::detail::iter_at(first, last)};
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>>
    requires std::permutable<iterator_t<R>> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<R>, Proj>, const T*>
  constexpr borrowed_subrange_t<R> operator()(R&& r, const T& value, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
  }
};
struct remove_if_fn {
  template <std::permutable I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  constexpr std::ranges::subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) const {
    I end = ::ycxx::detail::remove_if_impl<ranges_ops>(first, last, ::ycxx::detail::make_pred(pred, proj));
    return {std::move(end), ::ycxx::detail::iter_at(first, last)};
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
    requires std::permutable<iterator_t<R>>
  constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct remove_copy_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires std::indirectly_copyable<I, O> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  constexpr std::ranges::remove_copy_result<I, O> operator()(I first, S last, O result, const T& value,
                                                            Proj proj = {}) const {
    auto r = ::ycxx::detail::remove_copy_if_impl(std::move(first), std::move(last), std::move(result),
                                                 ::ycxx::detail::equals_value<T, Proj>{value, proj});
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range R, std::weakly_incrementable O, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>>
    requires std::indirectly_copyable<iterator_t<R>, O> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<R>, Proj>, const T*>
  constexpr std::ranges::remove_copy_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result, const T& value,
                                                                                 Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), value, std::move(proj));
  }
};
struct remove_copy_if_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires std::indirectly_copyable<I, O>
  constexpr std::ranges::remove_copy_if_result<I, O> operator()(I first, S last, O result, Pred pred,
                                                               Proj proj = {}) const {
    auto r = ::ycxx::detail::remove_copy_if_impl(std::move(first), std::move(last), std::move(result),
                                                 ::ycxx::detail::make_pred(pred, proj));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range R, std::weakly_incrementable O, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
    requires std::indirectly_copyable<iterator_t<R>, O>
  constexpr std::ranges::remove_copy_if_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result, Pred pred,
                                                                                    Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(pred), std::move(proj));
  }
};

// [alg.unique]
struct unique_fn {
  template <std::permutable I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<I, Proj>> C = std::ranges::equal_to>
  constexpr std::ranges::subrange<I> operator()(I first, S last, C comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::unique_impl<ranges_ops>(first, last, ::ycxx::detail::make_comp(comp, proj));
    return {std::move(end), ::ycxx::detail::iter_at(first, last)};
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<iterator_t<R>, Proj>> C = std::ranges::equal_to>
    requires std::permutable<iterator_t<R>>
  constexpr borrowed_subrange_t<R> operator()(R&& r, C comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct unique_copy_fn {
  template <class I, class O>
  static consteval int kind() {
    if constexpr (std::forward_iterator<I>)
      return 0;
    else if constexpr (requires { requires std::input_iterator<O>; } &&
                       requires { requires std::same_as<std::iter_value_t<I>, std::iter_value_t<O>>; })
      return 1;
    else
      return 2;
  }
  template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O, class Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<I, Proj>> C = std::ranges::equal_to>
    requires std::indirectly_copyable<I, O> &&
             (std::forward_iterator<I> || (std::input_iterator<O> && std::same_as<std::iter_value_t<I>, std::iter_value_t<O>>) ||
              std::indirectly_copyable_storable<I, O>)
  constexpr std::ranges::unique_copy_result<I, O> operator()(I first, S last, O result, C comp = {},
                                                            Proj proj = {}) const {
    auto r = ::ycxx::detail::unique_copy_impl<kind<I, O>()>(std::move(first), std::move(last), std::move(result),
                                                          ::ycxx::detail::make_comp(comp, proj));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range R, std::weakly_incrementable O, class Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<iterator_t<R>, Proj>> C = std::ranges::equal_to>
    requires std::indirectly_copyable<iterator_t<R>, O> &&
             (std::forward_iterator<iterator_t<R>> ||
              (std::input_iterator<O> && std::same_as<std::ranges::range_value_t<R>, std::iter_value_t<O>>) ||
              std::indirectly_copyable_storable<iterator_t<R>, O>)
  constexpr std::ranges::unique_copy_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result, C comp = {},
                                                                                 Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(comp), std::move(proj));
  }
};

// [alg.reverse]
struct reverse_fn {
  template <std::bidirectional_iterator I, std::sentinel_for<I> S>
    requires std::permutable<I>
  constexpr I operator()(I first, S last) const {
    I end = ::ycxx::detail::iter_at(std::move(first), last);
    ::ycxx::detail::reverse_impl<ranges_ops>(first, end);
    return end;
  }
  template <std::ranges::bidirectional_range R>
    requires std::permutable<iterator_t<R>>
  constexpr borrowed_iterator_t<R> operator()(R&& r) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r));
  }
};
struct reverse_copy_fn {
  template <std::bidirectional_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O>
    requires std::indirectly_copyable<I, O>
  constexpr std::ranges::reverse_copy_result<I, O> operator()(I first, S last, O result) const {
    I end = ::ycxx::detail::iter_at(first, last);
    for (I it = end; it != first; ++result)
      *result = *--it;
    return {std::move(end), std::move(result)};
  }
  template <std::ranges::bidirectional_range R, std::weakly_incrementable O>
    requires std::indirectly_copyable<iterator_t<R>, O>
  constexpr std::ranges::reverse_copy_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};

// [alg.rotate]
struct rotate_fn {
  template <std::permutable I, std::sentinel_for<I> S>
  constexpr std::ranges::subrange<I> operator()(I first, I middle, S last) const {
    I end = ::ycxx::detail::iter_at(middle, std::move(last));
    I r = ::ycxx::detail::rotate_impl<ranges_ops>(std::move(first), std::move(middle), end);
    return {std::move(r), std::move(end)};
  }
  template <std::ranges::forward_range R>
    requires std::permutable<iterator_t<R>>
  constexpr borrowed_subrange_t<R> operator()(R&& r, iterator_t<R> middle) const {
    return (*this)(std::ranges::begin(r), std::move(middle), std::ranges::end(r));
  }
};
struct rotate_copy_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O>
    requires std::indirectly_copyable<I, O>
  constexpr std::ranges::rotate_copy_result<I, O> operator()(I first, I middle, S last, O result) const {
    auto r = ::ycxx::detail::copy_dispatch(middle, std::move(last), std::move(result));
    auto r2 = ::ycxx::detail::copy_dispatch(std::move(first), std::move(middle), std::move(r.second));
    return {std::move(r.first), std::move(r2.second)};
  }
  template <std::ranges::forward_range R, std::weakly_incrementable O>
    requires std::indirectly_copyable<iterator_t<R>, O>
  constexpr std::ranges::rotate_copy_result<borrowed_iterator_t<R>, O> operator()(R&& r, iterator_t<R> middle,
                                                                                 O result) const {
    return (*this)(std::ranges::begin(r), std::move(middle), std::ranges::end(r), std::move(result));
  }
};

// [alg.shift]
struct shift_left_fn {
  template <std::permutable I, std::sentinel_for<I> S>
  constexpr std::ranges::subrange<I> operator()(I first, S last, std::iter_difference_t<I> n) const {
    ::ycxx::detail::precondition(n >= 0, "ranges::shift_left: negative n");
    I end = ::ycxx::detail::shift_left_impl<ranges_ops>(first, std::move(last), n);
    return {std::move(first), std::move(end)};
  }
  template <std::ranges::forward_range R>
    requires std::permutable<iterator_t<R>>
  constexpr borrowed_subrange_t<R> operator()(R&& r, std::ranges::range_difference_t<R> n) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), n);
  }
};
struct shift_right_fn {
  template <std::permutable I, std::sentinel_for<I> S>
  constexpr std::ranges::subrange<I> operator()(I first, S last, std::iter_difference_t<I> n) const {
    ::ycxx::detail::precondition(n >= 0, "ranges::shift_right: negative n");
    auto r = ::ycxx::detail::shift_right_impl<ranges_ops>(std::move(first), std::move(last), n);
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range R>
    requires std::permutable<iterator_t<R>>
  constexpr borrowed_subrange_t<R> operator()(R&& r, std::ranges::range_difference_t<R> n) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), n);
  }
};

// [alg.random.sample], [alg.random.shuffle]
struct sample_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O, class Gen>
    requires(std::forward_iterator<I> || std::random_access_iterator<O>) && std::indirectly_copyable<I, O> &&
            std::uniform_random_bit_generator<std::remove_reference_t<Gen>>
  O operator()(I first, S last, O out, std::iter_difference_t<I> n, Gen&& g) const {
    using D = std::iter_difference_t<I>;
    if (n <= 0)
      return out;
    if constexpr (std::forward_iterator<I>) {
      D left = std::ranges::distance(first, last);
      for (; n > 0 && first != last; (void)++first, --left)
        if (static_cast<D>(::ycxx::detail::uniform_upto(g, static_cast<unsigned long long>(left - 1))) < n) {
          *out = *first;
          ++out;
          --n;
        }
      return out;
    } else {
      using OD = std::iter_difference_t<O>;
      D k = 0;
      for (; k < n && first != last; (void)++first, ++k)
        out[static_cast<OD>(k)] = *first;
      for (; first != last; (void)++first, ++k) {
        D j = static_cast<D>(::ycxx::detail::uniform_upto(g, static_cast<unsigned long long>(k)));
        if (j < n)
          out[static_cast<OD>(j)] = *first;
      }
      return out + static_cast<OD>(k < n ? k : n);
    }
  }
  template <std::ranges::input_range R, std::weakly_incrementable O, class Gen>
    requires(std::ranges::forward_range<R> || std::random_access_iterator<O>) &&
            std::indirectly_copyable<iterator_t<R>, O> && std::uniform_random_bit_generator<std::remove_reference_t<Gen>>
  O operator()(R&& r, O out, std::ranges::range_difference_t<R> n, Gen&& g) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(out), n, static_cast<Gen&&>(g));
  }
};
struct shuffle_fn {
  template <std::random_access_iterator I, std::sentinel_for<I> S, class Gen>
    requires std::permutable<I> && std::uniform_random_bit_generator<std::remove_reference_t<Gen>>
  I operator()(I first, S last, Gen&& g) const {
    using D = std::iter_difference_t<I>;
    I end = ::ycxx::detail::iter_at(first, std::move(last));
    D n = end - first;
    for (D i = 1; i < n; ++i) {
      D j = static_cast<D>(::ycxx::detail::uniform_upto(g, static_cast<unsigned long long>(i)));
      if (j != i)
        std::ranges::iter_swap(first + i, first + j);
    }
    return end;
  }
  template <std::ranges::random_access_range R, class Gen>
    requires std::permutable<iterator_t<R>> && std::uniform_random_bit_generator<std::remove_reference_t<Gen>>
  borrowed_iterator_t<R> operator()(R&& r, Gen&& g) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), static_cast<Gen&&>(g));
  }
};

} // namespace ycxx::detail::ranges_algo

namespace std::ranges {
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::transform_fn, ycxx::detail::par::kind::transform> transform{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::replace_fn, ycxx::detail::par::kind::replace> replace{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::replace_if_fn, ycxx::detail::par::kind::replace_if> replace_if{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::replace_copy_fn, ycxx::detail::par::kind::replace_copy> replace_copy{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::replace_copy_if_fn, ycxx::detail::par::kind::replace_copy_if> replace_copy_if{};
inline constexpr ycxx::detail::ranges_algo::generate_fn generate{};
inline constexpr ycxx::detail::ranges_algo::generate_n_fn generate_n{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::remove_fn, ycxx::detail::par::kind::remove> remove{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::remove_if_fn, ycxx::detail::par::kind::remove_if> remove_if{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::remove_copy_fn, ycxx::detail::par::kind::remove_copy> remove_copy{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::remove_copy_if_fn, ycxx::detail::par::kind::remove_copy_if> remove_copy_if{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::unique_fn, ycxx::detail::par::kind::unique> unique{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::unique_copy_fn, ycxx::detail::par::kind::unique_copy> unique_copy{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::reverse_fn, ycxx::detail::par::kind::reverse> reverse{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::reverse_copy_fn, ycxx::detail::par::kind::reverse_copy> reverse_copy{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::rotate_fn, ycxx::detail::par::kind::rotate> rotate{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::rotate_copy_fn, ycxx::detail::par::kind::rotate_copy> rotate_copy{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::shift_left_fn, ycxx::detail::par::kind::shift_left> shift_left{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::shift_right_fn, ycxx::detail::par::kind::shift_right> shift_right{};
inline constexpr ycxx::detail::ranges_algo::sample_fn sample{};
inline constexpr ycxx::detail::ranges_algo::shuffle_fn shuffle{};
} // namespace std::ranges
