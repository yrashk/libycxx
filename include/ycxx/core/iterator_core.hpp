// libycxx core: iterator associated types, iterator_traits, ranges::iter_move / iter_swap, and the
// iterator concepts ([iterator.assoc.types], [iterator.cust], [iterator.concepts]).
#pragma once

#include <ycxx/core/concepts.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/memory_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

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
template <class _Tp>
  requires is_object_v<_Tp>
struct incrementable_traits<_Tp*> {
  using difference_type = ptrdiff_t;
};
template <class _Ip>
struct incrementable_traits<const _Ip> : incrementable_traits<_Ip> {};
template <class _Tp>
  requires requires { typename _Tp::difference_type; }
struct incrementable_traits<_Tp> {
  using difference_type = typename _Tp::difference_type;
};
template <class _Tp>
  requires(!requires { typename _Tp::difference_type; }) &&
          requires(const _Tp& a, const _Tp& b) {
            { a - b } -> integral;
          }
struct incrementable_traits<_Tp> {
  using difference_type = make_signed_t<decltype(declval<_Tp>() - declval<_Tp>())>;
};

template <class _Tp>
struct iterator_traits;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// Detects that iterator_traits<I> names the primary template. The marker is private, so it is
// not part of the public interface, and it is not reachable through a user specialization that
// derives from another iterator_traits (private members are not accessible via the derived class).
struct __iterator_traits_access {
  template <class _Ip>
  using __marker = typename std::iterator_traits<_Ip>::__primary_marker;
};
template <class _Ip>
concept __is_primary_iterator_traits = requires { typename __iterator_traits_access::__marker<_Ip>; } &&
                                     __is_same(__iterator_traits_access::__marker<_Ip>, std::iterator_traits<_Ip>);

template <class _Tp>
using __with_reference = _Tp&;
template <class _Tp>
concept __can_reference = requires { typename __with_reference<_Tp>; };
template <class _Tp>
concept __dereferenceable = requires(_Tp& t) {
  { *t } -> __can_reference;
};

template <class _Tp>
struct __cond_value_type {};
template <class _Tp>
  requires std::is_object_v<_Tp>
struct __cond_value_type<_Tp> {
  using value_type = std::remove_cv_t<_Tp>;
};
template <class _Tp>
concept __has_member_value_type = requires { typename _Tp::value_type; };
template <class _Tp>
concept __has_member_element_type = requires { typename _Tp::element_type; };
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Ip>
using iter_difference_t =
    typename conditional_t<__ycxx::__detail::__is_primary_iterator_traits<remove_cvref_t<_Ip>>,
                           incrementable_traits<remove_cvref_t<_Ip>>, iterator_traits<remove_cvref_t<_Ip>>>::difference_type;

// [readable.traits]
template <class>
struct indirectly_readable_traits {};
template <class _Tp>
struct indirectly_readable_traits<_Tp*> : __ycxx::__detail::__cond_value_type<_Tp> {};
template <class _Ip>
  requires is_array_v<_Ip>
struct indirectly_readable_traits<_Ip> {
  using value_type = remove_cv_t<remove_extent_t<_Ip>>;
};
template <class _Ip>
struct indirectly_readable_traits<const _Ip> : indirectly_readable_traits<_Ip> {};
template <__ycxx::__detail::__has_member_value_type _Tp>
struct indirectly_readable_traits<_Tp> : __ycxx::__detail::__cond_value_type<typename _Tp::value_type> {};
template <__ycxx::__detail::__has_member_element_type _Tp>
struct indirectly_readable_traits<_Tp> : __ycxx::__detail::__cond_value_type<typename _Tp::element_type> {};
template <class _Tp>
  requires __ycxx::__detail::__has_member_value_type<_Tp> && __ycxx::__detail::__has_member_element_type<_Tp>
struct indirectly_readable_traits<_Tp> {};
template <class _Tp>
  requires __ycxx::__detail::__has_member_value_type<_Tp> && __ycxx::__detail::__has_member_element_type<_Tp> &&
           same_as<remove_cv_t<typename _Tp::element_type>, remove_cv_t<typename _Tp::value_type>>
struct indirectly_readable_traits<_Tp> : __ycxx::__detail::__cond_value_type<typename _Tp::value_type> {};

template <class _Ip>
using iter_value_t =
    typename conditional_t<__ycxx::__detail::__is_primary_iterator_traits<remove_cvref_t<_Ip>>,
                           indirectly_readable_traits<remove_cvref_t<_Ip>>, iterator_traits<remove_cvref_t<_Ip>>>::value_type;

template <__ycxx::__detail::__dereferenceable _Tp>
using iter_reference_t = decltype(*declval<_Tp&>());

} // namespace std

// ---------------------------------------------------------------------------------------------
// [iterator.traits]
// ---------------------------------------------------------------------------------------------
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Ip>
concept __cpp17_iterator = requires(_Ip i) {
  { *i } -> __can_reference;
  { ++i } -> std::same_as<_Ip&>;
  { *i++ } -> __can_reference;
} && std::copyable<_Ip>;

template <class _Ip>
concept __cpp17_input_iterator =
    __cpp17_iterator<_Ip> && std::equality_comparable<_Ip> && requires(_Ip i) {
      typename std::incrementable_traits<_Ip>::difference_type;
      typename std::indirectly_readable_traits<_Ip>::value_type;
      typename std::common_reference_t<std::iter_reference_t<_Ip>&&,
                                       typename std::indirectly_readable_traits<_Ip>::value_type&>;
      typename std::common_reference_t<decltype(*i++)&&, typename std::indirectly_readable_traits<_Ip>::value_type&>;
      requires std::signed_integral<typename std::incrementable_traits<_Ip>::difference_type>;
    };

template <class _Ip>
concept __cpp17_forward_iterator =
    __cpp17_input_iterator<_Ip> && std::constructible_from<_Ip> && std::is_reference_v<std::iter_reference_t<_Ip>> &&
    std::same_as<std::remove_cvref_t<std::iter_reference_t<_Ip>>, typename std::indirectly_readable_traits<_Ip>::value_type> &&
    requires(_Ip i) {
      { i++ } -> std::convertible_to<const _Ip&>;
      { *i++ } -> std::same_as<std::iter_reference_t<_Ip>>;
    };

template <class _Ip>
concept __cpp17_bidirectional_iterator = __cpp17_forward_iterator<_Ip> && requires(_Ip i) {
  { --i } -> std::same_as<_Ip&>;
  { i-- } -> std::convertible_to<const _Ip&>;
  { *i-- } -> std::same_as<std::iter_reference_t<_Ip>>;
};

template <class _Ip>
concept __cpp17_random_access_iterator =
    __cpp17_bidirectional_iterator<_Ip> && std::totally_ordered<_Ip> &&
    requires(_Ip i, typename std::incrementable_traits<_Ip>::difference_type n) {
      { i += n } -> std::same_as<_Ip&>;
      { i -= n } -> std::same_as<_Ip&>;
      { i + n } -> std::same_as<_Ip>;
      { n + i } -> std::same_as<_Ip>;
      { i - n } -> std::same_as<_Ip>;
      { i - i } -> std::same_as<decltype(n)>;
      { i[n] } -> std::convertible_to<std::iter_reference_t<_Ip>>;
    };

template <class _Ip>
concept __has_all_iterator_members = requires {
  typename _Ip::difference_type;
  typename _Ip::value_type;
  typename _Ip::reference;
  typename _Ip::iterator_category;
};

// The members of iterator_traits<I>, computed per [iterator.traits]/3.
template <class _Ip>
struct __iterator_traits_impl {}; // not an iterator

// 3.1: I defines all four member types
template <class _Ip>
  requires __has_all_iterator_members<_Ip>
struct __iterator_traits_impl<_Ip> {
  using iterator_category = typename _Ip::iterator_category;
  using value_type = typename _Ip::value_type;
  using difference_type = typename _Ip::difference_type;
  using pointer = decltype([] {
    if constexpr (requires { typename _Ip::pointer; })
      return std::type_identity<typename _Ip::pointer>{};
    else
      return std::type_identity<void>{};
  }())::type;
  using reference = typename _Ip::reference;
};

template <class _Ip>
consteval auto __cpp17_input_pointer() {
  if constexpr (requires { typename _Ip::pointer; })
    return std::type_identity<typename _Ip::pointer>{};
  else if constexpr (requires(_Ip& i) { i.operator->(); })
    return std::type_identity<decltype(std::declval<_Ip&>().operator->())>{};
  else
    return std::type_identity<void>{};
}
template <class _Ip>
consteval auto __cpp17_input_reference() {
  if constexpr (requires { typename _Ip::reference; })
    return std::type_identity<typename _Ip::reference>{};
  else
    return std::type_identity<std::iter_reference_t<_Ip>>{};
}
template <class _Ip>
consteval auto __cpp17_input_category() {
  if constexpr (requires { typename _Ip::iterator_category; })
    return std::type_identity<typename _Ip::iterator_category>{};
  else if constexpr (__cpp17_random_access_iterator<_Ip>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else if constexpr (__cpp17_bidirectional_iterator<_Ip>)
    return std::type_identity<std::bidirectional_iterator_tag>{};
  else if constexpr (__cpp17_forward_iterator<_Ip>)
    return std::type_identity<std::forward_iterator_tag>{};
  else
    return std::type_identity<std::input_iterator_tag>{};
}

// 3.2: I models cpp17-input-iterator
template <class _Ip>
  requires(!__has_all_iterator_members<_Ip>) && __cpp17_input_iterator<_Ip>
struct __iterator_traits_impl<_Ip> {
  using iterator_category = typename decltype(__cpp17_input_category<_Ip>())::type;
  using value_type = typename std::indirectly_readable_traits<_Ip>::value_type;
  using difference_type = typename std::incrementable_traits<_Ip>::difference_type;
  using pointer = typename decltype(__cpp17_input_pointer<_Ip>())::type;
  using reference = typename decltype(__cpp17_input_reference<_Ip>())::type;
};

// 3.3: I models cpp17-iterator
template <class _Ip>
  requires(!__has_all_iterator_members<_Ip>) && (!__cpp17_input_iterator<_Ip>) && __cpp17_iterator<_Ip>
struct __iterator_traits_impl<_Ip> {
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = decltype([] {
    if constexpr (requires { typename std::incrementable_traits<_Ip>::difference_type; })
      return std::type_identity<typename std::incrementable_traits<_Ip>::difference_type>{};
    else
      return std::type_identity<void>{};
  }())::type;
  using pointer = void;
  using reference = void;
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Ip>
struct iterator_traits : __ycxx::__detail::__iterator_traits_impl<_Ip> {
private:
  friend struct __ycxx::__detail::__iterator_traits_access;
  using __primary_marker = iterator_traits; // marks the primary template ([iterator.traits]/4)
};

template <class _Tp>
  requires is_object_v<_Tp>
struct iterator_traits<_Tp*> {
  using iterator_concept = contiguous_iterator_tag;
  using iterator_category = random_access_iterator_tag;
  using value_type = remove_cv_t<_Tp>;
  using difference_type = ptrdiff_t;
  using pointer = _Tp*;
  using reference = _Tp&;
};

} // namespace std

// ---------------------------------------------------------------------------------------------
// [iterator.cust.move], [iterator.cust.swap]
// ---------------------------------------------------------------------------------------------
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__iter_move_cpo {

void iter_move() = delete;

template <class _Tp>
concept __adl_iter_move = (std::is_class_v<std::remove_cvref_t<_Tp>> || std::is_union_v<std::remove_cvref_t<_Tp>> ||
                         std::is_enum_v<std::remove_cvref_t<_Tp>>) &&
                        requires(_Tp&& t) { iter_move(static_cast<_Tp&&>(t)); };

template <class _Tp>
consteval bool __iter_move_noexcept() {
  if constexpr (__adl_iter_move<_Tp>)
    return noexcept(iter_move(std::declval<_Tp>()));
  else
    return noexcept(*std::declval<_Tp>());
}

// The result type, spelled out rather than deduced: deducing it would instantiate the
// iterator's operator* (eagerly, on Clang, as it is constexpr) whenever a concept merely checks
// iter_move, e.g. for a list<T> iterator while T is still incomplete.
template <class _Tp>
struct result {
  using type = decltype(*std::declval<_Tp>());
};
template <class _Tp>
  requires(!__adl_iter_move<_Tp>) && std::is_lvalue_reference_v<decltype(*std::declval<_Tp>())>
struct result<_Tp> {
  using type = std::remove_reference_t<decltype(*std::declval<_Tp>())>&&;
};
template <class _Tp>
  requires __adl_iter_move<_Tp>
struct result<_Tp> {
  using type = decltype(iter_move(std::declval<_Tp>()));
};

struct __fn {
  template <class _Tp>
    requires __adl_iter_move<_Tp> || requires(_Tp&& t) { *static_cast<_Tp&&>(t); }
  [[nodiscard]] constexpr typename result<_Tp>::type operator()(_Tp&& t) const noexcept(__iter_move_noexcept<_Tp>()) {
    if constexpr (__adl_iter_move<_Tp>)
      return iter_move(static_cast<_Tp&&>(t));
    else if constexpr (std::is_lvalue_reference_v<decltype(*static_cast<_Tp&&>(t))>)
      return static_cast<std::remove_reference_t<decltype(*static_cast<_Tp&&>(t))>&&>(*static_cast<_Tp&&>(t));
    else
      return *static_cast<_Tp&&>(t);
  }
};

}} // namespace __ycxx::__detail::__iter_move_cpo

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace cpo {
inline constexpr __ycxx::__detail::__iter_move_cpo::__fn iter_move{};
}
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] std {

template <__ycxx::__detail::__dereferenceable _Tp>
  requires requires(_Tp& t) {
    { ranges::iter_move(t) } -> __ycxx::__detail::__can_reference;
  }
using iter_rvalue_reference_t = decltype(ranges::iter_move(declval<_Tp&>()));

// [iterator.concept.readable]
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _In>
concept __indirectly_readable_impl =
    requires(const _In in) {
      typename std::iter_value_t<_In>;
      typename std::iter_reference_t<_In>;
      typename std::iter_rvalue_reference_t<_In>;
      { *in } -> std::same_as<std::iter_reference_t<_In>>;
      { std::ranges::iter_move(in) } -> std::same_as<std::iter_rvalue_reference_t<_In>>;
    } && std::common_reference_with<std::iter_reference_t<_In>&&, std::iter_value_t<_In>&> &&
    std::common_reference_with<std::iter_reference_t<_In>&&, std::iter_rvalue_reference_t<_In>&&> &&
    std::common_reference_with<std::iter_rvalue_reference_t<_In>&&, const std::iter_value_t<_In>&>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _In>
concept indirectly_readable = __ycxx::__detail::__indirectly_readable_impl<remove_cvref_t<_In>>;

template <indirectly_readable _Tp>
using iter_common_reference_t = common_reference_t<iter_reference_t<_Tp>, iter_value_t<_Tp>&>;

// [iterator.concept.writable]
template <class _Out, class _Tp>
concept indirectly_writable = requires(_Out&& __o, _Tp&& t) {
  *__o = static_cast<_Tp&&>(t);
  *static_cast<_Out&&>(__o) = static_cast<_Tp&&>(t);
  const_cast<const iter_reference_t<_Out>&&>(*__o) = static_cast<_Tp&&>(t);
  const_cast<const iter_reference_t<_Out>&&>(*static_cast<_Out&&>(__o)) = static_cast<_Tp&&>(t);
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// is-integer-like / is-signed-integer-like: libycxx's only integer-class type is int128 where
// the compiler provides it (treated as an integer type by the language anyway).
template <class _Tp>
concept __integer_like = std::integral<_Tp> && !__is_same(__remove_cv(_Tp), bool);
template <class _Tp>
concept __signed_integer_like = __integer_like<_Tp> && std::signed_integral<_Tp>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [iterator.concept.winc]
template <class _Ip>
concept weakly_incrementable = movable<_Ip> && requires(_Ip i) {
  typename iter_difference_t<_Ip>;
  requires __ycxx::__detail::__signed_integer_like<iter_difference_t<_Ip>>;
  { ++i } -> same_as<_Ip&>;
  i++;
};

template <class _Ip>
concept incrementable = regular<_Ip> && weakly_incrementable<_Ip> && requires(_Ip i) {
  { i++ } -> same_as<_Ip>;
};

template <class _Ip>
concept input_or_output_iterator = requires(_Ip i) {
  { *i } -> __ycxx::__detail::__can_reference;
} && weakly_incrementable<_Ip>;

template <class _Sp, class _Ip>
concept sentinel_for = semiregular<_Sp> && input_or_output_iterator<_Ip> && __ycxx::__detail::__weakly_equality_comparable_with<_Sp, _Ip>;

template <class _Sp, class _Ip>
constexpr bool disable_sized_sentinel_for = false;

template <class _Sp, class _Ip>
concept sized_sentinel_for =
    sentinel_for<_Sp, _Ip> && !disable_sized_sentinel_for<remove_cv_t<_Sp>, remove_cv_t<_Ip>> &&
    requires(const _Ip& i, const _Sp& s) {
      { s - i } -> same_as<iter_difference_t<_Ip>>;
      { i - s } -> same_as<iter_difference_t<_Ip>>;
    };

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// ITER_TRAITS(I) / ITER_CONCEPT(I)
template <class _Ip>
using __iter_traits = std::conditional_t<__is_primary_iterator_traits<_Ip>, _Ip, std::iterator_traits<_Ip>>;

template <class _Ip>
consteval auto __iter_concept_impl() {
  using _Tp = __iter_traits<_Ip>;
  if constexpr (requires { typename _Tp::iterator_concept; })
    return std::type_identity<typename _Tp::iterator_concept>{};
  else if constexpr (requires { typename _Tp::iterator_category; })
    return std::type_identity<typename _Tp::iterator_category>{};
  else if constexpr (__is_primary_iterator_traits<_Ip>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else
    return std::type_identity<void>{}; // no ITER_CONCEPT
}
template <class _Ip>
using __iter_concept = typename decltype(__iter_concept_impl<_Ip>())::type;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Ip>
concept input_iterator = input_or_output_iterator<_Ip> && indirectly_readable<_Ip> &&
                         requires { typename __ycxx::__detail::__iter_concept<_Ip>; } &&
                         derived_from<__ycxx::__detail::__iter_concept<_Ip>, input_iterator_tag>;

template <class _Ip, class _Tp>
concept output_iterator = input_or_output_iterator<_Ip> && indirectly_writable<_Ip, _Tp> && requires(_Ip i, _Tp&& t) {
  *i++ = static_cast<_Tp&&>(t);
};

template <class _Ip>
concept forward_iterator = input_iterator<_Ip> && derived_from<__ycxx::__detail::__iter_concept<_Ip>, forward_iterator_tag> &&
                           incrementable<_Ip> && sentinel_for<_Ip, _Ip>;

template <class _Ip>
concept bidirectional_iterator = forward_iterator<_Ip> &&
                                 derived_from<__ycxx::__detail::__iter_concept<_Ip>, bidirectional_iterator_tag> &&
                                 requires(_Ip i) {
                                   { --i } -> same_as<_Ip&>;
                                   { i-- } -> same_as<_Ip>;
                                 };

template <class _Ip>
concept random_access_iterator =
    bidirectional_iterator<_Ip> && derived_from<__ycxx::__detail::__iter_concept<_Ip>, random_access_iterator_tag> &&
    totally_ordered<_Ip> && sized_sentinel_for<_Ip, _Ip> && requires(_Ip i, const _Ip __j, const iter_difference_t<_Ip> n) {
      { i += n } -> same_as<_Ip&>;
      { __j + n } -> same_as<_Ip>;
      { n + __j } -> same_as<_Ip>;
      { i -= n } -> same_as<_Ip&>;
      { __j - n } -> same_as<_Ip>;
      { __j[n] } -> same_as<iter_reference_t<_Ip>>;
    };

template <class _Ip>
concept contiguous_iterator =
    random_access_iterator<_Ip> && derived_from<__ycxx::__detail::__iter_concept<_Ip>, contiguous_iterator_tag> &&
    is_lvalue_reference_v<iter_reference_t<_Ip>> && same_as<iter_value_t<_Ip>, remove_cvref_t<iter_reference_t<_Ip>>> &&
    requires(const _Ip& i) {
      { std::to_address(i) } -> same_as<add_pointer_t<iter_reference_t<_Ip>>>;
    };

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// projected-impl ([projected]): projected<I, Proj> is the nested class `type`. A nested class
// is not a template specialization, so ADL on it neither instantiates I or Proj nor sees
// __ycxx::__detail. It records I and Proj (reserved member names) so that indirect-value-t can see
// through it ([indirectcallable.traits], P2609).
template <class _Ip, class _Proj>
struct __projected_impl {
  struct type {
    using value_type = std::remove_cvref_t<std::invoke_result_t<_Proj&, std::iter_reference_t<_Ip>>>;
    using __projected_iter = _Ip;
    using __projected_proj = _Proj;
    std::invoke_result_t<_Proj&, std::iter_reference_t<_Ip>> operator*() const; // not defined
  };
};
template <class _Ip, class _Proj>
  requires std::weakly_incrementable<_Ip>
struct __projected_impl<_Ip, _Proj> {
  struct type {
    using value_type = std::remove_cvref_t<std::invoke_result_t<_Proj&, std::iter_reference_t<_Ip>>>;
    using difference_type = std::iter_difference_t<_Ip>;
    using __projected_iter = _Ip;
    using __projected_proj = _Proj;
    std::invoke_result_t<_Proj&, std::iter_reference_t<_Ip>> operator*() const; // not defined
  };
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
concept __is_projected = requires {
  typename _Tp::__projected_iter;
  typename _Tp::__projected_proj;
} && std::is_same_v<_Tp, typename __adl_free::__projected_impl<typename _Tp::__projected_iter, typename _Tp::__projected_proj>::type>;

template <class _Tp>
struct __indirect_value {
  using type = std::iter_value_t<_Tp>&;
};
template <__is_projected _Pp>
struct __indirect_value<_Pp> {
  using type = std::invoke_result_t<typename _Pp::__projected_proj&, typename __indirect_value<typename _Pp::__projected_iter>::type>;
};
template <class _Tp>
using __indirect_value_t = typename __indirect_value<_Tp>::type;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [indirectcallable.indirectinvocable]
template <class _Fp, class _Ip>
concept indirectly_unary_invocable =
    indirectly_readable<_Ip> && copy_constructible<_Fp> && invocable<_Fp&, __ycxx::__detail::__indirect_value_t<_Ip>> &&
    invocable<_Fp&, iter_reference_t<_Ip>> &&
    common_reference_with<invoke_result_t<_Fp&, __ycxx::__detail::__indirect_value_t<_Ip>>, invoke_result_t<_Fp&, iter_reference_t<_Ip>>>;

template <class _Fp, class _Ip>
concept indirectly_regular_unary_invocable =
    indirectly_readable<_Ip> && copy_constructible<_Fp> && regular_invocable<_Fp&, __ycxx::__detail::__indirect_value_t<_Ip>> &&
    regular_invocable<_Fp&, iter_reference_t<_Ip>> &&
    common_reference_with<invoke_result_t<_Fp&, __ycxx::__detail::__indirect_value_t<_Ip>>, invoke_result_t<_Fp&, iter_reference_t<_Ip>>>;

template <class _Fp, class _Ip>
concept indirect_unary_predicate = indirectly_readable<_Ip> && copy_constructible<_Fp> &&
                                   predicate<_Fp&, __ycxx::__detail::__indirect_value_t<_Ip>> && predicate<_Fp&, iter_reference_t<_Ip>>;

template <class _Fp, class _I1, class _I2>
concept indirect_binary_predicate =
    indirectly_readable<_I1> && indirectly_readable<_I2> && copy_constructible<_Fp> &&
    predicate<_Fp&, __ycxx::__detail::__indirect_value_t<_I1>, __ycxx::__detail::__indirect_value_t<_I2>> &&
    predicate<_Fp&, __ycxx::__detail::__indirect_value_t<_I1>, iter_reference_t<_I2>> &&
    predicate<_Fp&, iter_reference_t<_I1>, __ycxx::__detail::__indirect_value_t<_I2>> &&
    predicate<_Fp&, iter_reference_t<_I1>, iter_reference_t<_I2>>;

template <class _Fp, class _I1, class _I2 = _I1>
concept indirect_equivalence_relation =
    indirectly_readable<_I1> && indirectly_readable<_I2> && copy_constructible<_Fp> &&
    equivalence_relation<_Fp&, __ycxx::__detail::__indirect_value_t<_I1>, __ycxx::__detail::__indirect_value_t<_I2>> &&
    equivalence_relation<_Fp&, __ycxx::__detail::__indirect_value_t<_I1>, iter_reference_t<_I2>> &&
    equivalence_relation<_Fp&, iter_reference_t<_I1>, __ycxx::__detail::__indirect_value_t<_I2>> &&
    equivalence_relation<_Fp&, iter_reference_t<_I1>, iter_reference_t<_I2>>;

template <class _Fp, class _I1, class _I2 = _I1>
concept indirect_strict_weak_order =
    indirectly_readable<_I1> && indirectly_readable<_I2> && copy_constructible<_Fp> &&
    strict_weak_order<_Fp&, __ycxx::__detail::__indirect_value_t<_I1>, __ycxx::__detail::__indirect_value_t<_I2>> &&
    strict_weak_order<_Fp&, __ycxx::__detail::__indirect_value_t<_I1>, iter_reference_t<_I2>> &&
    strict_weak_order<_Fp&, iter_reference_t<_I1>, __ycxx::__detail::__indirect_value_t<_I2>> &&
    strict_weak_order<_Fp&, iter_reference_t<_I1>, iter_reference_t<_I2>>;

template <class _Fp, class... _Is>
  requires(indirectly_readable<_Is> && ...) && invocable<_Fp, iter_reference_t<_Is>...>
using indirect_result_t = invoke_result_t<_Fp, iter_reference_t<_Is>...>;

// [projected]
template <indirectly_readable _Ip, indirectly_regular_unary_invocable<_Ip> _Proj>
using projected = typename __ycxx::__adl_free::__projected_impl<_Ip, _Proj>::type;

template <indirectly_readable _Ip, indirectly_regular_unary_invocable<_Ip> _Proj>
using projected_value_t = remove_cvref_t<invoke_result_t<_Proj&, iter_value_t<_Ip>&>>;

// [alg.req]
template <class _In, class _Out>
concept indirectly_movable = indirectly_readable<_In> && indirectly_writable<_Out, iter_rvalue_reference_t<_In>>;
template <class _In, class _Out>
concept indirectly_movable_storable =
    indirectly_movable<_In, _Out> && indirectly_writable<_Out, iter_value_t<_In>> && movable<iter_value_t<_In>> &&
    constructible_from<iter_value_t<_In>, iter_rvalue_reference_t<_In>> &&
    assignable_from<iter_value_t<_In>&, iter_rvalue_reference_t<_In>>;
template <class _In, class _Out>
concept indirectly_copyable = indirectly_readable<_In> && indirectly_writable<_Out, iter_reference_t<_In>>;
template <class _In, class _Out>
concept indirectly_copyable_storable =
    indirectly_copyable<_In, _Out> && indirectly_writable<_Out, iter_value_t<_In>&> &&
    indirectly_writable<_Out, const iter_value_t<_In>&> && indirectly_writable<_Out, iter_value_t<_In>&&> &&
    indirectly_writable<_Out, const iter_value_t<_In>&&> && copyable<iter_value_t<_In>> &&
    constructible_from<iter_value_t<_In>, iter_reference_t<_In>> &&
    assignable_from<iter_value_t<_In>&, iter_reference_t<_In>>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// iter-exchange-move ([iterator.cust.swap]). Outside the CPO's namespace, which must declare
// nothing ADL could find on the iter_swap object.
template <class _Xp, class _Yp>
constexpr std::iter_value_t<_Xp> __iter_exchange_move(_Xp&& __x, _Yp&& y) noexcept(
    noexcept(std::iter_value_t<_Xp>(std::ranges::iter_move(__x))) && noexcept(*__x = std::ranges::iter_move(y))) {
  std::iter_value_t<_Xp> __old(std::ranges::iter_move(__x));
  *__x = std::ranges::iter_move(y);
  return __old;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__iter_swap_cpo {

template <class _I1, class _I2>
void iter_swap(_I1, _I2) = delete;

template <class _Tp, class _Up>
concept __adl_iter_swap =
    (std::is_class_v<std::remove_cvref_t<_Tp>> || std::is_union_v<std::remove_cvref_t<_Tp>> ||
     std::is_enum_v<std::remove_cvref_t<_Tp>> || std::is_class_v<std::remove_cvref_t<_Up>> ||
     std::is_union_v<std::remove_cvref_t<_Up>> || std::is_enum_v<std::remove_cvref_t<_Up>>) &&
    requires(_Tp&& t, _Up&& __u) { iter_swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)); };

template <class _Tp, class _Up>
consteval bool __iter_swap_noexcept() {
  if constexpr (__adl_iter_swap<_Tp, _Up>)
    return noexcept((void)iter_swap(std::declval<_Tp>(), std::declval<_Up>()));
  else if constexpr (std::indirectly_readable<_Tp> && std::indirectly_readable<_Up> &&
                     std::swappable_with<std::iter_reference_t<_Tp>, std::iter_reference_t<_Up>>)
    return noexcept(std::ranges::swap(*std::declval<_Tp>(), *std::declval<_Up>()));
  else
    return noexcept((void)(*std::declval<_Tp>() = ::__ycxx::__detail::__iter_exchange_move(std::declval<_Up>(), std::declval<_Tp>())));
}

struct __fn {
  template <class _Tp, class _Up>
    requires __adl_iter_swap<_Tp, _Up> ||
             (std::indirectly_readable<_Tp> && std::indirectly_readable<_Up> &&
              std::swappable_with<std::iter_reference_t<_Tp>, std::iter_reference_t<_Up>>) ||
             (std::indirectly_movable_storable<_Tp, _Up> && std::indirectly_movable_storable<_Up, _Tp>)
  constexpr void operator()(_Tp&& t, _Up&& __u) const noexcept(__iter_swap_noexcept<_Tp, _Up>()) {
    if constexpr (__adl_iter_swap<_Tp, _Up>)
      (void)iter_swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u));
    else if constexpr (std::indirectly_readable<_Tp> && std::indirectly_readable<_Up> &&
                       std::swappable_with<std::iter_reference_t<_Tp>, std::iter_reference_t<_Up>>)
      std::ranges::swap(*t, *__u);
    else
      (void)(*t = ::__ycxx::__detail::__iter_exchange_move(__u, t));
  }
};

}} // namespace __ycxx::__detail::__iter_swap_cpo

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace cpo {
inline constexpr __ycxx::__detail::__iter_swap_cpo::__fn iter_swap{};
}
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _I1, class _I2 = _I1>
concept indirectly_swappable = indirectly_readable<_I1> && indirectly_readable<_I2> && requires(const _I1 __i1, const _I2 __i2) {
  ranges::iter_swap(__i1, __i1);
  ranges::iter_swap(__i2, __i2);
  ranges::iter_swap(__i1, __i2);
  ranges::iter_swap(__i2, __i1);
};

template <class _I1, class _I2, class _Rp, class _P1 = identity, class _P2 = identity>
concept indirectly_comparable = indirect_binary_predicate<_Rp, projected<_I1, _P1>, projected<_I2, _P2>>;

template <class _Ip>
concept permutable = forward_iterator<_Ip> && indirectly_movable_storable<_Ip, _Ip> && indirectly_swappable<_Ip, _Ip>;

template <class _I1, class _I2, class _Out, class _Rp = ranges::less, class _P1 = identity, class _P2 = identity>
concept mergeable = input_iterator<_I1> && input_iterator<_I2> && weakly_incrementable<_Out> &&
                    indirectly_copyable<_I1, _Out> && indirectly_copyable<_I2, _Out> &&
                    indirect_strict_weak_order<_Rp, projected<_I1, _P1>, projected<_I2, _P2>>;

template <class _Ip, class _Rp = ranges::less, class _Pp = identity>
concept sortable = permutable<_Ip> && indirect_strict_weak_order<_Rp, projected<_Ip, _Pp>>;

} // namespace std
