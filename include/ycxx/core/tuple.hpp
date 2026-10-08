// libycxx core: <tuple>, plus uses-allocator construction ([allocator.uses.construction]) which
// is specified in terms of tuples.
//
// Layout: one base subobject per element (tuple_leaf<I, T>) holding the element as a
// [[no_unique_address]] member, in declaration order, so empty elements take no space.
#pragma once

#include <ycxx/core/pair.hpp>
#include <ycxx/core/ignore.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/integer_sequence.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Tp, class _Up>
concept __different_from_ = !__is_same(std::remove_cvref_t<_Tp>, std::remove_cvref_t<_Up>);

template <class _Tp>
inline constexpr bool __is_tuple_specialization = false;
template <class... _Ts>
inline constexpr bool __is_tuple_specialization<std::tuple<_Ts...>> = true;

struct __alloc_tag_t {};

// Uses-allocator construction of one element in place ([allocator.uses.construction]).
template <class _Tp, class _Alloc, class... _Args>
constexpr _Tp __make_using_alloc(const _Alloc& a, _Args&&... __args);

template <std::size_t _Ip, class _Tp>
struct __tuple_leaf {
  [[no_unique_address]] _Tp value;

  constexpr __tuple_leaf() : value() {}
  template <class... _Args>
  constexpr explicit __tuple_leaf(std::in_place_t, _Args&&... __args) : value(static_cast<_Args&&>(__args)...) {}
  // Uses-allocator construction ([allocator.uses.construction]) performed directly into the
  // element: no intermediate prvalue, so non-movable elements work.
  template <class _Alloc, class... _Args>
    requires(!std::uses_allocator_v<std::remove_cv_t<_Tp>, _Alloc> && !__is_pair_v<std::remove_cv_t<_Tp>>)
  constexpr __tuple_leaf(__alloc_tag_t, const _Alloc&, _Args&&... __args) : value(static_cast<_Args&&>(__args)...) {}
  template <class _Alloc, class... _Args>
    requires std::uses_allocator_v<std::remove_cv_t<_Tp>, _Alloc> &&
             std::is_constructible_v<_Tp, std::allocator_arg_t, const _Alloc&, _Args...>
  constexpr __tuple_leaf(__alloc_tag_t, const _Alloc& a, _Args&&... __args)
      : value(std::allocator_arg, a, static_cast<_Args&&>(__args)...) {}
  template <class _Alloc, class... _Args>
    requires std::uses_allocator_v<std::remove_cv_t<_Tp>, _Alloc> &&
             (!std::is_constructible_v<_Tp, std::allocator_arg_t, const _Alloc&, _Args...>)
  constexpr __tuple_leaf(__alloc_tag_t, const _Alloc& a, _Args&&... __args) : value(static_cast<_Args&&>(__args)..., a) {}
  template <class _Alloc, class... _Args>
    requires(!std::uses_allocator_v<std::remove_cv_t<_Tp>, _Alloc> && __is_pair_v<std::remove_cv_t<_Tp>>)
  constexpr __tuple_leaf(__alloc_tag_t, const _Alloc& a, _Args&&... __args)
      : value(__make_using_alloc<_Tp>(a, static_cast<_Args&&>(__args)...)) {}
};

template <class _Seq, class... _Ts>
struct __tuple_storage;
template <std::size_t... _Ip, class... _Ts>
struct __tuple_storage<std::index_sequence<_Ip...>, _Ts...> : __tuple_leaf<_Ip, _Ts>... {
  constexpr __tuple_storage() = default;
  template <class... _Args>
  constexpr explicit __tuple_storage(std::in_place_t, _Args&&... __args)
      : __tuple_leaf<_Ip, _Ts>(std::in_place, static_cast<_Args&&>(__args))... {}
  template <class _Alloc, class... _Args>
  constexpr __tuple_storage(__alloc_tag_t, const _Alloc& a, _Args&&... __args)
      : __tuple_leaf<_Ip, _Ts>(__alloc_tag_t{}, a, static_cast<_Args&&>(__args))... {}
  // Default-initialise each element with an allocator.
  template <class _Alloc>
  constexpr __tuple_storage(__alloc_tag_t, const _Alloc& a, std::in_place_t) : __tuple_leaf<_Ip, _Ts>(__alloc_tag_t{}, a)... {}
};

template <std::size_t _Ip, class _Tp>
constexpr _Tp& __leaf_get(__tuple_leaf<_Ip, _Tp>& __l) noexcept {
  return __l.value;
}
template <std::size_t _Ip, class _Tp>
constexpr const _Tp& __leaf_get(const __tuple_leaf<_Ip, _Tp>& __l) noexcept {
  return __l.value;
}

template <class _Tp>
void __implicit_copy_list_init(const _Tp&);
template <class _Tp>
concept __implicit_default = requires { __implicit_copy_list_init<_Tp>({}); };

// ---- constraint helpers ------------------------------------------------------------------
// Each element-wise property is its own class template, instantiated only when a constraint
// reaches it. Constructor constraints check cheap conditions first; constraint satisfaction
// short-circuits, which avoids recursive instantiation (e.g. tuple<X> where X is constructible
// from anything). A normal overload requires `_Cp && !__dangles`, its deleted twin `_Cp && __dangles`,
// so the pair is mutually exclusive and needs no subsumption.
template <template <class, class> class _Pred, class _TT, class _UT>
struct __all_pairs {
  static constexpr bool value = false;
};
template <template <class, class> class _Pred, class... _Ts, class... _Us>
  requires(sizeof...(_Ts) == sizeof...(_Us))
struct __all_pairs<_Pred, std::tuple<_Ts...>, std::tuple<_Us...>> {
  static constexpr bool value = (_Pred<_Ts, _Us>::value && ...);
};
template <template <class, class> class _Pred, class _TT, class _UT>
struct __any_pair {
  static constexpr bool value = false;
};
template <template <class, class> class _Pred, class... _Ts, class... _Us>
  requires(sizeof...(_Ts) == sizeof...(_Us))
struct __any_pair<_Pred, std::tuple<_Ts...>, std::tuple<_Us...>> {
  static constexpr bool value = (_Pred<_Ts, _Us>::value || ...);
};

template <class _Tp, class _Up>
struct __converts_to : std::bool_constant<std::is_convertible_v<_Up, _Tp>> {};
template <class _Tp, class _Up>
struct __assignable_from_ : std::bool_constant<std::is_assignable_v<_Tp&, _Up>> {};
template <class _Tp, class _Up>
struct __const_assignable_from_ : std::bool_constant<std::is_assignable_v<const _Tp&, _Up>> {};

template <class _TT, class _UT>
concept __elems_constructible = __all_pairs<std::is_constructible, _TT, _UT>::value;
template <class _TT, class _UT>
concept __elems_convertible = __all_pairs<__converts_to, _TT, _UT>::value;
template <class _TT, class _UT>
concept __elems_dangle = __any_pair<std::reference_constructs_from_temporary, _TT, _UT>::value;
template <class _TT, class _UT>
concept __elems_nothrow = __all_pairs<std::is_nothrow_constructible, _TT, _UT>::value;
template <class _TT, class _UT>
concept __elems_assignable = __all_pairs<__assignable_from_, _TT, _UT>::value;
template <class _TT, class _UT>
concept __elems_const_assignable = __all_pairs<__const_assignable_from_, _TT, _UT>::value;

// Element access types of a tuple-like `_Up&&` ("decltype(get<I>(FWD(u)))...").
template <class _Up, class _Seq>
struct __get_types;
template <class _Up, std::size_t... _Ip>
struct __get_types<_Up, std::index_sequence<_Ip...>> {
  using type = std::tuple<decltype(get<_Ip>(std::declval<_Up>()))...>;
};
// For a tuple specialization the types are spelled out, without the unqualified call: its
// argument-dependent lookup would instantiate the element types' template arguments
// (a tuple<Holder<Incomplete>*> must stay copyable).
template <class _Up, std::size_t... _Ip>
  requires __is_tuple_specialization<std::remove_cvref_t<_Up>>
struct __get_types<_Up, std::index_sequence<_Ip...>> {
  template <std::size_t _Jp>
  using __elem = std::tuple_element_t<_Jp, std::remove_cvref_t<_Up>>;
  template <std::size_t _Jp>
  using __cv_elem = std::conditional_t<std::is_const_v<std::remove_reference_t<_Up>>, const __elem<_Jp>, __elem<_Jp>>;
  // get<J>(u) on an lvalue gives cv_elem<J>&, on an rvalue cv_elem<J>&& (collapsing for references).
  using type = std::tuple<std::conditional_t<std::is_lvalue_reference_v<_Up>, __cv_elem<_Ip>&, __cv_elem<_Ip>&&>...>;
};
template <class _Up>
using __get_types_t =
    typename __get_types<_Up, std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<_Up>>>>::type;

// [tuple.cnstr]/12 disambiguation for the UTypes&&... constructors.
template <class _TT, class... _Us>
concept __tuple_args_ok =
    sizeof...(_Us) == std::tuple_size_v<_TT> && sizeof...(_Us) >= 1 &&
    (sizeof...(_Us) > 1 || !__is_same(std::remove_cvref_t<_Us...[0]>, _TT)) &&
    (sizeof...(_Us) == 1 || sizeof...(_Us) > 3 || !__is_same(std::remove_cvref_t<_Us...[0]>, std::allocator_arg_t) ||
     __is_same(std::remove_cvref_t<std::tuple_element_t<0, _TT>>, std::allocator_arg_t));

// [tuple.cnstr]/21: from a tuple<U> when sizeof...(Types) == 1 (T is Types, U is UTypes).
template <class _Tp, class _Up, class _Src>
concept __tuple_conv_single_ok1 =
    !__is_same(_Tp, _Up) && !std::is_convertible_v<_Src, _Tp> && !std::is_constructible_v<_Tp, _Src>;
template <class _TT, class _Src>
concept __tuple_conv_single_ok =
    std::tuple_size_v<_TT> != 1 ||
    __tuple_conv_single_ok1<std::tuple_element_t<0, _TT>, std::tuple_element_t<0, std::remove_cvref_t<_Src>>, _Src>;

// [tuple.cnstr]/29: from another tuple-like when sizeof...(Types) == 1.
template <class _TT, class _Src>
concept __tuple_like_single_ok = std::tuple_size_v<_TT> != 1 ||
                               (!std::is_convertible_v<_Src, std::tuple_element_t<0, _TT>> &&
                                !std::is_constructible_v<std::tuple_element_t<0, _TT>, _Src>);

// Constructible from a cvref tuple specialization `_Src` (constraints in standard order).
template <class _TT, class _Src>
concept __tuple_from_tuple = __is_tuple_specialization<std::remove_cvref_t<_Src>> &&
                           std::tuple_size_v<std::remove_cvref_t<_Src>> == std::tuple_size_v<_TT> &&
                           __tuple_conv_single_ok<_TT, _Src> && __elems_constructible<_TT, __get_types_t<_Src>>;
// Constructible from any other tuple-like `_Src` (pair, array, complex; not subrange).
template <class _TT, class _Src>
concept __tuple_from_other = __tuple_like<_Src> && !__is_tuple_specialization<std::remove_cvref_t<_Src>> &&
                           !__is_subrange<std::remove_cvref_t<_Src>> &&
                           std::tuple_size_v<std::remove_cvref_t<_Src>> == std::tuple_size_v<_TT> &&
                           __tuple_like_single_ok<_TT, _Src> && __elems_constructible<_TT, __get_types_t<_Src>>;
template <class _TT, class _Src>
concept __tuple_from = __tuple_from_tuple<_TT, _Src> || __tuple_from_other<_TT, _Src>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class... _Types>
class tuple {
  using __storage = __ycxx::__detail::__tuple_storage<index_sequence_for<_Types...>, _Types...>;
  [[no_unique_address]] __storage __s_;

  template <class... _Up>
  friend class tuple;
  template <size_t _Ip, class... _Tp>
  friend constexpr tuple_element_t<_Ip, tuple<_Tp...>>& get(tuple<_Tp...>&) noexcept;
  template <size_t _Ip, class... _Tp>
  friend constexpr const tuple_element_t<_Ip, tuple<_Tp...>>& get(const tuple<_Tp...>&) noexcept;

  using __self = tuple<_Types...>;

  // Unpacks a tuple-like source into the storage.
  template <class _Up, size_t... _Ip>
  constexpr tuple(__ycxx::__detail::__alloc_tag_t*, _Up&& __u, index_sequence<_Ip...>)
      : __s_(in_place, get<_Ip>(static_cast<_Up&&>(__u))...) {}
  template <class _Alloc, class _Up, size_t... _Ip>
  constexpr tuple(__ycxx::__detail::__alloc_tag_t*, const _Alloc& a, _Up&& __u, index_sequence<_Ip...>)
      : __s_(__ycxx::__detail::__alloc_tag_t{}, a, get<_Ip>(static_cast<_Up&&>(__u))...) {}
  template <class _Up>
  using __seq_of = make_index_sequence<tuple_size_v<remove_cvref_t<_Up>>>;
  template <class _Src>
  using gets = __ycxx::__detail::__get_types_t<_Src>;

  template <class _Tup, size_t... _Ip>
  constexpr void __assign_from(_Tup&& __u, index_sequence<_Ip...>) {
    ((void)(__ycxx::__detail::__leaf_get<_Ip>(__s_) = get<_Ip>(static_cast<_Tup&&>(__u))), ...);
  }
  // Assigns through const T& (the const-qualified assignment operators).
  template <class _Tup, size_t... _Ip>
  constexpr void __assign_from(_Tup&& __u, index_sequence<_Ip...>) const {
    ((void)(__ycxx::__detail::__leaf_get<_Ip>(__s_) = get<_Ip>(static_cast<_Tup&&>(__u))), ...);
  }

public:
  // ---- [tuple.cnstr] ----
  constexpr explicit((!__ycxx::__detail::__implicit_default<_Types> || ...)) tuple()
    requires(is_default_constructible_v<_Types> && ...)
      : __s_() {}

  // noexcept as the converting constructor below: a strengthening ([res.on.exception.handling]/5)
  // the senders rely on, deciding whether decayed-tuple{as...} can throw from the decay-copies
  // of as... ([exec.when.all]/12, /17; [exec.into.variant]/6).
  constexpr explicit(!(is_convertible_v<const _Types&, _Types> && ...)) tuple(const _Types&... __args) noexcept(
      (is_nothrow_copy_constructible_v<_Types> && ...))
    requires(sizeof...(_Types) >= 1) && (is_copy_constructible_v<_Types> && ...)
      : __s_(in_place, __args...) {}

  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_args_ok<tuple, _UTypes...> &&
             __ycxx::__detail::__elems_constructible<tuple, tuple<_UTypes&&...>> &&
             (!__ycxx::__detail::__elems_dangle<tuple, tuple<_UTypes&&...>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, tuple<_UTypes&&...>>) tuple(_UTypes&&... __u) noexcept(
      __ycxx::__detail::__elems_nothrow<tuple, tuple<_UTypes&&...>>)
      : __s_(in_place, static_cast<_UTypes&&>(__u)...) {}
  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_args_ok<tuple, _UTypes...> &&
             __ycxx::__detail::__elems_constructible<tuple, tuple<_UTypes&&...>> &&
             __ycxx::__detail::__elems_dangle<tuple, tuple<_UTypes&&...>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, tuple<_UTypes&&...>>) tuple(_UTypes&&...) = delete;

  tuple(const tuple&) = default;
  tuple(tuple&&) = default;

  // Converting constructors from tuple<UTypes...> and pair<U1, U2> in all four value categories
  // (separate overloads, as specified, so an rvalue can fall back to the const& form), and from
  // other tuple-like types. Generated pattern: each has a deleted twin for dangling references.
  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, tuple<_UTypes...>&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&>>) tuple(tuple<_UTypes...>& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), static_cast<tuple<_UTypes...>&>(__u), __seq_of<tuple<_UTypes...>&>{}) {}
  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, tuple<_UTypes...>&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&>>) tuple(tuple<_UTypes...>&) = delete;
  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, const tuple<_UTypes...>&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&>>) tuple(const tuple<_UTypes...>& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), static_cast<const tuple<_UTypes...>&>(__u), __seq_of<const tuple<_UTypes...>&>{}) {}
  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, const tuple<_UTypes...>&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&>>) tuple(const tuple<_UTypes...>&) = delete;
  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, tuple<_UTypes...>&&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&&>>) tuple(tuple<_UTypes...>&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), static_cast<tuple<_UTypes...>&&>(__u), __seq_of<tuple<_UTypes...>&&>{}) {}
  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, tuple<_UTypes...>&&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&&>>) tuple(tuple<_UTypes...>&&) = delete;
  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, const tuple<_UTypes...>&&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&&>>) tuple(const tuple<_UTypes...>&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), static_cast<const tuple<_UTypes...>&&>(__u), __seq_of<const tuple<_UTypes...>&&>{}) {}
  template <class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, const tuple<_UTypes...>&&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&&>>) tuple(const tuple<_UTypes...>&&) = delete;
  template <class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, pair<_U1, _U2>&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&>>) tuple(pair<_U1, _U2>& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), static_cast<pair<_U1, _U2>&>(__u), __seq_of<pair<_U1, _U2>&>{}) {}
  template <class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, pair<_U1, _U2>&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&>>) tuple(pair<_U1, _U2>&) = delete;
  template <class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, const pair<_U1, _U2>&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&>>) tuple(const pair<_U1, _U2>& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), static_cast<const pair<_U1, _U2>&>(__u), __seq_of<const pair<_U1, _U2>&>{}) {}
  template <class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, const pair<_U1, _U2>&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&>>) tuple(const pair<_U1, _U2>&) = delete;
  template <class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, pair<_U1, _U2>&&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&&>>) tuple(pair<_U1, _U2>&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), static_cast<pair<_U1, _U2>&&>(__u), __seq_of<pair<_U1, _U2>&&>{}) {}
  template <class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, pair<_U1, _U2>&&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&&>>) tuple(pair<_U1, _U2>&&) = delete;
  template <class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, const pair<_U1, _U2>&&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&&>>) tuple(const pair<_U1, _U2>&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), static_cast<const pair<_U1, _U2>&&>(__u), __seq_of<const pair<_U1, _U2>&&>{}) {}
  template <class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, const pair<_U1, _U2>&&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&&>>) tuple(const pair<_U1, _U2>&&) = delete;
  template <class _Src>
    requires __ycxx::__detail::__tuple_from_other<tuple, _Src> && (!__ycxx::__detail::__is_pair_v<remove_cvref_t<_Src>>) && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<_Src>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<_Src>>) tuple(_Src&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), static_cast<_Src&&>(__u), __seq_of<_Src>{}) {}
  template <class _Src>
    requires __ycxx::__detail::__tuple_from_other<tuple, _Src> && (!__ycxx::__detail::__is_pair_v<remove_cvref_t<_Src>>) && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<_Src>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<_Src>>) tuple(_Src&&) = delete;

  // ---- allocator-extended constructors ----
  template <class _Alloc>
    requires(is_default_constructible_v<_Types> && ...)
  constexpr explicit((!__ycxx::__detail::__implicit_default<_Types> || ...)) tuple(allocator_arg_t, const _Alloc& a)
      : __s_(__ycxx::__detail::__alloc_tag_t{}, a, in_place) {}
  template <class _Alloc>
    requires(sizeof...(_Types) >= 1) && (is_copy_constructible_v<_Types> && ...)
  constexpr explicit(!(is_convertible_v<const _Types&, _Types> && ...))
      tuple(allocator_arg_t, const _Alloc& a, const _Types&... __args)
      : __s_(__ycxx::__detail::__alloc_tag_t{}, a, __args...) {}
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_args_ok<tuple, _UTypes...> &&
             __ycxx::__detail::__elems_constructible<tuple, tuple<_UTypes&&...>> &&
             (!__ycxx::__detail::__elems_dangle<tuple, tuple<_UTypes&&...>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, tuple<_UTypes&&...>>)
      tuple(allocator_arg_t, const _Alloc& a, _UTypes&&... __u)
      : __s_(__ycxx::__detail::__alloc_tag_t{}, a, static_cast<_UTypes&&>(__u)...) {}
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_args_ok<tuple, _UTypes...> &&
             __ycxx::__detail::__elems_constructible<tuple, tuple<_UTypes&&...>> &&
             __ycxx::__detail::__elems_dangle<tuple, tuple<_UTypes&&...>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, tuple<_UTypes&&...>>)
      tuple(allocator_arg_t, const _Alloc& a, _UTypes&&...) = delete;
  template <class _Alloc>
    requires(is_copy_constructible_v<_Types> && ...)
  constexpr tuple(allocator_arg_t, const _Alloc& a, const tuple& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, __u, index_sequence_for<_Types...>{}) {}
  template <class _Alloc>
    requires(is_move_constructible_v<_Types> && ...)
  constexpr tuple(allocator_arg_t, const _Alloc& a, tuple&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<tuple&&>(__u),
              index_sequence_for<_Types...>{}) {}
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, tuple<_UTypes...>&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&>>) tuple(allocator_arg_t, const _Alloc& a, tuple<_UTypes...>& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<tuple<_UTypes...>&>(__u), __seq_of<tuple<_UTypes...>&>{}) {}
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, tuple<_UTypes...>&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&>>) tuple(allocator_arg_t, const _Alloc& a, tuple<_UTypes...>&) = delete;
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, const tuple<_UTypes...>&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&>>) tuple(allocator_arg_t, const _Alloc& a, const tuple<_UTypes...>& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<const tuple<_UTypes...>&>(__u), __seq_of<const tuple<_UTypes...>&>{}) {}
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, const tuple<_UTypes...>&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&>>) tuple(allocator_arg_t, const _Alloc& a, const tuple<_UTypes...>&) = delete;
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, tuple<_UTypes...>&&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&&>>) tuple(allocator_arg_t, const _Alloc& a, tuple<_UTypes...>&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<tuple<_UTypes...>&&>(__u), __seq_of<tuple<_UTypes...>&&>{}) {}
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, tuple<_UTypes...>&&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<tuple<_UTypes...>&&>>) tuple(allocator_arg_t, const _Alloc& a, tuple<_UTypes...>&&) = delete;
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, const tuple<_UTypes...>&&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&&>>) tuple(allocator_arg_t, const _Alloc& a, const tuple<_UTypes...>&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<const tuple<_UTypes...>&&>(__u), __seq_of<const tuple<_UTypes...>&&>{}) {}
  template <class _Alloc, class... _UTypes>
    requires __ycxx::__detail::__tuple_from_tuple<tuple, const tuple<_UTypes...>&&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const tuple<_UTypes...>&&>>) tuple(allocator_arg_t, const _Alloc& a, const tuple<_UTypes...>&&) = delete;
  template <class _Alloc, class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, pair<_U1, _U2>&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&>>) tuple(allocator_arg_t, const _Alloc& a, pair<_U1, _U2>& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<pair<_U1, _U2>&>(__u), __seq_of<pair<_U1, _U2>&>{}) {}
  template <class _Alloc, class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, pair<_U1, _U2>&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&>>) tuple(allocator_arg_t, const _Alloc& a, pair<_U1, _U2>&) = delete;
  template <class _Alloc, class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, const pair<_U1, _U2>&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&>>) tuple(allocator_arg_t, const _Alloc& a, const pair<_U1, _U2>& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<const pair<_U1, _U2>&>(__u), __seq_of<const pair<_U1, _U2>&>{}) {}
  template <class _Alloc, class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, const pair<_U1, _U2>&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&>>) tuple(allocator_arg_t, const _Alloc& a, const pair<_U1, _U2>&) = delete;
  template <class _Alloc, class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, pair<_U1, _U2>&&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&&>>) tuple(allocator_arg_t, const _Alloc& a, pair<_U1, _U2>&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<pair<_U1, _U2>&&>(__u), __seq_of<pair<_U1, _U2>&&>{}) {}
  template <class _Alloc, class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, pair<_U1, _U2>&&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<pair<_U1, _U2>&&>>) tuple(allocator_arg_t, const _Alloc& a, pair<_U1, _U2>&&) = delete;
  template <class _Alloc, class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, const pair<_U1, _U2>&&> && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&&>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&&>>) tuple(allocator_arg_t, const _Alloc& a, const pair<_U1, _U2>&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<const pair<_U1, _U2>&&>(__u), __seq_of<const pair<_U1, _U2>&&>{}) {}
  template <class _Alloc, class _U1, class _U2>
    requires __ycxx::__detail::__tuple_from_other<tuple, const pair<_U1, _U2>&&> && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&&>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<const pair<_U1, _U2>&&>>) tuple(allocator_arg_t, const _Alloc& a, const pair<_U1, _U2>&&) = delete;
  template <class _Alloc, class _Src>
    requires __ycxx::__detail::__tuple_from_other<tuple, _Src> && (!__ycxx::__detail::__is_pair_v<remove_cvref_t<_Src>>) && (!__ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<_Src>>)
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<_Src>>) tuple(allocator_arg_t, const _Alloc& a, _Src&& __u)
      : tuple(static_cast<__ycxx::__detail::__alloc_tag_t*>(nullptr), a, static_cast<_Src&&>(__u), __seq_of<_Src>{}) {}
  template <class _Alloc, class _Src>
    requires __ycxx::__detail::__tuple_from_other<tuple, _Src> && (!__ycxx::__detail::__is_pair_v<remove_cvref_t<_Src>>) && __ycxx::__detail::__elems_dangle<tuple, __ycxx::__detail::__get_types_t<_Src>>
  constexpr explicit(!__ycxx::__detail::__elems_convertible<tuple, __ycxx::__detail::__get_types_t<_Src>>) tuple(allocator_arg_t, const _Alloc& a, _Src&&) = delete;

  // ---- [tuple.assign] ----
  // When every element is a trivially assignable object, the defaulted operators keep tuple
  // trivially copyable; otherwise the element-wise (reference-assigning) versions apply.
  static constexpr bool __trivial_copy_assign = ((is_trivially_copy_assignable_v<_Types> && !is_reference_v<_Types>) && ...);
  static constexpr bool __trivial_move_assign = ((is_trivially_move_assignable_v<_Types> && !is_reference_v<_Types>) && ...);
  tuple& operator=(const tuple&)
    requires __trivial_copy_assign
  = default;
  tuple& operator=(tuple&&)
    requires __trivial_move_assign
  = default;
  constexpr tuple& operator=(const tuple& __u)
    requires(!__trivial_copy_assign) && (is_copy_assignable_v<_Types> && ...)
  {
    __assign_from(__u, index_sequence_for<_Types...>{});
    return *this;
  }
  constexpr const tuple& operator=(const tuple& __u) const
    requires(is_copy_assignable_v<const _Types> && ...)
  {
    __assign_from(__u, index_sequence_for<_Types...>{});
    return *this;
  }
  constexpr tuple& operator=(tuple&& __u) noexcept((is_nothrow_move_assignable_v<_Types> && ...))
    requires(!__trivial_move_assign) && (is_move_assignable_v<_Types> && ...)
  {
    __assign_from(static_cast<tuple&&>(__u), index_sequence_for<_Types...>{});
    return *this;
  }
  constexpr const tuple& operator=(tuple&& __u) const
    requires(is_assignable_v<const _Types&, _Types> && ...)
  {
    __assign_from(static_cast<tuple&&>(__u), index_sequence_for<_Types...>{});
    return *this;
  }
  template <class... _UTypes>
    requires(sizeof...(_UTypes) == sizeof...(_Types)) && (is_assignable_v<_Types&, const _UTypes&> && ...)
  constexpr tuple& operator=(const tuple<_UTypes...>& __u) {
    __assign_from(__u, index_sequence_for<_Types...>{});
    return *this;
  }
  template <class... _UTypes>
    requires(sizeof...(_UTypes) == sizeof...(_Types)) && (is_assignable_v<const _Types&, const _UTypes&> && ...)
  constexpr const tuple& operator=(const tuple<_UTypes...>& __u) const {
    __assign_from(__u, index_sequence_for<_Types...>{});
    return *this;
  }
  template <class... _UTypes>
    requires(sizeof...(_UTypes) == sizeof...(_Types)) && (is_assignable_v<_Types&, _UTypes> && ...)
  constexpr tuple& operator=(tuple<_UTypes...>&& __u) {
    __assign_from(static_cast<tuple<_UTypes...>&&>(__u), index_sequence_for<_Types...>{});
    return *this;
  }
  template <class... _UTypes>
    requires(sizeof...(_UTypes) == sizeof...(_Types)) && (is_assignable_v<const _Types&, _UTypes> && ...)
  constexpr const tuple& operator=(tuple<_UTypes...>&& __u) const {
    __assign_from(static_cast<tuple<_UTypes...>&&>(__u), index_sequence_for<_Types...>{});
    return *this;
  }
  template <class _U1, class _U2>
    requires(sizeof...(_Types) == 2) && is_assignable_v<_Types...[0] &, const _U1&> && is_assignable_v<_Types...[1] &, const _U2&>
  constexpr tuple& operator=(const pair<_U1, _U2>& __u) {
    __ycxx::__detail::__leaf_get<0>(__s_) = __u.first;
    __ycxx::__detail::__leaf_get<1>(__s_) = __u.second;
    return *this;
  }
  template <class _U1, class _U2>
    requires(sizeof...(_Types) == 2) && is_assignable_v<const _Types...[0] &, const _U1&> &&
            is_assignable_v<const _Types...[1] &, const _U2&>
  constexpr const tuple& operator=(const pair<_U1, _U2>& __u) const {
    __ycxx::__detail::__leaf_get<0>(__s_) = __u.first;
    __ycxx::__detail::__leaf_get<1>(__s_) = __u.second;
    return *this;
  }
  template <class _U1, class _U2>
    requires(sizeof...(_Types) == 2) && is_assignable_v<_Types...[0] &, _U1> && is_assignable_v<_Types...[1] &, _U2>
  constexpr tuple& operator=(pair<_U1, _U2>&& __u) {
    __ycxx::__detail::__leaf_get<0>(__s_) = static_cast<_U1&&>(__u.first);
    __ycxx::__detail::__leaf_get<1>(__s_) = static_cast<_U2&&>(__u.second);
    return *this;
  }
  template <class _U1, class _U2>
    requires(sizeof...(_Types) == 2) && is_assignable_v<const _Types...[0] &, _U1> && is_assignable_v<const _Types...[1] &, _U2>
  constexpr const tuple& operator=(pair<_U1, _U2>&& __u) const {
    __ycxx::__detail::__leaf_get<0>(__s_) = static_cast<_U1&&>(__u.first);
    __ycxx::__detail::__leaf_get<1>(__s_) = static_cast<_U2&&>(__u.second);
    return *this;
  }
  template <__ycxx::__detail::__tuple_like _UTuple>
    requires(!__ycxx::__detail::__is_tuple_specialization<remove_cvref_t<_UTuple>>) &&
            (!__ycxx::__detail::__is_pair_v<remove_cvref_t<_UTuple>>) && (!__ycxx::__detail::__is_subrange<remove_cvref_t<_UTuple>>) &&
            (tuple_size_v<remove_cvref_t<_UTuple>> == sizeof...(_Types)) &&
            __ycxx::__detail::__elems_assignable<tuple, __ycxx::__detail::__get_types_t<_UTuple>>
  constexpr tuple& operator=(_UTuple&& __u) {
    __assign_from(static_cast<_UTuple&&>(__u), index_sequence_for<_Types...>{});
    return *this;
  }
  template <__ycxx::__detail::__tuple_like _UTuple>
    requires(!__ycxx::__detail::__is_tuple_specialization<remove_cvref_t<_UTuple>>) &&
            (!__ycxx::__detail::__is_pair_v<remove_cvref_t<_UTuple>>) && (!__ycxx::__detail::__is_subrange<remove_cvref_t<_UTuple>>) &&
            (tuple_size_v<remove_cvref_t<_UTuple>> == sizeof...(_Types)) &&
            __ycxx::__detail::__elems_const_assignable<tuple, __ycxx::__detail::__get_types_t<_UTuple>>
  constexpr const tuple& operator=(_UTuple&& __u) const {
    __assign_from(static_cast<_UTuple&&>(__u), index_sequence_for<_Types...>{});
    return *this;
  }

  // ---- [tuple.swap] ----
  constexpr void swap(tuple& __rhs) noexcept((is_nothrow_swappable_v<_Types> && ...)) {
    [&]<size_t... _Ip>(index_sequence<_Ip...>) {
      ((void)__ycxx::__detail::__swap_adl::__do_swap(__ycxx::__detail::__leaf_get<_Ip>(__s_), __ycxx::__detail::__leaf_get<_Ip>(__rhs.__s_)), ...);
    }(index_sequence_for<_Types...>{});
  }
  constexpr void swap(const tuple& __rhs) const noexcept((is_nothrow_swappable_v<const _Types> && ...)) {
    [&]<size_t... _Ip>(index_sequence<_Ip...>) {
      ((void)__ycxx::__detail::__swap_adl::__do_swap(__ycxx::__detail::__leaf_get<_Ip>(__s_), __ycxx::__detail::__leaf_get<_Ip>(__rhs.__s_)), ...);
    }(index_sequence_for<_Types...>{});
  }
};

template <>
class tuple<> {
public:
  constexpr tuple() noexcept = default;
  template <class _Alloc>
  constexpr tuple(allocator_arg_t, const _Alloc&) noexcept {}
  template <class _Alloc>
  constexpr tuple(allocator_arg_t, const _Alloc&, const tuple&) noexcept {}
  template <__ycxx::__detail::__tuple_like _UTuple>
    requires __ycxx::__detail::__different_from_<_UTuple, tuple> && (tuple_size_v<remove_cvref_t<_UTuple>> == 0) &&
             (!__ycxx::__detail::__is_subrange<remove_cvref_t<_UTuple>>)
  constexpr tuple(_UTuple&&) noexcept {}
  template <class _Alloc, __ycxx::__detail::__tuple_like _UTuple>
    requires __ycxx::__detail::__different_from_<_UTuple, tuple> && (tuple_size_v<remove_cvref_t<_UTuple>> == 0) &&
             (!__ycxx::__detail::__is_subrange<remove_cvref_t<_UTuple>>)
  constexpr tuple(allocator_arg_t, const _Alloc&, _UTuple&&) noexcept {}
  tuple(const tuple&) = default;
  tuple& operator=(const tuple&) = default;
  constexpr const tuple& operator=(const tuple&) const noexcept { return *this; }
  template <__ycxx::__detail::__tuple_like _UTuple>
    requires __ycxx::__detail::__different_from_<_UTuple, tuple> && (tuple_size_v<remove_cvref_t<_UTuple>> == 0)
  constexpr tuple& operator=(_UTuple&&) noexcept {
    return *this;
  }
  template <__ycxx::__detail::__tuple_like _UTuple>
    requires __ycxx::__detail::__different_from_<_UTuple, tuple> && (tuple_size_v<remove_cvref_t<_UTuple>> == 0)
  constexpr const tuple& operator=(_UTuple&&) const noexcept {
    return *this;
  }
  constexpr void swap(tuple&) noexcept {}
  constexpr void swap(const tuple&) const noexcept {}
};

// ---- deduction guides ----
template <class... _UTypes>
tuple(_UTypes...) -> tuple<_UTypes...>;
template <class _T1, class _T2>
tuple(pair<_T1, _T2>) -> tuple<_T1, _T2>;
template <class _Alloc, class... _UTypes>
tuple(allocator_arg_t, _Alloc, _UTypes...) -> tuple<_UTypes...>;
template <class _Alloc, class _T1, class _T2>
tuple(allocator_arg_t, _Alloc, pair<_T1, _T2>) -> tuple<_T1, _T2>;
template <class _Alloc, class... _UTypes>
tuple(allocator_arg_t, _Alloc, tuple<_UTypes...>) -> tuple<_UTypes...>;

// ---- [tuple.helper] ----
template <class... _Types>
struct tuple_size<tuple<_Types...>> : integral_constant<size_t, sizeof...(_Types)> {};
template <size_t _Ip, class... _Types>
struct tuple_element<_Ip, tuple<_Types...>> {
  static_assert(_Ip < sizeof...(_Types), "tuple_element index out of range");
  using type = _Types...[_Ip];
};

// ---- [tuple.elem] ----
template <size_t _Ip, class... _Types>
constexpr tuple_element_t<_Ip, tuple<_Types...>>& get(tuple<_Types...>& t) noexcept {
  static_assert(_Ip < sizeof...(_Types), "std::get: tuple index out of range");
  return __ycxx::__detail::__leaf_get<_Ip>(t.__s_);
}
template <size_t _Ip, class... _Types>
constexpr const tuple_element_t<_Ip, tuple<_Types...>>& get(const tuple<_Types...>& t) noexcept {
  static_assert(_Ip < sizeof...(_Types), "std::get: tuple index out of range");
  return __ycxx::__detail::__leaf_get<_Ip>(t.__s_);
}
template <size_t _Ip, class... _Types>
constexpr tuple_element_t<_Ip, tuple<_Types...>>&& get(tuple<_Types...>&& t) noexcept {
  return static_cast<tuple_element_t<_Ip, tuple<_Types...>>&&>(std::get<_Ip>(t));
}
template <size_t _Ip, class... _Types>
constexpr const tuple_element_t<_Ip, tuple<_Types...>>&& get(const tuple<_Types...>&& t) noexcept {
  return static_cast<const tuple_element_t<_Ip, tuple<_Types...>>&&>(std::get<_Ip>(t));
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp, class... _Types>
consteval std::size_t type_index() {
  constexpr bool __hits[] = {__is_same(_Tp, _Types)..., false};
  std::size_t found = sizeof...(_Types), count = 0;
  for (std::size_t i = 0; i < sizeof...(_Types); ++i)
    if (__hits[i]) {
      found = i;
      ++count;
    }
  return count == 1 ? found : sizeof...(_Types);
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp, class... _Types>
  requires(__ycxx::__detail::type_index<_Tp, _Types...>() < sizeof...(_Types))
constexpr _Tp& get(tuple<_Types...>& t) noexcept {
  return std::get<__ycxx::__detail::type_index<_Tp, _Types...>()>(t);
}
template <class _Tp, class... _Types>
  requires(__ycxx::__detail::type_index<_Tp, _Types...>() < sizeof...(_Types))
constexpr _Tp&& get(tuple<_Types...>&& t) noexcept {
  return std::get<__ycxx::__detail::type_index<_Tp, _Types...>()>(static_cast<tuple<_Types...>&&>(t));
}
template <class _Tp, class... _Types>
  requires(__ycxx::__detail::type_index<_Tp, _Types...>() < sizeof...(_Types))
constexpr const _Tp& get(const tuple<_Types...>& t) noexcept {
  return std::get<__ycxx::__detail::type_index<_Tp, _Types...>()>(t);
}
template <class _Tp, class... _Types>
  requires(__ycxx::__detail::type_index<_Tp, _Types...>() < sizeof...(_Types))
constexpr const _Tp&& get(const tuple<_Types...>&& t) noexcept {
  return std::get<__ycxx::__detail::type_index<_Tp, _Types...>()>(static_cast<const tuple<_Types...>&&>(t));
}

// ---- [tuple.creation] ----
template <class... _TTypes>
constexpr tuple<unwrap_ref_decay_t<_TTypes>...> make_tuple(_TTypes&&... t) {
  return tuple<unwrap_ref_decay_t<_TTypes>...>(static_cast<_TTypes&&>(t)...);
}
template <class... _TTypes>
constexpr tuple<_TTypes&&...> forward_as_tuple(_TTypes&&... t) noexcept {
  return tuple<_TTypes&&...>(static_cast<_TTypes&&>(t)...);
}
template <class... _TTypes>
constexpr tuple<_TTypes&...> tie(_TTypes&... t) noexcept {
  return tuple<_TTypes&...>(t...);
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// tuple_cat: (outer, inner) index pairs for the flattened element list.
template <class... _Tuples>
struct __cat_plan {
  static constexpr std::size_t __sizes[] = {std::tuple_size_v<std::remove_cvref_t<_Tuples>>..., 0};
  static constexpr std::size_t __total = (std::tuple_size_v<std::remove_cvref_t<_Tuples>> + ... + 0);
  struct __entry {
    std::size_t outer, __inner;
  };
  static constexpr auto __entries = [] {
    struct __arr {
      __entry e[__total ? __total : 1];
    } r{};
    std::size_t k = 0;
    for (std::size_t __o = 0; __o < sizeof...(_Tuples); ++__o)
      for (std::size_t i = 0; i < __sizes[__o]; ++i)
        r.e[k++] = {__o, i};
    return r;
  }();
};
template <class _Tup, std::size_t _Ip>
using __cat_elem = std::tuple_element_t<_Ip, std::remove_cvref_t<_Tup>>;

template <class _Plan, class _FwdTuple, class... _Tuples, std::size_t... _Kp>
constexpr auto __tuple_cat_impl(_FwdTuple&& __fwd, std::index_sequence<_Kp...>) {
  using result = std::tuple<__cat_elem<_Tuples...[_Plan::__entries.e[_Kp].outer], _Plan::__entries.e[_Kp].__inner>...>;
  return result(get<_Plan::__entries.e[_Kp].__inner>(
      std::get<_Plan::__entries.e[_Kp].outer>(static_cast<_FwdTuple&&>(__fwd)))...);
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <__ycxx::__detail::__tuple_like... _Tuples>
constexpr auto tuple_cat(_Tuples&&... __tpls) {
  using __plan = __ycxx::__detail::__cat_plan<_Tuples...>;
  return __ycxx::__detail::__tuple_cat_impl<__plan, tuple<_Tuples&&...>, _Tuples...>(
      std::forward_as_tuple(static_cast<_Tuples&&>(__tpls)...), make_index_sequence<__plan::__total>{});
}

// ---- [tuple.apply] ----
template <class _Fp, __ycxx::__detail::__tuple_like _Tuple>
  requires is_applicable_v<_Fp, _Tuple>
constexpr apply_result_t<_Fp, _Tuple> apply(_Fp&& __f, _Tuple&& t) noexcept(is_nothrow_applicable_v<_Fp, _Tuple>) {
  return [&]<size_t... _Ip>(index_sequence<_Ip...>) -> apply_result_t<_Fp, _Tuple> {
    return __ycxx::__detail::invoke(static_cast<_Fp&&>(__f), get<_Ip>(static_cast<_Tuple&&>(t))...);
  }(__ycxx::__detail::__tuple_indices<_Tuple>{});
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp, class _Tuple>
consteval bool __make_from_tuple_dangles() {
  if constexpr (std::tuple_size_v<std::remove_reference_t<_Tuple>> == 1)
    return std::reference_constructs_from_temporary_v<_Tp, decltype(get<0>(std::declval<_Tuple>()))>;
  else
    return false;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _Tp, __ycxx::__detail::__tuple_like _Tuple>
constexpr _Tp make_from_tuple(_Tuple&& t) {
  static_assert(!__ycxx::__detail::__make_from_tuple_dangles<_Tp, _Tuple>(),
                "std::make_from_tuple: would bind a reference to a temporary");
  return [&]<size_t... _Ip>(index_sequence<_Ip...>) -> _Tp {
    static_assert(is_constructible_v<_Tp, decltype(get<_Ip>(declval<_Tuple>()))...>,
                  "std::make_from_tuple: T is not constructible from the tuple's elements");
    return _Tp(get<_Ip>(static_cast<_Tuple&&>(t))...);
  }(__ycxx::__detail::__tuple_indices<_Tuple>{});
}

// ---- [tuple.rel] ----
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp, class _Up, std::size_t... _Ip>
consteval bool __tuple_eq_ok(std::index_sequence<_Ip...>*) {
  return (requires(const _Tp& t, const _Up& __u) {
    { get<_Ip>(t) == get<_Ip>(__u) } -> __boolean_testable;
  } && ...);
}
template <class _Tp, class _Up>
concept __tuple_eq_comparable = std::tuple_size_v<_Tp> == std::tuple_size_v<_Up> &&
                              __tuple_eq_ok<_Tp, _Up>(static_cast<__tuple_indices<_Tp>*>(nullptr));

template <class _Tp, class _Up, std::size_t... _Ip>
auto __tuple_cmp_cat(std::index_sequence<_Ip...>*)
    -> std::common_comparison_category_t<__synth_three_way_result<std::tuple_element_t<_Ip, _Tp>, std::tuple_element_t<_Ip, _Up>>...>;

template <class _Tp, class _Up>
using __tuple_cmp_result = decltype(__tuple_cmp_cat<_Tp, _Up>(static_cast<__tuple_indices<_Tp>*>(nullptr)));
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class... _TTypes, class... _UTypes>
  requires __ycxx::__detail::__tuple_eq_comparable<tuple<_TTypes...>, tuple<_UTypes...>>
constexpr bool operator==(const tuple<_TTypes...>& t, const tuple<_UTypes...>& __u) {
  return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
    return (static_cast<bool>(std::get<_Ip>(t) == std::get<_Ip>(__u)) && ...);
  }(index_sequence_for<_TTypes...>{});
}
template <class... _TTypes, __ycxx::__detail::__tuple_like _UTuple>
  requires(!__ycxx::__detail::__is_tuple_specialization<_UTuple>) &&
          __ycxx::__detail::__tuple_eq_comparable<tuple<_TTypes...>, _UTuple>
constexpr bool operator==(const tuple<_TTypes...>& t, const _UTuple& __u) {
  return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
    return (static_cast<bool>(std::get<_Ip>(t) == get<_Ip>(__u)) && ...);
  }(index_sequence_for<_TTypes...>{});
}

template <class... _TTypes, class... _UTypes>
  requires(sizeof...(_TTypes) == sizeof...(_UTypes))
constexpr __ycxx::__detail::__tuple_cmp_result<tuple<_TTypes...>, tuple<_UTypes...>> operator<=>(const tuple<_TTypes...>& t,
                                                                                        const tuple<_UTypes...>& __u) {
  using _Rp = __ycxx::__detail::__tuple_cmp_result<tuple<_TTypes...>, tuple<_UTypes...>>;
  _Rp r = _Rp::equivalent;
  [&]<size_t... _Ip>(index_sequence<_Ip...>) {
    (void)((r = __ycxx::__detail::__synth_three_way(std::get<_Ip>(t), std::get<_Ip>(__u)), r == 0) && ...);
  }(index_sequence_for<_TTypes...>{});
  return r;
}
template <class... _TTypes, __ycxx::__detail::__tuple_like _UTuple>
  requires(!__ycxx::__detail::__is_tuple_specialization<_UTuple>) && (sizeof...(_TTypes) == tuple_size_v<_UTuple>)
constexpr __ycxx::__detail::__tuple_cmp_result<tuple<_TTypes...>, _UTuple> operator<=>(const tuple<_TTypes...>& t,
                                                                              const _UTuple& __u) {
  using _Rp = __ycxx::__detail::__tuple_cmp_result<tuple<_TTypes...>, _UTuple>;
  _Rp r = _Rp::equivalent;
  [&]<size_t... _Ip>(index_sequence<_Ip...>) {
    (void)((r = __ycxx::__detail::__synth_three_way(std::get<_Ip>(t), get<_Ip>(__u)), r == 0) && ...);
  }(index_sequence_for<_TTypes...>{});
  return r;
}

// ---- [tuple.traits], [tuple.special] ----
template <class... _Types, class _Alloc>
struct uses_allocator<tuple<_Types...>, _Alloc> : true_type {};

template <class... _Types>
  requires(is_swappable_v<_Types> && ...)
constexpr void swap(tuple<_Types...>& __x, tuple<_Types...>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}
template <class... _Types>
  requires(is_swappable_v<const _Types> && ...)
constexpr void swap(const tuple<_Types...>& __x, const tuple<_Types...>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [tuple.common.ref] ----
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _TT, class _UT, template <class> class _TQ, template <class> class _UQ, class _Seq>
struct __tuple_common_ref;
template <class _TT, class _UT, template <class> class _TQ, template <class> class _UQ, std::size_t... _Ip>
  requires requires {
    typename std::tuple<std::common_reference_t<_TQ<std::tuple_element_t<_Ip, _TT>>, _UQ<std::tuple_element_t<_Ip, _UT>>>...>;
  }
struct __tuple_common_ref<_TT, _UT, _TQ, _UQ, std::index_sequence<_Ip...>> {
  using type = std::tuple<std::common_reference_t<_TQ<std::tuple_element_t<_Ip, _TT>>, _UQ<std::tuple_element_t<_Ip, _UT>>>...>;
};
template <class _TT, class _UT, class _Seq>
struct __tuple_common_type;
template <class _TT, class _UT, std::size_t... _Ip>
  requires requires { typename std::tuple<std::common_type_t<std::tuple_element_t<_Ip, _TT>, std::tuple_element_t<_Ip, _UT>>...>; }
struct __tuple_common_type<_TT, _UT, std::index_sequence<_Ip...>> {
  using type = std::tuple<std::common_type_t<std::tuple_element_t<_Ip, _TT>, std::tuple_element_t<_Ip, _UT>>...>;
};
template <class _TT, class _UT>
concept __tuple_common_candidates =
    (__is_tuple_specialization<_TT> || __is_tuple_specialization<_UT>) && __is_same(_TT, std::decay_t<_TT>) &&
    __is_same(_UT, std::decay_t<_UT>) && std::tuple_size_v<_TT> == std::tuple_size_v<_UT>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <__ycxx::__detail::__tuple_like _TTuple, __ycxx::__detail::__tuple_like _UTuple, template <class> class _TQual,
          template <class> class _UQual>
  requires __ycxx::__detail::__tuple_common_candidates<_TTuple, _UTuple> &&
           requires { typename __ycxx::__detail::__tuple_common_ref<_TTuple, _UTuple, _TQual, _UQual, __ycxx::__detail::__tuple_indices<_TTuple>>::type; }
struct basic_common_reference<_TTuple, _UTuple, _TQual, _UQual>
    : __ycxx::__detail::__tuple_common_ref<_TTuple, _UTuple, _TQual, _UQual, __ycxx::__detail::__tuple_indices<_TTuple>> {};

template <__ycxx::__detail::__tuple_like _TTuple, __ycxx::__detail::__tuple_like _UTuple>
  requires __ycxx::__detail::__tuple_common_candidates<_TTuple, _UTuple> &&
           requires { typename __ycxx::__detail::__tuple_common_type<_TTuple, _UTuple, __ycxx::__detail::__tuple_indices<_TTuple>>::type; }
struct common_type<_TTuple, _UTuple>
    : __ycxx::__detail::__tuple_common_type<_TTuple, _UTuple, __ycxx::__detail::__tuple_indices<_TTuple>> {};
}} // namespace std

// =============================================================================================
// [allocator.uses.construction]
// =============================================================================================
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp, class _Alloc, class... _Args>
  requires(!__ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>>)
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, _Args&&... __args) noexcept {
  if constexpr (!uses_allocator_v<remove_cv_t<_Tp>, _Alloc> && is_constructible_v<_Tp, _Args...>) {
    return std::forward_as_tuple(static_cast<_Args&&>(__args)...);
  } else if constexpr (uses_allocator_v<remove_cv_t<_Tp>, _Alloc> && is_constructible_v<_Tp, allocator_arg_t, const _Alloc&, _Args...>) {
    return tuple<allocator_arg_t, const _Alloc&, _Args&&...>(allocator_arg, __alloc, static_cast<_Args&&>(__args)...);
  } else if constexpr (uses_allocator_v<remove_cv_t<_Tp>, _Alloc> && is_constructible_v<_Tp, _Args..., const _Alloc&>) {
    return std::forward_as_tuple(static_cast<_Args&&>(__args)..., __alloc);
  } else {
    static_assert(__ycxx::__detail::__always_false<_Tp>, "uses-allocator construction: T is not constructible");
  }
}

// Declared here, defined after every other overload: its recursive calls for T1 and T2 use
// qualified lookup, which only sees overloads declared before the definition.
template <class _Tp, class _Alloc, class _Tuple1, class _Tuple2>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>>
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, piecewise_construct_t, _Tuple1&& __x,
                                                _Tuple2&& y) noexcept;
template <class _Tp, class _Alloc>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>>
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc) noexcept {
  return std::uses_allocator_construction_args<_Tp>(__alloc, piecewise_construct, tuple<>{}, tuple<>{});
}
template <class _Tp, class _Alloc, class _Up, class _Vp>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>>
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, _Up&& __u, _Vp&& __v) noexcept {
  return std::uses_allocator_construction_args<_Tp>(__alloc, piecewise_construct,
                                                  std::forward_as_tuple(static_cast<_Up&&>(__u)),
                                                  std::forward_as_tuple(static_cast<_Vp&&>(__v)));
}
template <class _Tp, class _Alloc, class _Up, class _Vp>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>>
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, pair<_Up, _Vp>& __pr) noexcept {
  return std::uses_allocator_construction_args<_Tp>(__alloc, piecewise_construct, std::forward_as_tuple(__pr.first),
                                                  std::forward_as_tuple(__pr.second));
}
template <class _Tp, class _Alloc, class _Up, class _Vp>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>>
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, const pair<_Up, _Vp>& __pr) noexcept {
  return std::uses_allocator_construction_args<_Tp>(__alloc, piecewise_construct, std::forward_as_tuple(__pr.first),
                                                  std::forward_as_tuple(__pr.second));
}
template <class _Tp, class _Alloc, class _Up, class _Vp>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>>
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, pair<_Up, _Vp>&& __pr) noexcept {
  return std::uses_allocator_construction_args<_Tp>(__alloc, piecewise_construct,
                                                  std::forward_as_tuple(static_cast<_Up&&>(__pr.first)),
                                                  std::forward_as_tuple(static_cast<_Vp&&>(__pr.second)));
}
template <class _Tp, class _Alloc, class _Up, class _Vp>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>>
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, const pair<_Up, _Vp>&& __pr) noexcept {
  return std::uses_allocator_construction_args<_Tp>(__alloc, piecewise_construct,
                                                  std::forward_as_tuple(static_cast<const _Up&&>(__pr.first)),
                                                  std::forward_as_tuple(static_cast<const _Vp&&>(__pr.second)));
}
template <class _Tp, class _Alloc, __ycxx::__detail::__pair_like _Pp>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>> && (!__ycxx::__detail::__is_subrange<remove_cvref_t<_Pp>>) &&
           (!__ycxx::__detail::__is_pair_v<remove_cvref_t<_Pp>>)
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, _Pp&& p) noexcept {
  return std::uses_allocator_construction_args<_Tp>(__alloc, piecewise_construct,
                                                  std::forward_as_tuple(get<0>(static_cast<_Pp&&>(p))),
                                                  std::forward_as_tuple(get<1>(static_cast<_Pp&&>(p))));
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _Tp, class _Alloc, class... _Args>
constexpr _Tp make_obj_using_allocator(const _Alloc& __alloc, _Args&&... __args);
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// pair-convert: a type convertible to any pair, for the final uses_allocator_construction_args
// overload ([allocator.uses.construction]/17).
template <class _Tp, class _Alloc, class _Up>
class __pair_converter {
  using __pair_type = std::remove_cv_t<_Tp>;
  const _Alloc& __alloc_;
  _Up& __u_;

  constexpr auto __do_construct(const __pair_type& p) const { return std::make_obj_using_allocator<__pair_type>(__alloc_, p); }
  constexpr auto __do_construct(__pair_type&& p) const {
    return std::make_obj_using_allocator<__pair_type>(__alloc_, static_cast<__pair_type&&>(p));
  }

public:
  constexpr __pair_converter(const _Alloc& a, _Up& __u) noexcept : __alloc_(a), __u_(__u) {}
  constexpr operator __pair_type() const { return __do_construct(static_cast<_Up&&>(__u_)); }
};
// FUN(u) from [allocator.uses.construction]/19: well-formed for pair and classes derived from it.
template <class _Ap, class _Bp>
void __pair_fun(const std::pair<_Ap, _Bp>&);
template <class _Up>
concept __pair_fun_callable = requires(_Up&& __u) { __pair_fun(static_cast<_Up&&>(__u)); };
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _Tp, class _Alloc, class _Up>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>> &&
           (__ycxx::__detail::__is_subrange<remove_cvref_t<_Up>> ||
            (!__ycxx::__detail::__pair_like<_Up> && !__ycxx::__detail::__pair_fun_callable<_Up>))
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, _Up&& __u) noexcept {
  return std::make_tuple(__ycxx::__detail::__pair_converter<_Tp, _Alloc, _Up>(__alloc, __u));
}

template <class _Tp, class _Alloc, class _Tuple1, class _Tuple2>
  requires __ycxx::__detail::__is_pair_v<remove_cv_t<_Tp>>
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, piecewise_construct_t, _Tuple1&& __x,
                                                _Tuple2&& y) noexcept {
  using _T1 = typename remove_cv_t<_Tp>::first_type;
  using _T2 = typename remove_cv_t<_Tp>::second_type;
  return std::make_tuple(
      piecewise_construct,
      std::apply([&__alloc](auto&&... a) { return std::uses_allocator_construction_args<_T1>(__alloc, static_cast<decltype(a)&&>(a)...); },
                 static_cast<_Tuple1&&>(__x)),
      std::apply([&__alloc](auto&&... a) { return std::uses_allocator_construction_args<_T2>(__alloc, static_cast<decltype(a)&&>(a)...); },
                 static_cast<_Tuple2&&>(y)));
}

template <class _Tp, class _Alloc, class... _Args>
constexpr _Tp make_obj_using_allocator(const _Alloc& __alloc, _Args&&... __args) {
  return std::make_from_tuple<_Tp>(std::uses_allocator_construction_args<_Tp>(__alloc, static_cast<_Args&&>(__args)...));
}

template <class _Tp, class _Alloc, class... _Args>
constexpr _Tp* uninitialized_construct_using_allocator(_Tp* p, const _Alloc& __alloc, _Args&&... __args) {
  return std::apply(
      [&]<class... _Up>(_Up&&... __xs) { return std::construct_at(p, static_cast<_Up&&>(__xs)...); },
      std::uses_allocator_construction_args<_Tp>(__alloc, static_cast<_Args&&>(__args)...));
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp, class _Alloc, class... _Args>
constexpr _Tp __make_using_alloc(const _Alloc& a, _Args&&... __args) {
  if constexpr (std::is_reference_v<_Tp>)
    return static_cast<_Tp>(static_cast<_Args...[0] &&>(__args...[0]));
  else
    return std::make_obj_using_allocator<_Tp>(a, static_cast<_Args&&>(__args)...);
}
}} // namespace __ycxx::__detail
