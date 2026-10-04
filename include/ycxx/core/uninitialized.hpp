// libycxx core: the specialized <memory> algorithms ([specialized.algorithms]):
// uninitialized_{default,value}_construct[_n], uninitialized_{copy,move,fill}[_n] and their
// std::ranges forms, and ranges::destroy / destroy_n. construct_at and destroy_at live in
// memory_base.hpp.
//
// If constructing an element throws, the elements already constructed are destroyed before
// the exception propagates ([specialized.algorithms.general]/2). A guard object does this from
// its destructor, so the same code serves -fno-exceptions builds.
// The execution-policy overloads are not provided (no <execution>).
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/range_dangling.hpp>
#include <ycxx/core/algorithm_results.hpp>

namespace ycxx::detail {

// [special.mem.concepts]
template <class I>
concept nothrow_input_iterator = std::input_iterator<I> && std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
                                 std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>, std::iter_value_t<I>>;
template <class S, class I>
concept nothrow_sentinel_for = std::sentinel_for<S, I>;
template <class R>
concept nothrow_input_range = std::ranges::range<R> && nothrow_input_iterator<std::ranges::iterator_t<R>> &&
                              nothrow_sentinel_for<std::ranges::sentinel_t<R>, std::ranges::iterator_t<R>>;
template <class I>
concept nothrow_forward_iterator = nothrow_input_iterator<I> && std::forward_iterator<I> && nothrow_sentinel_for<I, I>;
template <class R>
concept nothrow_forward_range = nothrow_input_range<R> && nothrow_forward_iterator<std::ranges::iterator_t<R>>;

// voidify ([specialized.algorithms.general]/4).
template <class T>
constexpr void* voidify(T& obj) noexcept {
  return __builtin_addressof(obj);
}

// deref-move ([specialized.algorithms.general]/4).
template <class I>
constexpr decltype(auto) deref_move(I& it) {
  if constexpr (std::is_lvalue_reference_v<decltype(*it)>)
    return static_cast<std::remove_reference_t<decltype(*it)>&&>(*it);
  else
    return *it;
}

// Destroys [first, *cur) on scope exit unless released: the rollback of a partially
// constructed range.
template <class I>
struct uninit_guard {
  I first;
  I* cur;
  constexpr ~uninit_guard() {
    if (cur)
      for (; first != *cur; ++first)
        std::destroy_at(__builtin_addressof(*first));
  }
  constexpr void release() noexcept { cur = nullptr; }
};
template <class I>
uninit_guard(I, I*) -> uninit_guard<I>;

} // namespace ycxx::detail

namespace std {

// [uninitialized.construct.default]
template <class NoThrowForwardIterator>
constexpr void uninitialized_default_construct(NoThrowForwardIterator first, NoThrowForwardIterator last) {
  using T = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = first;
  ycxx::detail::uninit_guard g{first, &cur};
  for (; cur != last; ++cur)
    ::new (::ycxx::detail::voidify(*cur)) T;
  g.release();
}
template <class NoThrowForwardIterator, class Size>
constexpr NoThrowForwardIterator uninitialized_default_construct_n(NoThrowForwardIterator first, Size n) {
  using T = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = first;
  ycxx::detail::uninit_guard g{first, &cur};
  for (; n > 0; (void)++cur, --n)
    ::new (::ycxx::detail::voidify(*cur)) T;
  g.release();
  return cur;
}

// [uninitialized.construct.value]
template <class NoThrowForwardIterator>
constexpr void uninitialized_value_construct(NoThrowForwardIterator first, NoThrowForwardIterator last) {
  using T = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = first;
  ycxx::detail::uninit_guard g{first, &cur};
  for (; cur != last; ++cur)
    ::new (::ycxx::detail::voidify(*cur)) T();
  g.release();
}
template <class NoThrowForwardIterator, class Size>
constexpr NoThrowForwardIterator uninitialized_value_construct_n(NoThrowForwardIterator first, Size n) {
  using T = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = first;
  ycxx::detail::uninit_guard g{first, &cur};
  for (; n > 0; (void)++cur, --n)
    ::new (::ycxx::detail::voidify(*cur)) T();
  g.release();
  return cur;
}

// [uninitialized.copy]
template <class InputIterator, class NoThrowForwardIterator>
constexpr NoThrowForwardIterator uninitialized_copy(InputIterator first, InputIterator last,
                                                    NoThrowForwardIterator result) {
  using T = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = result;
  ycxx::detail::uninit_guard g{result, &cur};
  for (; first != last; ++cur, (void)++first)
    ::new (::ycxx::detail::voidify(*cur)) T(*first);
  g.release();
  return cur;
}
template <class InputIterator, class Size, class NoThrowForwardIterator>
constexpr NoThrowForwardIterator uninitialized_copy_n(InputIterator first, Size n, NoThrowForwardIterator result) {
  using T = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = result;
  ycxx::detail::uninit_guard g{result, &cur};
  for (; n > 0; ++cur, (void)++first, --n)
    ::new (::ycxx::detail::voidify(*cur)) T(*first);
  g.release();
  return cur;
}

// [uninitialized.move]
template <class InputIterator, class NoThrowForwardIterator>
constexpr NoThrowForwardIterator uninitialized_move(InputIterator first, InputIterator last,
                                                    NoThrowForwardIterator result) {
  using T = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = result;
  ycxx::detail::uninit_guard g{result, &cur};
  for (; first != last; (void)++cur, ++first)
    ::new (::ycxx::detail::voidify(*cur)) T(::ycxx::detail::deref_move(first));
  g.release();
  return cur;
}
template <class InputIterator, class Size, class NoThrowForwardIterator>
constexpr pair<InputIterator, NoThrowForwardIterator> uninitialized_move_n(InputIterator first, Size n,
                                                                           NoThrowForwardIterator result) {
  using T = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = result;
  ycxx::detail::uninit_guard g{result, &cur};
  for (; n > 0; ++cur, (void)++first, --n)
    ::new (::ycxx::detail::voidify(*cur)) T(::ycxx::detail::deref_move(first));
  g.release();
  return {first, cur};
}

// [uninitialized.fill]
template <class NoThrowForwardIterator, class T = typename iterator_traits<NoThrowForwardIterator>::value_type>
constexpr void uninitialized_fill(NoThrowForwardIterator first, NoThrowForwardIterator last, const T& x) {
  using V = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = first;
  ycxx::detail::uninit_guard g{first, &cur};
  for (; cur != last; ++cur)
    ::new (::ycxx::detail::voidify(*cur)) V(x);
  g.release();
}
template <class NoThrowForwardIterator, class Size,
          class T = typename iterator_traits<NoThrowForwardIterator>::value_type>
constexpr NoThrowForwardIterator uninitialized_fill_n(NoThrowForwardIterator first, Size n, const T& x) {
  using V = typename iterator_traits<NoThrowForwardIterator>::value_type;
  NoThrowForwardIterator cur = first;
  ycxx::detail::uninit_guard g{first, &cur};
  for (; n--; ++cur)
    ::new (::ycxx::detail::voidify(*cur)) V(x);
  g.release();
  return cur;
}

} // namespace std

// ---------------------------------------------------------------------------------------------
// std::ranges forms
// ---------------------------------------------------------------------------------------------
namespace std::ranges {
template <class I, class O>
using uninitialized_copy_result = in_out_result<I, O>;
template <class I, class O>
using uninitialized_copy_n_result = in_out_result<I, O>;
template <class I, class O>
using uninitialized_move_result = in_out_result<I, O>;
template <class I, class O>
using uninitialized_move_n_result = in_out_result<I, O>;
} // namespace std::ranges

namespace ycxx::detail::uninit_fn {

using std::iter_difference_t;
using std::iter_value_t;
using std::ranges::borrowed_iterator_t;
using std::ranges::range_value_t;

// The element type the range forms construct: remove_reference_t<iter_reference_t<I>>.
template <class I>
using elem_t = std::remove_reference_t<std::iter_reference_t<I>>;

struct default_construct {
  template <nothrow_forward_iterator I, nothrow_sentinel_for<I> S>
    requires std::default_initializable<iter_value_t<I>>
  static constexpr I operator()(I first, S last) {
    I cur = first;
    uninit_guard g{first, &cur};
    for (; cur != last; ++cur)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<I>;
    g.release();
    return cur;
  }
  template <nothrow_forward_range R>
    requires std::default_initializable<range_value_t<R>>
  static constexpr borrowed_iterator_t<R> operator()(R&& r) {
    return operator()(std::ranges::begin(r), std::ranges::end(r));
  }
};
struct default_construct_n {
  template <nothrow_forward_iterator I>
    requires std::default_initializable<iter_value_t<I>>
  static constexpr I operator()(I first, iter_difference_t<I> n) {
    I cur = first;
    uninit_guard g{first, &cur};
    for (; n > 0; (void)++cur, --n)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<I>;
    g.release();
    return cur;
  }
};

struct value_construct {
  template <nothrow_forward_iterator I, nothrow_sentinel_for<I> S>
    requires std::default_initializable<iter_value_t<I>>
  static constexpr I operator()(I first, S last) {
    I cur = first;
    uninit_guard g{first, &cur};
    for (; cur != last; ++cur)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<I>();
    g.release();
    return cur;
  }
  template <nothrow_forward_range R>
    requires std::default_initializable<range_value_t<R>>
  static constexpr borrowed_iterator_t<R> operator()(R&& r) {
    return operator()(std::ranges::begin(r), std::ranges::end(r));
  }
};
struct value_construct_n {
  template <nothrow_forward_iterator I>
    requires std::default_initializable<iter_value_t<I>>
  static constexpr I operator()(I first, iter_difference_t<I> n) {
    I cur = first;
    uninit_guard g{first, &cur};
    for (; n > 0; (void)++cur, --n)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<I>();
    g.release();
    return cur;
  }
};

struct copy {
  template <std::input_iterator I, std::sentinel_for<I> S1, nothrow_forward_iterator O, nothrow_sentinel_for<O> S2>
    requires std::constructible_from<iter_value_t<O>, std::iter_reference_t<I>>
  static constexpr std::ranges::uninitialized_copy_result<I, O> operator()(I ifirst, S1 ilast, O ofirst, S2 olast) {
    O cur = ofirst;
    uninit_guard g{ofirst, &cur};
    for (; ifirst != ilast && cur != olast; ++cur, (void)++ifirst)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<O>(*ifirst);
    g.release();
    return {static_cast<I&&>(ifirst), cur};
  }
  template <std::ranges::input_range IR, nothrow_forward_range OR>
    requires std::constructible_from<range_value_t<OR>, std::ranges::range_reference_t<IR>>
  static constexpr std::ranges::uninitialized_copy_result<borrowed_iterator_t<IR>, borrowed_iterator_t<OR>>
  operator()(IR&& in_range, OR&& out_range) {
    auto r = operator()(std::ranges::begin(in_range), std::ranges::end(in_range), std::ranges::begin(out_range),
                        std::ranges::end(out_range));
    return {static_cast<decltype(r.in)&&>(r.in), r.out};
  }
};
struct copy_n {
  template <std::input_iterator I, nothrow_forward_iterator O, nothrow_sentinel_for<O> S>
    requires std::constructible_from<iter_value_t<O>, std::iter_reference_t<I>>
  static constexpr std::ranges::uninitialized_copy_n_result<I, O> operator()(I ifirst, iter_difference_t<I> n,
                                                                             O ofirst, S olast) {
    O cur = ofirst;
    uninit_guard g{ofirst, &cur};
    for (; n > 0 && cur != olast; ++cur, (void)++ifirst, --n)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<O>(*ifirst);
    g.release();
    return {static_cast<I&&>(ifirst), cur};
  }
};

struct move {
  template <std::input_iterator I, std::sentinel_for<I> S1, nothrow_forward_iterator O, nothrow_sentinel_for<O> S2>
    requires std::constructible_from<iter_value_t<O>, std::iter_rvalue_reference_t<I>>
  static constexpr std::ranges::uninitialized_move_result<I, O> operator()(I ifirst, S1 ilast, O ofirst, S2 olast) {
    O cur = ofirst;
    uninit_guard g{ofirst, &cur};
    for (; ifirst != ilast && cur != olast; ++cur, (void)++ifirst)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<O>(std::ranges::iter_move(ifirst));
    g.release();
    return {static_cast<I&&>(ifirst), cur};
  }
  template <std::ranges::input_range IR, nothrow_forward_range OR>
    requires std::constructible_from<range_value_t<OR>, std::ranges::range_rvalue_reference_t<IR>>
  static constexpr std::ranges::uninitialized_move_result<borrowed_iterator_t<IR>, borrowed_iterator_t<OR>>
  operator()(IR&& in_range, OR&& out_range) {
    auto r = operator()(std::ranges::begin(in_range), std::ranges::end(in_range), std::ranges::begin(out_range),
                        std::ranges::end(out_range));
    return {static_cast<decltype(r.in)&&>(r.in), r.out};
  }
};
struct move_n {
  template <std::input_iterator I, nothrow_forward_iterator O, nothrow_sentinel_for<O> S>
    requires std::constructible_from<iter_value_t<O>, std::iter_rvalue_reference_t<I>>
  static constexpr std::ranges::uninitialized_move_n_result<I, O> operator()(I ifirst, iter_difference_t<I> n,
                                                                             O ofirst, S olast) {
    O cur = ofirst;
    uninit_guard g{ofirst, &cur};
    for (; n > 0 && cur != olast; ++cur, (void)++ifirst, --n)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<O>(std::ranges::iter_move(ifirst));
    g.release();
    return {static_cast<I&&>(ifirst), cur};
  }
};

struct fill {
  template <nothrow_forward_iterator I, nothrow_sentinel_for<I> S, class T = iter_value_t<I>>
    requires std::constructible_from<iter_value_t<I>, const T&>
  static constexpr I operator()(I first, S last, const T& x) {
    I cur = first;
    uninit_guard g{first, &cur};
    for (; cur != last; ++cur)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<I>(x);
    g.release();
    return cur;
  }
  template <nothrow_forward_range R, class T = range_value_t<R>>
    requires std::constructible_from<range_value_t<R>, const T&>
  static constexpr borrowed_iterator_t<R> operator()(R&& r, const T& x) {
    return operator()(std::ranges::begin(r), std::ranges::end(r), x);
  }
};
struct fill_n {
  template <nothrow_forward_iterator I, class T = iter_value_t<I>>
    requires std::constructible_from<iter_value_t<I>, const T&>
  static constexpr I operator()(I first, iter_difference_t<I> n, const T& x) {
    I cur = first;
    uninit_guard g{first, &cur};
    for (; n > 0; (void)++cur, --n)
      ::new (::ycxx::detail::voidify(*cur)) elem_t<I>(x);
    g.release();
    return cur;
  }
};

// [specialized.destroy]
struct destroy {
  template <nothrow_input_iterator I, nothrow_sentinel_for<I> S>
    requires std::destructible<iter_value_t<I>>
  static constexpr I operator()(I first, S last) noexcept {
    for (; first != last; ++first)
      std::destroy_at(__builtin_addressof(*first));
    return first;
  }
  template <nothrow_input_range R>
    requires std::destructible<range_value_t<R>>
  static constexpr borrowed_iterator_t<R> operator()(R&& r) noexcept {
    return operator()(std::ranges::begin(r), std::ranges::end(r));
  }
};
struct destroy_n {
  template <nothrow_input_iterator I>
    requires std::destructible<iter_value_t<I>>
  static constexpr I operator()(I first, iter_difference_t<I> n) noexcept {
    for (; n > 0; (void)++first, --n)
      std::destroy_at(__builtin_addressof(*first));
    return first;
  }
};

} // namespace ycxx::detail::uninit_fn

namespace std::ranges {
inline constexpr ycxx::detail::uninit_fn::default_construct uninitialized_default_construct{};
inline constexpr ycxx::detail::uninit_fn::default_construct_n uninitialized_default_construct_n{};
inline constexpr ycxx::detail::uninit_fn::value_construct uninitialized_value_construct{};
inline constexpr ycxx::detail::uninit_fn::value_construct_n uninitialized_value_construct_n{};
inline constexpr ycxx::detail::uninit_fn::copy uninitialized_copy{};
inline constexpr ycxx::detail::uninit_fn::copy_n uninitialized_copy_n{};
inline constexpr ycxx::detail::uninit_fn::move uninitialized_move{};
inline constexpr ycxx::detail::uninit_fn::move_n uninitialized_move_n{};
inline constexpr ycxx::detail::uninit_fn::fill uninitialized_fill{};
inline constexpr ycxx::detail::uninit_fn::fill_n uninitialized_fill_n{};
inline constexpr ycxx::detail::uninit_fn::destroy destroy{};
inline constexpr ycxx::detail::uninit_fn::destroy_n destroy_n{};
} // namespace std::ranges
