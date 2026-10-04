// libycxx core: iterator associated types, iterator_traits, ranges::iter_move / iter_swap, and the
// iterator concepts ([iterator.assoc.types], [iterator.cust], [iterator.concepts]).
#pragma once

#include <ycxx/core/concepts.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/memory_base.hpp>

namespace std {

// [std.iterator.tags]
struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};
struct contiguous_iterator_tag : public random_access_iterator_tag {};

// [incrementable.traits]
template <class>
struct incrementable_traits {};
template <class T>
  requires is_object_v<T>
struct incrementable_traits<T*> {
  using difference_type = ptrdiff_t;
};
template <class I>
struct incrementable_traits<const I> : incrementable_traits<I> {};
template <class T>
  requires requires { typename T::difference_type; }
struct incrementable_traits<T> {
  using difference_type = typename T::difference_type;
};
template <class T>
  requires(!requires { typename T::difference_type; }) &&
          requires(const T& a, const T& b) {
            { a - b } -> integral;
          }
struct incrementable_traits<T> {
  using difference_type = make_signed_t<decltype(declval<T>() - declval<T>())>;
};

template <class T>
struct iterator_traits;

} // namespace std

namespace ycxx::detail {
// Detects that iterator_traits<I> names the primary template. The marker is private, so it is
// not part of the public interface, and it is not reachable through a user specialization that
// derives from another iterator_traits (private members are not accessible via the derived class).
struct iterator_traits_access {
  template <class I>
  using marker = typename std::iterator_traits<I>::primary_marker;
};
template <class I>
concept is_primary_iterator_traits = requires { typename iterator_traits_access::marker<I>; } &&
                                     __is_same(iterator_traits_access::marker<I>, std::iterator_traits<I>);

template <class T>
using with_reference = T&;
template <class T>
concept can_reference = requires { typename with_reference<T>; };
template <class T>
concept dereferenceable = requires(T& t) {
  { *t } -> can_reference;
};

template <class T>
struct cond_value_type {};
template <class T>
  requires std::is_object_v<T>
struct cond_value_type<T> {
  using value_type = std::remove_cv_t<T>;
};
template <class T>
concept has_member_value_type = requires { typename T::value_type; };
template <class T>
concept has_member_element_type = requires { typename T::element_type; };
} // namespace ycxx::detail

namespace std {

template <class I>
using iter_difference_t =
    typename conditional_t<ycxx::detail::is_primary_iterator_traits<remove_cvref_t<I>>,
                           incrementable_traits<remove_cvref_t<I>>, iterator_traits<remove_cvref_t<I>>>::difference_type;

// [readable.traits]
template <class>
struct indirectly_readable_traits {};
template <class T>
struct indirectly_readable_traits<T*> : ycxx::detail::cond_value_type<T> {};
template <class I>
  requires is_array_v<I>
struct indirectly_readable_traits<I> {
  using value_type = remove_cv_t<remove_extent_t<I>>;
};
template <class I>
struct indirectly_readable_traits<const I> : indirectly_readable_traits<I> {};
template <ycxx::detail::has_member_value_type T>
struct indirectly_readable_traits<T> : ycxx::detail::cond_value_type<typename T::value_type> {};
template <ycxx::detail::has_member_element_type T>
struct indirectly_readable_traits<T> : ycxx::detail::cond_value_type<typename T::element_type> {};
template <class T>
  requires ycxx::detail::has_member_value_type<T> && ycxx::detail::has_member_element_type<T>
struct indirectly_readable_traits<T> {};
template <class T>
  requires ycxx::detail::has_member_value_type<T> && ycxx::detail::has_member_element_type<T> &&
           same_as<remove_cv_t<typename T::element_type>, remove_cv_t<typename T::value_type>>
struct indirectly_readable_traits<T> : ycxx::detail::cond_value_type<typename T::value_type> {};

template <class I>
using iter_value_t =
    typename conditional_t<ycxx::detail::is_primary_iterator_traits<remove_cvref_t<I>>,
                           indirectly_readable_traits<remove_cvref_t<I>>, iterator_traits<remove_cvref_t<I>>>::value_type;

template <ycxx::detail::dereferenceable T>
using iter_reference_t = decltype(*declval<T&>());

} // namespace std

// ---------------------------------------------------------------------------------------------
// [iterator.traits]
// ---------------------------------------------------------------------------------------------
namespace ycxx::detail {

template <class I>
concept cpp17_iterator = requires(I i) {
  { *i } -> can_reference;
  { ++i } -> std::same_as<I&>;
  { *i++ } -> can_reference;
} && std::copyable<I>;

template <class I>
concept cpp17_input_iterator =
    cpp17_iterator<I> && std::equality_comparable<I> && requires(I i) {
      typename std::incrementable_traits<I>::difference_type;
      typename std::indirectly_readable_traits<I>::value_type;
      typename std::common_reference_t<std::iter_reference_t<I>&&,
                                       typename std::indirectly_readable_traits<I>::value_type&>;
      typename std::common_reference_t<decltype(*i++)&&, typename std::indirectly_readable_traits<I>::value_type&>;
      requires std::signed_integral<typename std::incrementable_traits<I>::difference_type>;
    };

template <class I>
concept cpp17_forward_iterator =
    cpp17_input_iterator<I> && std::constructible_from<I> && std::is_reference_v<std::iter_reference_t<I>> &&
    std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>, typename std::indirectly_readable_traits<I>::value_type> &&
    requires(I i) {
      { i++ } -> std::convertible_to<const I&>;
      { *i++ } -> std::same_as<std::iter_reference_t<I>>;
    };

template <class I>
concept cpp17_bidirectional_iterator = cpp17_forward_iterator<I> && requires(I i) {
  { --i } -> std::same_as<I&>;
  { i-- } -> std::convertible_to<const I&>;
  { *i-- } -> std::same_as<std::iter_reference_t<I>>;
};

template <class I>
concept cpp17_random_access_iterator =
    cpp17_bidirectional_iterator<I> && std::totally_ordered<I> &&
    requires(I i, typename std::incrementable_traits<I>::difference_type n) {
      { i += n } -> std::same_as<I&>;
      { i -= n } -> std::same_as<I&>;
      { i + n } -> std::same_as<I>;
      { n + i } -> std::same_as<I>;
      { i - n } -> std::same_as<I>;
      { i - i } -> std::same_as<decltype(n)>;
      { i[n] } -> std::convertible_to<std::iter_reference_t<I>>;
    };

template <class I>
concept has_all_iterator_members = requires {
  typename I::difference_type;
  typename I::value_type;
  typename I::reference;
  typename I::iterator_category;
};

// The members of iterator_traits<I>, computed per [iterator.traits]/3.
template <class I>
struct iterator_traits_impl {}; // not an iterator

// 3.1: I defines all four member types
template <class I>
  requires has_all_iterator_members<I>
struct iterator_traits_impl<I> {
  using iterator_category = typename I::iterator_category;
  using value_type = typename I::value_type;
  using difference_type = typename I::difference_type;
  using pointer = decltype([] {
    if constexpr (requires { typename I::pointer; })
      return std::type_identity<typename I::pointer>{};
    else
      return std::type_identity<void>{};
  }())::type;
  using reference = typename I::reference;
};

template <class I>
consteval auto cpp17_input_pointer() {
  if constexpr (requires { typename I::pointer; })
    return std::type_identity<typename I::pointer>{};
  else if constexpr (requires(I& i) { i.operator->(); })
    return std::type_identity<decltype(std::declval<I&>().operator->())>{};
  else
    return std::type_identity<void>{};
}
template <class I>
consteval auto cpp17_input_reference() {
  if constexpr (requires { typename I::reference; })
    return std::type_identity<typename I::reference>{};
  else
    return std::type_identity<std::iter_reference_t<I>>{};
}
template <class I>
consteval auto cpp17_input_category() {
  if constexpr (requires { typename I::iterator_category; })
    return std::type_identity<typename I::iterator_category>{};
  else if constexpr (cpp17_random_access_iterator<I>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else if constexpr (cpp17_bidirectional_iterator<I>)
    return std::type_identity<std::bidirectional_iterator_tag>{};
  else if constexpr (cpp17_forward_iterator<I>)
    return std::type_identity<std::forward_iterator_tag>{};
  else
    return std::type_identity<std::input_iterator_tag>{};
}

// 3.2: I models cpp17-input-iterator
template <class I>
  requires(!has_all_iterator_members<I>) && cpp17_input_iterator<I>
struct iterator_traits_impl<I> {
  using iterator_category = typename decltype(cpp17_input_category<I>())::type;
  using value_type = typename std::indirectly_readable_traits<I>::value_type;
  using difference_type = typename std::incrementable_traits<I>::difference_type;
  using pointer = typename decltype(cpp17_input_pointer<I>())::type;
  using reference = typename decltype(cpp17_input_reference<I>())::type;
};

// 3.3: I models cpp17-iterator
template <class I>
  requires(!has_all_iterator_members<I>) && (!cpp17_input_iterator<I>) && cpp17_iterator<I>
struct iterator_traits_impl<I> {
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = decltype([] {
    if constexpr (requires { typename std::incrementable_traits<I>::difference_type; })
      return std::type_identity<typename std::incrementable_traits<I>::difference_type>{};
    else
      return std::type_identity<void>{};
  }())::type;
  using pointer = void;
  using reference = void;
};

} // namespace ycxx::detail

namespace std {

template <class I>
struct iterator_traits : ycxx::detail::iterator_traits_impl<I> {
private:
  friend struct ycxx::detail::iterator_traits_access;
  using primary_marker = iterator_traits; // marks the primary template ([iterator.traits]/4)
};

template <class T>
  requires is_object_v<T>
struct iterator_traits<T*> {
  using iterator_concept = contiguous_iterator_tag;
  using iterator_category = random_access_iterator_tag;
  using value_type = remove_cv_t<T>;
  using difference_type = ptrdiff_t;
  using pointer = T*;
  using reference = T&;
};

} // namespace std

// ---------------------------------------------------------------------------------------------
// [iterator.cust.move], [iterator.cust.swap]
// ---------------------------------------------------------------------------------------------
namespace ycxx::detail::iter_move_cpo {

void iter_move() = delete;

template <class T>
concept adl_iter_move = (std::is_class_v<std::remove_cvref_t<T>> || std::is_union_v<std::remove_cvref_t<T>> ||
                         std::is_enum_v<std::remove_cvref_t<T>>) &&
                        requires(T&& t) { iter_move(static_cast<T&&>(t)); };

template <class T>
consteval bool iter_move_noexcept() {
  if constexpr (adl_iter_move<T>)
    return noexcept(iter_move(std::declval<T>()));
  else
    return noexcept(*std::declval<T>());
}

// The result type, spelled out rather than deduced: deducing it would instantiate the
// iterator's operator* (eagerly, on Clang, as it is constexpr) whenever a concept merely checks
// iter_move, e.g. for a list<T> iterator while T is still incomplete.
template <class T>
struct result {
  using type = decltype(*std::declval<T>());
};
template <class T>
  requires(!adl_iter_move<T>) && std::is_lvalue_reference_v<decltype(*std::declval<T>())>
struct result<T> {
  using type = std::remove_reference_t<decltype(*std::declval<T>())>&&;
};
template <class T>
  requires adl_iter_move<T>
struct result<T> {
  using type = decltype(iter_move(std::declval<T>()));
};

struct fn {
  template <class T>
    requires adl_iter_move<T> || requires(T&& t) { *static_cast<T&&>(t); }
  [[nodiscard]] constexpr typename result<T>::type operator()(T&& t) const noexcept(iter_move_noexcept<T>()) {
    if constexpr (adl_iter_move<T>)
      return iter_move(static_cast<T&&>(t));
    else if constexpr (std::is_lvalue_reference_v<decltype(*static_cast<T&&>(t))>)
      return static_cast<std::remove_reference_t<decltype(*static_cast<T&&>(t))>&&>(*static_cast<T&&>(t));
    else
      return *static_cast<T&&>(t);
  }
};

} // namespace ycxx::detail::iter_move_cpo

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::iter_move_cpo::fn iter_move{};
}
} // namespace std::ranges

namespace std {

template <ycxx::detail::dereferenceable T>
  requires requires(T& t) {
    { ranges::iter_move(t) } -> ycxx::detail::can_reference;
  }
using iter_rvalue_reference_t = decltype(ranges::iter_move(declval<T&>()));

// [iterator.concept.readable]
} // namespace std

namespace ycxx::detail {
template <class In>
concept indirectly_readable_impl =
    requires(const In in) {
      typename std::iter_value_t<In>;
      typename std::iter_reference_t<In>;
      typename std::iter_rvalue_reference_t<In>;
      { *in } -> std::same_as<std::iter_reference_t<In>>;
      { std::ranges::iter_move(in) } -> std::same_as<std::iter_rvalue_reference_t<In>>;
    } && std::common_reference_with<std::iter_reference_t<In>&&, std::iter_value_t<In>&> &&
    std::common_reference_with<std::iter_reference_t<In>&&, std::iter_rvalue_reference_t<In>&&> &&
    std::common_reference_with<std::iter_rvalue_reference_t<In>&&, const std::iter_value_t<In>&>;
} // namespace ycxx::detail

namespace std {

template <class In>
concept indirectly_readable = ycxx::detail::indirectly_readable_impl<remove_cvref_t<In>>;

template <indirectly_readable T>
using iter_common_reference_t = common_reference_t<iter_reference_t<T>, iter_value_t<T>&>;

// [iterator.concept.writable]
template <class Out, class T>
concept indirectly_writable = requires(Out&& o, T&& t) {
  *o = static_cast<T&&>(t);
  *static_cast<Out&&>(o) = static_cast<T&&>(t);
  const_cast<const iter_reference_t<Out>&&>(*o) = static_cast<T&&>(t);
  const_cast<const iter_reference_t<Out>&&>(*static_cast<Out&&>(o)) = static_cast<T&&>(t);
};

} // namespace std

namespace ycxx::detail {
// is-integer-like / is-signed-integer-like: libycxx's only integer-class type is int128 where
// the compiler provides it (treated as an integer type by the language anyway).
template <class T>
concept integer_like = std::integral<T> && !__is_same(__remove_cv(T), bool);
template <class T>
concept signed_integer_like = integer_like<T> && std::signed_integral<T>;
} // namespace ycxx::detail

namespace std {

// [iterator.concept.winc]
template <class I>
concept weakly_incrementable = movable<I> && requires(I i) {
  typename iter_difference_t<I>;
  requires ycxx::detail::signed_integer_like<iter_difference_t<I>>;
  { ++i } -> same_as<I&>;
  i++;
};

template <class I>
concept incrementable = regular<I> && weakly_incrementable<I> && requires(I i) {
  { i++ } -> same_as<I>;
};

template <class I>
concept input_or_output_iterator = requires(I i) {
  { *i } -> ycxx::detail::can_reference;
} && weakly_incrementable<I>;

template <class S, class I>
concept sentinel_for = semiregular<S> && input_or_output_iterator<I> && ycxx::detail::weakly_equality_comparable_with<S, I>;

template <class S, class I>
constexpr bool disable_sized_sentinel_for = false;

template <class S, class I>
concept sized_sentinel_for =
    sentinel_for<S, I> && !disable_sized_sentinel_for<remove_cv_t<S>, remove_cv_t<I>> &&
    requires(const I& i, const S& s) {
      { s - i } -> same_as<iter_difference_t<I>>;
      { i - s } -> same_as<iter_difference_t<I>>;
    };

} // namespace std

namespace ycxx::detail {
// ITER_TRAITS(I) / ITER_CONCEPT(I)
template <class I>
using iter_traits = std::conditional_t<is_primary_iterator_traits<I>, I, std::iterator_traits<I>>;

template <class I>
consteval auto iter_concept_impl() {
  using T = iter_traits<I>;
  if constexpr (requires { typename T::iterator_concept; })
    return std::type_identity<typename T::iterator_concept>{};
  else if constexpr (requires { typename T::iterator_category; })
    return std::type_identity<typename T::iterator_category>{};
  else if constexpr (is_primary_iterator_traits<I>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else
    return std::type_identity<void>{}; // no ITER_CONCEPT
}
template <class I>
using iter_concept = typename decltype(iter_concept_impl<I>())::type;
} // namespace ycxx::detail

namespace std {

template <class I>
concept input_iterator = input_or_output_iterator<I> && indirectly_readable<I> &&
                         requires { typename ycxx::detail::iter_concept<I>; } &&
                         derived_from<ycxx::detail::iter_concept<I>, input_iterator_tag>;

template <class I, class T>
concept output_iterator = input_or_output_iterator<I> && indirectly_writable<I, T> && requires(I i, T&& t) {
  *i++ = static_cast<T&&>(t);
};

template <class I>
concept forward_iterator = input_iterator<I> && derived_from<ycxx::detail::iter_concept<I>, forward_iterator_tag> &&
                           incrementable<I> && sentinel_for<I, I>;

template <class I>
concept bidirectional_iterator = forward_iterator<I> &&
                                 derived_from<ycxx::detail::iter_concept<I>, bidirectional_iterator_tag> &&
                                 requires(I i) {
                                   { --i } -> same_as<I&>;
                                   { i-- } -> same_as<I>;
                                 };

template <class I>
concept random_access_iterator =
    bidirectional_iterator<I> && derived_from<ycxx::detail::iter_concept<I>, random_access_iterator_tag> &&
    totally_ordered<I> && sized_sentinel_for<I, I> && requires(I i, const I j, const iter_difference_t<I> n) {
      { i += n } -> same_as<I&>;
      { j + n } -> same_as<I>;
      { n + j } -> same_as<I>;
      { i -= n } -> same_as<I&>;
      { j - n } -> same_as<I>;
      { j[n] } -> same_as<iter_reference_t<I>>;
    };

template <class I>
concept contiguous_iterator =
    random_access_iterator<I> && derived_from<ycxx::detail::iter_concept<I>, contiguous_iterator_tag> &&
    is_lvalue_reference_v<iter_reference_t<I>> && same_as<iter_value_t<I>, remove_cvref_t<iter_reference_t<I>>> &&
    requires(const I& i) {
      { std::to_address(i) } -> same_as<add_pointer_t<iter_reference_t<I>>>;
    };

} // namespace std

namespace ycxx::adl_free {
// projected-impl ([projected]): projected<I, Proj> is the nested class `type`. A nested class
// is not a template specialization, so ADL on it neither instantiates I or Proj nor sees
// ycxx::detail. It records I and Proj (reserved member names) so that indirect-value-t can see
// through it ([indirectcallable.traits], P2609).
template <class I, class Proj>
struct projected_impl {
  struct type {
    using value_type = std::remove_cvref_t<std::invoke_result_t<Proj&, std::iter_reference_t<I>>>;
    using __projected_iter = I;
    using __projected_proj = Proj;
    std::invoke_result_t<Proj&, std::iter_reference_t<I>> operator*() const; // not defined
  };
};
template <class I, class Proj>
  requires std::weakly_incrementable<I>
struct projected_impl<I, Proj> {
  struct type {
    using value_type = std::remove_cvref_t<std::invoke_result_t<Proj&, std::iter_reference_t<I>>>;
    using difference_type = std::iter_difference_t<I>;
    using __projected_iter = I;
    using __projected_proj = Proj;
    std::invoke_result_t<Proj&, std::iter_reference_t<I>> operator*() const; // not defined
  };
};
} // namespace ycxx::adl_free

namespace ycxx::detail {
template <class T>
concept is_projected = requires {
  typename T::__projected_iter;
  typename T::__projected_proj;
} && std::is_same_v<T, typename adl_free::projected_impl<typename T::__projected_iter, typename T::__projected_proj>::type>;

template <class T>
struct indirect_value {
  using type = std::iter_value_t<T>&;
};
template <is_projected P>
struct indirect_value<P> {
  using type = std::invoke_result_t<typename P::__projected_proj&, typename indirect_value<typename P::__projected_iter>::type>;
};
template <class T>
using indirect_value_t = typename indirect_value<T>::type;
} // namespace ycxx::detail

namespace std {

// [indirectcallable.indirectinvocable]
template <class F, class I>
concept indirectly_unary_invocable =
    indirectly_readable<I> && copy_constructible<F> && invocable<F&, ycxx::detail::indirect_value_t<I>> &&
    invocable<F&, iter_reference_t<I>> &&
    common_reference_with<invoke_result_t<F&, ycxx::detail::indirect_value_t<I>>, invoke_result_t<F&, iter_reference_t<I>>>;

template <class F, class I>
concept indirectly_regular_unary_invocable =
    indirectly_readable<I> && copy_constructible<F> && regular_invocable<F&, ycxx::detail::indirect_value_t<I>> &&
    regular_invocable<F&, iter_reference_t<I>> &&
    common_reference_with<invoke_result_t<F&, ycxx::detail::indirect_value_t<I>>, invoke_result_t<F&, iter_reference_t<I>>>;

template <class F, class I>
concept indirect_unary_predicate = indirectly_readable<I> && copy_constructible<F> &&
                                   predicate<F&, ycxx::detail::indirect_value_t<I>> && predicate<F&, iter_reference_t<I>>;

template <class F, class I1, class I2>
concept indirect_binary_predicate =
    indirectly_readable<I1> && indirectly_readable<I2> && copy_constructible<F> &&
    predicate<F&, ycxx::detail::indirect_value_t<I1>, ycxx::detail::indirect_value_t<I2>> &&
    predicate<F&, ycxx::detail::indirect_value_t<I1>, iter_reference_t<I2>> &&
    predicate<F&, iter_reference_t<I1>, ycxx::detail::indirect_value_t<I2>> &&
    predicate<F&, iter_reference_t<I1>, iter_reference_t<I2>>;

template <class F, class I1, class I2 = I1>
concept indirect_equivalence_relation =
    indirectly_readable<I1> && indirectly_readable<I2> && copy_constructible<F> &&
    equivalence_relation<F&, ycxx::detail::indirect_value_t<I1>, ycxx::detail::indirect_value_t<I2>> &&
    equivalence_relation<F&, ycxx::detail::indirect_value_t<I1>, iter_reference_t<I2>> &&
    equivalence_relation<F&, iter_reference_t<I1>, ycxx::detail::indirect_value_t<I2>> &&
    equivalence_relation<F&, iter_reference_t<I1>, iter_reference_t<I2>>;

template <class F, class I1, class I2 = I1>
concept indirect_strict_weak_order =
    indirectly_readable<I1> && indirectly_readable<I2> && copy_constructible<F> &&
    strict_weak_order<F&, ycxx::detail::indirect_value_t<I1>, ycxx::detail::indirect_value_t<I2>> &&
    strict_weak_order<F&, ycxx::detail::indirect_value_t<I1>, iter_reference_t<I2>> &&
    strict_weak_order<F&, iter_reference_t<I1>, ycxx::detail::indirect_value_t<I2>> &&
    strict_weak_order<F&, iter_reference_t<I1>, iter_reference_t<I2>>;

template <class F, class... Is>
  requires(indirectly_readable<Is> && ...) && invocable<F, iter_reference_t<Is>...>
using indirect_result_t = invoke_result_t<F, iter_reference_t<Is>...>;

// [projected]
template <indirectly_readable I, indirectly_regular_unary_invocable<I> Proj>
using projected = typename ycxx::adl_free::projected_impl<I, Proj>::type;

template <indirectly_readable I, indirectly_regular_unary_invocable<I> Proj>
using projected_value_t = remove_cvref_t<invoke_result_t<Proj&, iter_value_t<I>&>>;

// [alg.req]
template <class In, class Out>
concept indirectly_movable = indirectly_readable<In> && indirectly_writable<Out, iter_rvalue_reference_t<In>>;
template <class In, class Out>
concept indirectly_movable_storable =
    indirectly_movable<In, Out> && indirectly_writable<Out, iter_value_t<In>> && movable<iter_value_t<In>> &&
    constructible_from<iter_value_t<In>, iter_rvalue_reference_t<In>> &&
    assignable_from<iter_value_t<In>&, iter_rvalue_reference_t<In>>;
template <class In, class Out>
concept indirectly_copyable = indirectly_readable<In> && indirectly_writable<Out, iter_reference_t<In>>;
template <class In, class Out>
concept indirectly_copyable_storable =
    indirectly_copyable<In, Out> && indirectly_writable<Out, iter_value_t<In>&> &&
    indirectly_writable<Out, const iter_value_t<In>&> && indirectly_writable<Out, iter_value_t<In>&&> &&
    indirectly_writable<Out, const iter_value_t<In>&&> && copyable<iter_value_t<In>> &&
    constructible_from<iter_value_t<In>, iter_reference_t<In>> &&
    assignable_from<iter_value_t<In>&, iter_reference_t<In>>;

} // namespace std

namespace ycxx::detail {
// iter-exchange-move ([iterator.cust.swap]). Outside the CPO's namespace, which must declare
// nothing ADL could find on the iter_swap object.
template <class X, class Y>
constexpr std::iter_value_t<X> iter_exchange_move(X&& x, Y&& y) noexcept(
    noexcept(std::iter_value_t<X>(std::ranges::iter_move(x))) && noexcept(*x = std::ranges::iter_move(y))) {
  std::iter_value_t<X> old(std::ranges::iter_move(x));
  *x = std::ranges::iter_move(y);
  return old;
}
} // namespace ycxx::detail

namespace ycxx::detail::iter_swap_cpo {

template <class I1, class I2>
void iter_swap(I1, I2) = delete;

template <class T, class U>
concept adl_iter_swap =
    (std::is_class_v<std::remove_cvref_t<T>> || std::is_union_v<std::remove_cvref_t<T>> ||
     std::is_enum_v<std::remove_cvref_t<T>> || std::is_class_v<std::remove_cvref_t<U>> ||
     std::is_union_v<std::remove_cvref_t<U>> || std::is_enum_v<std::remove_cvref_t<U>>) &&
    requires(T&& t, U&& u) { iter_swap(static_cast<T&&>(t), static_cast<U&&>(u)); };

template <class T, class U>
consteval bool iter_swap_noexcept() {
  if constexpr (adl_iter_swap<T, U>)
    return noexcept((void)iter_swap(std::declval<T>(), std::declval<U>()));
  else if constexpr (std::indirectly_readable<T> && std::indirectly_readable<U> &&
                     std::swappable_with<std::iter_reference_t<T>, std::iter_reference_t<U>>)
    return noexcept(std::ranges::swap(*std::declval<T>(), *std::declval<U>()));
  else
    return noexcept((void)(*std::declval<T>() = ::ycxx::detail::iter_exchange_move(std::declval<U>(), std::declval<T>())));
}

struct fn {
  template <class T, class U>
    requires adl_iter_swap<T, U> ||
             (std::indirectly_readable<T> && std::indirectly_readable<U> &&
              std::swappable_with<std::iter_reference_t<T>, std::iter_reference_t<U>>) ||
             (std::indirectly_movable_storable<T, U> && std::indirectly_movable_storable<U, T>)
  constexpr void operator()(T&& t, U&& u) const noexcept(iter_swap_noexcept<T, U>()) {
    if constexpr (adl_iter_swap<T, U>)
      (void)iter_swap(static_cast<T&&>(t), static_cast<U&&>(u));
    else if constexpr (std::indirectly_readable<T> && std::indirectly_readable<U> &&
                       std::swappable_with<std::iter_reference_t<T>, std::iter_reference_t<U>>)
      std::ranges::swap(*t, *u);
    else
      (void)(*t = ::ycxx::detail::iter_exchange_move(u, t));
  }
};

} // namespace ycxx::detail::iter_swap_cpo

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::iter_swap_cpo::fn iter_swap{};
}
} // namespace std::ranges

namespace std {

template <class I1, class I2 = I1>
concept indirectly_swappable = indirectly_readable<I1> && indirectly_readable<I2> && requires(const I1 i1, const I2 i2) {
  ranges::iter_swap(i1, i1);
  ranges::iter_swap(i2, i2);
  ranges::iter_swap(i1, i2);
  ranges::iter_swap(i2, i1);
};

template <class I1, class I2, class R, class P1 = identity, class P2 = identity>
concept indirectly_comparable = indirect_binary_predicate<R, projected<I1, P1>, projected<I2, P2>>;

template <class I>
concept permutable = forward_iterator<I> && indirectly_movable_storable<I, I> && indirectly_swappable<I, I>;

template <class I1, class I2, class Out, class R = ranges::less, class P1 = identity, class P2 = identity>
concept mergeable = input_iterator<I1> && input_iterator<I2> && weakly_incrementable<Out> &&
                    indirectly_copyable<I1, Out> && indirectly_copyable<I2, Out> &&
                    indirect_strict_weak_order<R, projected<I1, P1>, projected<I2, P2>>;

template <class I, class R = ranges::less, class P = identity>
concept sortable = permutable<I> && indirect_strict_weak_order<R, projected<I, P>>;

} // namespace std
