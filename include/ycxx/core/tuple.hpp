// libycxx core: <tuple>, plus uses-allocator construction ([allocator.uses.construction]) which
// is specified in terms of tuples.
//
// Layout: one base subobject per element (tuple_leaf<I, T>) holding the element as a
// [[no_unique_address]] member, in declaration order, so empty elements take no space.
#pragma once

#include <ycxx/core/pair.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/integer_sequence.hpp>

namespace ycxx::detail {

template <class T, class U>
concept different_from_ = !__is_same(std::remove_cvref_t<T>, std::remove_cvref_t<U>);

template <class T>
inline constexpr bool is_tuple_specialization = false;
template <class... Ts>
inline constexpr bool is_tuple_specialization<std::tuple<Ts...>> = true;

struct alloc_tag_t {};

// Uses-allocator construction of one element in place ([allocator.uses.construction]).
template <class T, class Alloc, class... Args>
constexpr T make_using_alloc(const Alloc& a, Args&&... args);

template <std::size_t I, class T>
struct tuple_leaf {
  [[no_unique_address]] T value;

  constexpr tuple_leaf() : value() {}
  template <class... Args>
  constexpr explicit tuple_leaf(std::in_place_t, Args&&... args) : value(static_cast<Args&&>(args)...) {}
  // Uses-allocator construction ([allocator.uses.construction]) performed directly into the
  // element: no intermediate prvalue, so non-movable elements work.
  template <class Alloc, class... Args>
    requires(!std::uses_allocator_v<std::remove_cv_t<T>, Alloc> && !is_pair_v<std::remove_cv_t<T>>)
  constexpr tuple_leaf(alloc_tag_t, const Alloc&, Args&&... args) : value(static_cast<Args&&>(args)...) {}
  template <class Alloc, class... Args>
    requires std::uses_allocator_v<std::remove_cv_t<T>, Alloc> &&
             std::is_constructible_v<T, std::allocator_arg_t, const Alloc&, Args...>
  constexpr tuple_leaf(alloc_tag_t, const Alloc& a, Args&&... args)
      : value(std::allocator_arg, a, static_cast<Args&&>(args)...) {}
  template <class Alloc, class... Args>
    requires std::uses_allocator_v<std::remove_cv_t<T>, Alloc> &&
             (!std::is_constructible_v<T, std::allocator_arg_t, const Alloc&, Args...>)
  constexpr tuple_leaf(alloc_tag_t, const Alloc& a, Args&&... args) : value(static_cast<Args&&>(args)..., a) {}
  template <class Alloc, class... Args>
    requires(!std::uses_allocator_v<std::remove_cv_t<T>, Alloc> && is_pair_v<std::remove_cv_t<T>>)
  constexpr tuple_leaf(alloc_tag_t, const Alloc& a, Args&&... args)
      : value(make_using_alloc<T>(a, static_cast<Args&&>(args)...)) {}
};

template <class Seq, class... Ts>
struct tuple_storage;
template <std::size_t... I, class... Ts>
struct tuple_storage<std::index_sequence<I...>, Ts...> : tuple_leaf<I, Ts>... {
  constexpr tuple_storage() = default;
  template <class... Args>
  constexpr explicit tuple_storage(std::in_place_t, Args&&... args)
      : tuple_leaf<I, Ts>(std::in_place, static_cast<Args&&>(args))... {}
  template <class Alloc, class... Args>
  constexpr tuple_storage(alloc_tag_t, const Alloc& a, Args&&... args)
      : tuple_leaf<I, Ts>(alloc_tag_t{}, a, static_cast<Args&&>(args))... {}
  // Default-initialise each element with an allocator.
  template <class Alloc>
  constexpr tuple_storage(alloc_tag_t, const Alloc& a, std::in_place_t) : tuple_leaf<I, Ts>(alloc_tag_t{}, a)... {}
};

template <std::size_t I, class T>
constexpr T& leaf_get(tuple_leaf<I, T>& l) noexcept {
  return l.value;
}
template <std::size_t I, class T>
constexpr const T& leaf_get(const tuple_leaf<I, T>& l) noexcept {
  return l.value;
}

template <class T>
void implicit_copy_list_init(const T&);
template <class T>
concept implicit_default = requires { implicit_copy_list_init<T>({}); };

// ---- constraint helpers ------------------------------------------------------------------
// Each element-wise property is its own class template, instantiated only when a constraint
// reaches it. Constructor constraints check cheap conditions first; constraint satisfaction
// short-circuits, which avoids recursive instantiation (e.g. tuple<X> where X is constructible
// from anything). A normal overload requires `C && !dangles`, its deleted twin `C && dangles`,
// so the pair is mutually exclusive and needs no subsumption.
template <template <class, class> class Pred, class TT, class UT>
struct all_pairs {
  static constexpr bool value = false;
};
template <template <class, class> class Pred, class... Ts, class... Us>
  requires(sizeof...(Ts) == sizeof...(Us))
struct all_pairs<Pred, std::tuple<Ts...>, std::tuple<Us...>> {
  static constexpr bool value = (Pred<Ts, Us>::value && ...);
};
template <template <class, class> class Pred, class TT, class UT>
struct any_pair {
  static constexpr bool value = false;
};
template <template <class, class> class Pred, class... Ts, class... Us>
  requires(sizeof...(Ts) == sizeof...(Us))
struct any_pair<Pred, std::tuple<Ts...>, std::tuple<Us...>> {
  static constexpr bool value = (Pred<Ts, Us>::value || ...);
};

template <class T, class U>
struct converts_to : std::bool_constant<std::is_convertible_v<U, T>> {};
template <class T, class U>
struct assignable_from_ : std::bool_constant<std::is_assignable_v<T&, U>> {};
template <class T, class U>
struct const_assignable_from_ : std::bool_constant<std::is_assignable_v<const T&, U>> {};

template <class TT, class UT>
concept elems_constructible = all_pairs<std::is_constructible, TT, UT>::value;
template <class TT, class UT>
concept elems_convertible = all_pairs<converts_to, TT, UT>::value;
template <class TT, class UT>
concept elems_dangle = any_pair<std::reference_constructs_from_temporary, TT, UT>::value;
template <class TT, class UT>
concept elems_nothrow = all_pairs<std::is_nothrow_constructible, TT, UT>::value;
template <class TT, class UT>
concept elems_assignable = all_pairs<assignable_from_, TT, UT>::value;
template <class TT, class UT>
concept elems_const_assignable = all_pairs<const_assignable_from_, TT, UT>::value;

// Element access types of a tuple-like `U&&` ("decltype(get<I>(FWD(u)))...").
template <class U, class Seq>
struct get_types;
template <class U, std::size_t... I>
struct get_types<U, std::index_sequence<I...>> {
  using type = std::tuple<decltype(get<I>(std::declval<U>()))...>;
};
template <class U>
using get_types_t =
    typename get_types<U, std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<U>>>>::type;

// [tuple.cnstr]/12 disambiguation for the UTypes&&... constructors.
template <class TT, class... Us>
concept tuple_args_ok =
    sizeof...(Us) == std::tuple_size_v<TT> && sizeof...(Us) >= 1 &&
    (sizeof...(Us) > 1 || !__is_same(std::remove_cvref_t<Us...[0]>, TT)) &&
    (sizeof...(Us) == 1 || sizeof...(Us) > 3 || !__is_same(std::remove_cvref_t<Us...[0]>, std::allocator_arg_t) ||
     __is_same(std::remove_cvref_t<std::tuple_element_t<0, TT>>, std::allocator_arg_t));

// [tuple.cnstr]/21: from a tuple<U> when sizeof...(Types) == 1 (T is Types, U is UTypes).
template <class T, class U, class Src>
concept tuple_conv_single_ok1 =
    !__is_same(T, U) && !std::is_convertible_v<Src, T> && !std::is_constructible_v<T, Src>;
template <class TT, class Src>
concept tuple_conv_single_ok =
    std::tuple_size_v<TT> != 1 ||
    tuple_conv_single_ok1<std::tuple_element_t<0, TT>, std::tuple_element_t<0, std::remove_cvref_t<Src>>, Src>;

// [tuple.cnstr]/29: from another tuple-like when sizeof...(Types) == 1.
template <class TT, class Src>
concept tuple_like_single_ok = std::tuple_size_v<TT> != 1 ||
                               (!std::is_convertible_v<Src, std::tuple_element_t<0, TT>> &&
                                !std::is_constructible_v<std::tuple_element_t<0, TT>, Src>);

// Constructible from a cvref tuple specialization `Src` (constraints in standard order).
template <class TT, class Src>
concept tuple_from_tuple = is_tuple_specialization<std::remove_cvref_t<Src>> &&
                           std::tuple_size_v<std::remove_cvref_t<Src>> == std::tuple_size_v<TT> &&
                           tuple_conv_single_ok<TT, Src> && elems_constructible<TT, get_types_t<Src>>;
// Constructible from any other tuple-like `Src` (pair, array, complex; not subrange).
template <class TT, class Src>
concept tuple_from_other = tuple_like<Src> && !is_tuple_specialization<std::remove_cvref_t<Src>> &&
                           !is_subrange<std::remove_cvref_t<Src>> &&
                           std::tuple_size_v<std::remove_cvref_t<Src>> == std::tuple_size_v<TT> &&
                           tuple_like_single_ok<TT, Src> && elems_constructible<TT, get_types_t<Src>>;
template <class TT, class Src>
concept tuple_from = tuple_from_tuple<TT, Src> || tuple_from_other<TT, Src>;

} // namespace ycxx::detail

namespace std {

template <class... Types>
class tuple {
  using storage = ycxx::detail::tuple_storage<index_sequence_for<Types...>, Types...>;
  [[no_unique_address]] storage s_;

  template <class... U>
  friend class tuple;
  template <size_t I, class... T>
  friend constexpr tuple_element_t<I, tuple<T...>>& get(tuple<T...>&) noexcept;
  template <size_t I, class... T>
  friend constexpr const tuple_element_t<I, tuple<T...>>& get(const tuple<T...>&) noexcept;

  using self = tuple<Types...>;
  static constexpr size_t N = sizeof...(Types);

  // Unpacks a tuple-like source into the storage.
  template <class U, size_t... I>
  constexpr tuple(ycxx::detail::alloc_tag_t*, U&& u, index_sequence<I...>)
      : s_(in_place, get<I>(static_cast<U&&>(u))...) {}
  template <class Alloc, class U, size_t... I>
  constexpr tuple(ycxx::detail::alloc_tag_t*, const Alloc& a, U&& u, index_sequence<I...>)
      : s_(ycxx::detail::alloc_tag_t{}, a, get<I>(static_cast<U&&>(u))...) {}
  template <class U>
  using seq_of = make_index_sequence<tuple_size_v<remove_cvref_t<U>>>;
  template <class Src>
  using gets = ycxx::detail::get_types_t<Src>;

  template <class Tup, size_t... I>
  constexpr void assign_from(Tup&& u, index_sequence<I...>) {
    ((void)(ycxx::detail::leaf_get<I>(s_) = get<I>(static_cast<Tup&&>(u))), ...);
  }
  // Assigns through const T& (the const-qualified assignment operators).
  template <class Tup, size_t... I>
  constexpr void assign_from(Tup&& u, index_sequence<I...>) const {
    ((void)(ycxx::detail::leaf_get<I>(s_) = get<I>(static_cast<Tup&&>(u))), ...);
  }

public:
  // ---- [tuple.cnstr] ----
  constexpr explicit((!ycxx::detail::implicit_default<Types> || ...)) tuple()
    requires(is_default_constructible_v<Types> && ...)
      : s_() {}

  constexpr explicit(!(is_convertible_v<const Types&, Types> && ...)) tuple(const Types&... args)
    requires(N >= 1) && (is_copy_constructible_v<Types> && ...)
      : s_(in_place, args...) {}

  template <class... UTypes>
    requires ycxx::detail::tuple_args_ok<tuple, UTypes...> &&
             ycxx::detail::elems_constructible<tuple, tuple<UTypes&&...>> &&
             (!ycxx::detail::elems_dangle<tuple, tuple<UTypes&&...>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, tuple<UTypes&&...>>) tuple(UTypes&&... u) noexcept(
      ycxx::detail::elems_nothrow<tuple, tuple<UTypes&&...>>)
      : s_(in_place, static_cast<UTypes&&>(u)...) {}
  template <class... UTypes>
    requires ycxx::detail::tuple_args_ok<tuple, UTypes...> &&
             ycxx::detail::elems_constructible<tuple, tuple<UTypes&&...>> &&
             ycxx::detail::elems_dangle<tuple, tuple<UTypes&&...>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, tuple<UTypes&&...>>) tuple(UTypes&&...) = delete;

  tuple(const tuple&) = default;
  tuple(tuple&&) = default;

  // Converting constructors from tuple<UTypes...> and pair<U1, U2> in all four value categories
  // (separate overloads, as specified, so an rvalue can fall back to the const& form), and from
  // other tuple-like types. Generated pattern: each has a deleted twin for dangling references.
  template <class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, tuple<UTypes...>&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&>>) tuple(tuple<UTypes...>& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), static_cast<tuple<UTypes...>&>(u), seq_of<tuple<UTypes...>&>{}) {}
  template <class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, tuple<UTypes...>&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&>>) tuple(tuple<UTypes...>&) = delete;
  template <class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, const tuple<UTypes...>&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&>>) tuple(const tuple<UTypes...>& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), static_cast<const tuple<UTypes...>&>(u), seq_of<const tuple<UTypes...>&>{}) {}
  template <class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, const tuple<UTypes...>&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&>>) tuple(const tuple<UTypes...>&) = delete;
  template <class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, tuple<UTypes...>&&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&&>>) tuple(tuple<UTypes...>&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), static_cast<tuple<UTypes...>&&>(u), seq_of<tuple<UTypes...>&&>{}) {}
  template <class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, tuple<UTypes...>&&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&&>>) tuple(tuple<UTypes...>&&) = delete;
  template <class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, const tuple<UTypes...>&&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&&>>) tuple(const tuple<UTypes...>&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), static_cast<const tuple<UTypes...>&&>(u), seq_of<const tuple<UTypes...>&&>{}) {}
  template <class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, const tuple<UTypes...>&&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&&>>) tuple(const tuple<UTypes...>&&) = delete;
  template <class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, pair<U1, U2>&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<pair<U1, U2>&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<pair<U1, U2>&>>) tuple(pair<U1, U2>& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), static_cast<pair<U1, U2>&>(u), seq_of<pair<U1, U2>&>{}) {}
  template <class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, pair<U1, U2>&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<pair<U1, U2>&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<pair<U1, U2>&>>) tuple(pair<U1, U2>&) = delete;
  template <class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, const pair<U1, U2>&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&>>) tuple(const pair<U1, U2>& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), static_cast<const pair<U1, U2>&>(u), seq_of<const pair<U1, U2>&>{}) {}
  template <class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, const pair<U1, U2>&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&>>) tuple(const pair<U1, U2>&) = delete;
  template <class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, pair<U1, U2>&&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<pair<U1, U2>&&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<pair<U1, U2>&&>>) tuple(pair<U1, U2>&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), static_cast<pair<U1, U2>&&>(u), seq_of<pair<U1, U2>&&>{}) {}
  template <class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, pair<U1, U2>&&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<pair<U1, U2>&&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<pair<U1, U2>&&>>) tuple(pair<U1, U2>&&) = delete;
  template <class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, const pair<U1, U2>&&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&&>>) tuple(const pair<U1, U2>&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), static_cast<const pair<U1, U2>&&>(u), seq_of<const pair<U1, U2>&&>{}) {}
  template <class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, const pair<U1, U2>&&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&&>>) tuple(const pair<U1, U2>&&) = delete;
  template <class Src>
    requires ycxx::detail::tuple_from_other<tuple, Src> && (!ycxx::detail::is_pair_v<remove_cvref_t<Src>>) && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<Src>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<Src>>) tuple(Src&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), static_cast<Src&&>(u), seq_of<Src>{}) {}
  template <class Src>
    requires ycxx::detail::tuple_from_other<tuple, Src> && (!ycxx::detail::is_pair_v<remove_cvref_t<Src>>) && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<Src>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<Src>>) tuple(Src&&) = delete;

  // ---- allocator-extended constructors ----
  template <class Alloc>
    requires(is_default_constructible_v<Types> && ...)
  constexpr explicit((!ycxx::detail::implicit_default<Types> || ...)) tuple(allocator_arg_t, const Alloc& a)
      : s_(ycxx::detail::alloc_tag_t{}, a, in_place) {}
  template <class Alloc>
    requires(N >= 1) && (is_copy_constructible_v<Types> && ...)
  constexpr explicit(!(is_convertible_v<const Types&, Types> && ...))
      tuple(allocator_arg_t, const Alloc& a, const Types&... args)
      : s_(ycxx::detail::alloc_tag_t{}, a, args...) {}
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_args_ok<tuple, UTypes...> &&
             ycxx::detail::elems_constructible<tuple, tuple<UTypes&&...>> &&
             (!ycxx::detail::elems_dangle<tuple, tuple<UTypes&&...>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, tuple<UTypes&&...>>)
      tuple(allocator_arg_t, const Alloc& a, UTypes&&... u)
      : s_(ycxx::detail::alloc_tag_t{}, a, static_cast<UTypes&&>(u)...) {}
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_args_ok<tuple, UTypes...> &&
             ycxx::detail::elems_constructible<tuple, tuple<UTypes&&...>> &&
             ycxx::detail::elems_dangle<tuple, tuple<UTypes&&...>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, tuple<UTypes&&...>>)
      tuple(allocator_arg_t, const Alloc& a, UTypes&&...) = delete;
  template <class Alloc>
    requires(is_copy_constructible_v<Types> && ...)
  constexpr tuple(allocator_arg_t, const Alloc& a, const tuple& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, u, index_sequence_for<Types...>{}) {}
  template <class Alloc>
    requires(is_move_constructible_v<Types> && ...)
  constexpr tuple(allocator_arg_t, const Alloc& a, tuple&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<tuple&&>(u),
              index_sequence_for<Types...>{}) {}
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, tuple<UTypes...>&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&>>) tuple(allocator_arg_t, const Alloc& a, tuple<UTypes...>& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<tuple<UTypes...>&>(u), seq_of<tuple<UTypes...>&>{}) {}
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, tuple<UTypes...>&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&>>) tuple(allocator_arg_t, const Alloc& a, tuple<UTypes...>&) = delete;
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, const tuple<UTypes...>&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&>>) tuple(allocator_arg_t, const Alloc& a, const tuple<UTypes...>& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<const tuple<UTypes...>&>(u), seq_of<const tuple<UTypes...>&>{}) {}
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, const tuple<UTypes...>&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&>>) tuple(allocator_arg_t, const Alloc& a, const tuple<UTypes...>&) = delete;
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, tuple<UTypes...>&&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&&>>) tuple(allocator_arg_t, const Alloc& a, tuple<UTypes...>&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<tuple<UTypes...>&&>(u), seq_of<tuple<UTypes...>&&>{}) {}
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, tuple<UTypes...>&&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<tuple<UTypes...>&&>>) tuple(allocator_arg_t, const Alloc& a, tuple<UTypes...>&&) = delete;
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, const tuple<UTypes...>&&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&&>>) tuple(allocator_arg_t, const Alloc& a, const tuple<UTypes...>&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<const tuple<UTypes...>&&>(u), seq_of<const tuple<UTypes...>&&>{}) {}
  template <class Alloc, class... UTypes>
    requires ycxx::detail::tuple_from_tuple<tuple, const tuple<UTypes...>&&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const tuple<UTypes...>&&>>) tuple(allocator_arg_t, const Alloc& a, const tuple<UTypes...>&&) = delete;
  template <class Alloc, class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, pair<U1, U2>&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<pair<U1, U2>&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<pair<U1, U2>&>>) tuple(allocator_arg_t, const Alloc& a, pair<U1, U2>& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<pair<U1, U2>&>(u), seq_of<pair<U1, U2>&>{}) {}
  template <class Alloc, class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, pair<U1, U2>&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<pair<U1, U2>&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<pair<U1, U2>&>>) tuple(allocator_arg_t, const Alloc& a, pair<U1, U2>&) = delete;
  template <class Alloc, class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, const pair<U1, U2>&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&>>) tuple(allocator_arg_t, const Alloc& a, const pair<U1, U2>& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<const pair<U1, U2>&>(u), seq_of<const pair<U1, U2>&>{}) {}
  template <class Alloc, class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, const pair<U1, U2>&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&>>) tuple(allocator_arg_t, const Alloc& a, const pair<U1, U2>&) = delete;
  template <class Alloc, class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, pair<U1, U2>&&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<pair<U1, U2>&&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<pair<U1, U2>&&>>) tuple(allocator_arg_t, const Alloc& a, pair<U1, U2>&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<pair<U1, U2>&&>(u), seq_of<pair<U1, U2>&&>{}) {}
  template <class Alloc, class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, pair<U1, U2>&&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<pair<U1, U2>&&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<pair<U1, U2>&&>>) tuple(allocator_arg_t, const Alloc& a, pair<U1, U2>&&) = delete;
  template <class Alloc, class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, const pair<U1, U2>&&> && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&&>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&&>>) tuple(allocator_arg_t, const Alloc& a, const pair<U1, U2>&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<const pair<U1, U2>&&>(u), seq_of<const pair<U1, U2>&&>{}) {}
  template <class Alloc, class U1, class U2>
    requires ycxx::detail::tuple_from_other<tuple, const pair<U1, U2>&&> && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&&>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<const pair<U1, U2>&&>>) tuple(allocator_arg_t, const Alloc& a, const pair<U1, U2>&&) = delete;
  template <class Alloc, class Src>
    requires ycxx::detail::tuple_from_other<tuple, Src> && (!ycxx::detail::is_pair_v<remove_cvref_t<Src>>) && (!ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<Src>>)
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<Src>>) tuple(allocator_arg_t, const Alloc& a, Src&& u)
      : tuple(static_cast<ycxx::detail::alloc_tag_t*>(nullptr), a, static_cast<Src&&>(u), seq_of<Src>{}) {}
  template <class Alloc, class Src>
    requires ycxx::detail::tuple_from_other<tuple, Src> && (!ycxx::detail::is_pair_v<remove_cvref_t<Src>>) && ycxx::detail::elems_dangle<tuple, ycxx::detail::get_types_t<Src>>
  constexpr explicit(!ycxx::detail::elems_convertible<tuple, ycxx::detail::get_types_t<Src>>) tuple(allocator_arg_t, const Alloc& a, Src&&) = delete;

  // ---- [tuple.assign] ----
  // When every element is a trivially assignable object, the defaulted operators keep tuple
  // trivially copyable; otherwise the element-wise (reference-assigning) versions apply.
  static constexpr bool trivial_copy_assign = ((is_trivially_copy_assignable_v<Types> && !is_reference_v<Types>) && ...);
  static constexpr bool trivial_move_assign = ((is_trivially_move_assignable_v<Types> && !is_reference_v<Types>) && ...);
  tuple& operator=(const tuple&)
    requires trivial_copy_assign
  = default;
  tuple& operator=(tuple&&)
    requires trivial_move_assign
  = default;
  constexpr tuple& operator=(const tuple& u)
    requires(!trivial_copy_assign) && (is_copy_assignable_v<Types> && ...)
  {
    assign_from(u, index_sequence_for<Types...>{});
    return *this;
  }
  constexpr const tuple& operator=(const tuple& u) const
    requires(is_copy_assignable_v<const Types> && ...)
  {
    assign_from(u, index_sequence_for<Types...>{});
    return *this;
  }
  constexpr tuple& operator=(tuple&& u) noexcept((is_nothrow_move_assignable_v<Types> && ...))
    requires(!trivial_move_assign) && (is_move_assignable_v<Types> && ...)
  {
    assign_from(static_cast<tuple&&>(u), index_sequence_for<Types...>{});
    return *this;
  }
  constexpr const tuple& operator=(tuple&& u) const
    requires(is_assignable_v<const Types&, Types> && ...)
  {
    assign_from(static_cast<tuple&&>(u), index_sequence_for<Types...>{});
    return *this;
  }
  template <class... UTypes>
    requires(sizeof...(UTypes) == N) && (is_assignable_v<Types&, const UTypes&> && ...)
  constexpr tuple& operator=(const tuple<UTypes...>& u) {
    assign_from(u, index_sequence_for<Types...>{});
    return *this;
  }
  template <class... UTypes>
    requires(sizeof...(UTypes) == N) && (is_assignable_v<const Types&, const UTypes&> && ...)
  constexpr const tuple& operator=(const tuple<UTypes...>& u) const {
    assign_from(u, index_sequence_for<Types...>{});
    return *this;
  }
  template <class... UTypes>
    requires(sizeof...(UTypes) == N) && (is_assignable_v<Types&, UTypes> && ...)
  constexpr tuple& operator=(tuple<UTypes...>&& u) {
    assign_from(static_cast<tuple<UTypes...>&&>(u), index_sequence_for<Types...>{});
    return *this;
  }
  template <class... UTypes>
    requires(sizeof...(UTypes) == N) && (is_assignable_v<const Types&, UTypes> && ...)
  constexpr const tuple& operator=(tuple<UTypes...>&& u) const {
    assign_from(static_cast<tuple<UTypes...>&&>(u), index_sequence_for<Types...>{});
    return *this;
  }
  template <class U1, class U2>
    requires(N == 2) && is_assignable_v<Types...[0] &, const U1&> && is_assignable_v<Types...[1] &, const U2&>
  constexpr tuple& operator=(const pair<U1, U2>& u) {
    ycxx::detail::leaf_get<0>(s_) = u.first;
    ycxx::detail::leaf_get<1>(s_) = u.second;
    return *this;
  }
  template <class U1, class U2>
    requires(N == 2) && is_assignable_v<const Types...[0] &, const U1&> &&
            is_assignable_v<const Types...[1] &, const U2&>
  constexpr const tuple& operator=(const pair<U1, U2>& u) const {
    ycxx::detail::leaf_get<0>(s_) = u.first;
    ycxx::detail::leaf_get<1>(s_) = u.second;
    return *this;
  }
  template <class U1, class U2>
    requires(N == 2) && is_assignable_v<Types...[0] &, U1> && is_assignable_v<Types...[1] &, U2>
  constexpr tuple& operator=(pair<U1, U2>&& u) {
    ycxx::detail::leaf_get<0>(s_) = static_cast<U1&&>(u.first);
    ycxx::detail::leaf_get<1>(s_) = static_cast<U2&&>(u.second);
    return *this;
  }
  template <class U1, class U2>
    requires(N == 2) && is_assignable_v<const Types...[0] &, U1> && is_assignable_v<const Types...[1] &, U2>
  constexpr const tuple& operator=(pair<U1, U2>&& u) const {
    ycxx::detail::leaf_get<0>(s_) = static_cast<U1&&>(u.first);
    ycxx::detail::leaf_get<1>(s_) = static_cast<U2&&>(u.second);
    return *this;
  }
  template <ycxx::detail::tuple_like UTuple>
    requires(!ycxx::detail::is_tuple_specialization<remove_cvref_t<UTuple>>) &&
            (!ycxx::detail::is_pair_v<remove_cvref_t<UTuple>>) && (!ycxx::detail::is_subrange<remove_cvref_t<UTuple>>) &&
            (tuple_size_v<remove_cvref_t<UTuple>> == N) &&
            ycxx::detail::elems_assignable<tuple, ycxx::detail::get_types_t<UTuple>>
  constexpr tuple& operator=(UTuple&& u) {
    assign_from(static_cast<UTuple&&>(u), index_sequence_for<Types...>{});
    return *this;
  }
  template <ycxx::detail::tuple_like UTuple>
    requires(!ycxx::detail::is_tuple_specialization<remove_cvref_t<UTuple>>) &&
            (!ycxx::detail::is_pair_v<remove_cvref_t<UTuple>>) && (!ycxx::detail::is_subrange<remove_cvref_t<UTuple>>) &&
            (tuple_size_v<remove_cvref_t<UTuple>> == N) &&
            ycxx::detail::elems_const_assignable<tuple, ycxx::detail::get_types_t<UTuple>>
  constexpr const tuple& operator=(UTuple&& u) const {
    assign_from(static_cast<UTuple&&>(u), index_sequence_for<Types...>{});
    return *this;
  }

  // ---- [tuple.swap] ----
  constexpr void swap(tuple& rhs) noexcept((is_nothrow_swappable_v<Types> && ...)) {
    [&]<size_t... I>(index_sequence<I...>) {
      ((void)ycxx::detail::swap_adl::do_swap(ycxx::detail::leaf_get<I>(s_), ycxx::detail::leaf_get<I>(rhs.s_)), ...);
    }(index_sequence_for<Types...>{});
  }
  constexpr void swap(const tuple& rhs) const noexcept((is_nothrow_swappable_v<const Types> && ...)) {
    [&]<size_t... I>(index_sequence<I...>) {
      ((void)ycxx::detail::swap_adl::do_swap(ycxx::detail::leaf_get<I>(s_), ycxx::detail::leaf_get<I>(rhs.s_)), ...);
    }(index_sequence_for<Types...>{});
  }
};

template <>
class tuple<> {
public:
  constexpr tuple() noexcept = default;
  template <class Alloc>
  constexpr tuple(allocator_arg_t, const Alloc&) noexcept {}
  template <class Alloc>
  constexpr tuple(allocator_arg_t, const Alloc&, const tuple&) noexcept {}
  template <ycxx::detail::tuple_like UTuple>
    requires ycxx::detail::different_from_<UTuple, tuple> && (tuple_size_v<remove_cvref_t<UTuple>> == 0) &&
             (!ycxx::detail::is_subrange<remove_cvref_t<UTuple>>)
  constexpr tuple(UTuple&&) noexcept {}
  template <class Alloc, ycxx::detail::tuple_like UTuple>
    requires ycxx::detail::different_from_<UTuple, tuple> && (tuple_size_v<remove_cvref_t<UTuple>> == 0) &&
             (!ycxx::detail::is_subrange<remove_cvref_t<UTuple>>)
  constexpr tuple(allocator_arg_t, const Alloc&, UTuple&&) noexcept {}
  tuple(const tuple&) = default;
  tuple& operator=(const tuple&) = default;
  constexpr const tuple& operator=(const tuple&) const noexcept { return *this; }
  template <ycxx::detail::tuple_like UTuple>
    requires ycxx::detail::different_from_<UTuple, tuple> && (tuple_size_v<remove_cvref_t<UTuple>> == 0)
  constexpr tuple& operator=(UTuple&&) noexcept {
    return *this;
  }
  template <ycxx::detail::tuple_like UTuple>
    requires ycxx::detail::different_from_<UTuple, tuple> && (tuple_size_v<remove_cvref_t<UTuple>> == 0)
  constexpr const tuple& operator=(UTuple&&) const noexcept {
    return *this;
  }
  constexpr void swap(tuple&) noexcept {}
  constexpr void swap(const tuple&) const noexcept {}
};

// ---- deduction guides ----
template <class... UTypes>
tuple(UTypes...) -> tuple<UTypes...>;
template <class T1, class T2>
tuple(pair<T1, T2>) -> tuple<T1, T2>;
template <class Alloc, class... UTypes>
tuple(allocator_arg_t, Alloc, UTypes...) -> tuple<UTypes...>;
template <class Alloc, class T1, class T2>
tuple(allocator_arg_t, Alloc, pair<T1, T2>) -> tuple<T1, T2>;
template <class Alloc, class... UTypes>
tuple(allocator_arg_t, Alloc, tuple<UTypes...>) -> tuple<UTypes...>;

// ---- [tuple.helper] ----
template <class... Types>
struct tuple_size<tuple<Types...>> : integral_constant<size_t, sizeof...(Types)> {};
template <size_t I, class... Types>
struct tuple_element<I, tuple<Types...>> {
  static_assert(I < sizeof...(Types), "tuple_element index out of range");
  using type = Types...[I];
};

// ---- [tuple.elem] ----
template <size_t I, class... Types>
constexpr tuple_element_t<I, tuple<Types...>>& get(tuple<Types...>& t) noexcept {
  static_assert(I < sizeof...(Types), "std::get: tuple index out of range");
  return ycxx::detail::leaf_get<I>(t.s_);
}
template <size_t I, class... Types>
constexpr const tuple_element_t<I, tuple<Types...>>& get(const tuple<Types...>& t) noexcept {
  static_assert(I < sizeof...(Types), "std::get: tuple index out of range");
  return ycxx::detail::leaf_get<I>(t.s_);
}
template <size_t I, class... Types>
constexpr tuple_element_t<I, tuple<Types...>>&& get(tuple<Types...>&& t) noexcept {
  return static_cast<tuple_element_t<I, tuple<Types...>>&&>(std::get<I>(t));
}
template <size_t I, class... Types>
constexpr const tuple_element_t<I, tuple<Types...>>&& get(const tuple<Types...>&& t) noexcept {
  return static_cast<const tuple_element_t<I, tuple<Types...>>&&>(std::get<I>(t));
}

} // namespace std

namespace ycxx::detail {
template <class T, class... Types>
consteval std::size_t type_index() {
  constexpr bool hits[] = {__is_same(T, Types)..., false};
  std::size_t found = sizeof...(Types), count = 0;
  for (std::size_t i = 0; i < sizeof...(Types); ++i)
    if (hits[i]) {
      found = i;
      ++count;
    }
  return count == 1 ? found : sizeof...(Types);
}
} // namespace ycxx::detail

namespace std {

template <class T, class... Types>
  requires(ycxx::detail::type_index<T, Types...>() < sizeof...(Types))
constexpr T& get(tuple<Types...>& t) noexcept {
  return std::get<ycxx::detail::type_index<T, Types...>()>(t);
}
template <class T, class... Types>
  requires(ycxx::detail::type_index<T, Types...>() < sizeof...(Types))
constexpr T&& get(tuple<Types...>&& t) noexcept {
  return std::get<ycxx::detail::type_index<T, Types...>()>(static_cast<tuple<Types...>&&>(t));
}
template <class T, class... Types>
  requires(ycxx::detail::type_index<T, Types...>() < sizeof...(Types))
constexpr const T& get(const tuple<Types...>& t) noexcept {
  return std::get<ycxx::detail::type_index<T, Types...>()>(t);
}
template <class T, class... Types>
  requires(ycxx::detail::type_index<T, Types...>() < sizeof...(Types))
constexpr const T&& get(const tuple<Types...>&& t) noexcept {
  return std::get<ycxx::detail::type_index<T, Types...>()>(static_cast<const tuple<Types...>&&>(t));
}

// ---- ignore ----
} // namespace std

namespace ycxx::detail {
struct ignore_type {
  constexpr const ignore_type& operator=(const auto&) const noexcept { return *this; }
};
} // namespace ycxx::detail

namespace std {
inline constexpr ycxx::detail::ignore_type ignore;

// ---- [tuple.creation] ----
template <class... TTypes>
constexpr tuple<unwrap_ref_decay_t<TTypes>...> make_tuple(TTypes&&... t) {
  return tuple<unwrap_ref_decay_t<TTypes>...>(static_cast<TTypes&&>(t)...);
}
template <class... TTypes>
constexpr tuple<TTypes&&...> forward_as_tuple(TTypes&&... t) noexcept {
  return tuple<TTypes&&...>(static_cast<TTypes&&>(t)...);
}
template <class... TTypes>
constexpr tuple<TTypes&...> tie(TTypes&... t) noexcept {
  return tuple<TTypes&...>(t...);
}

} // namespace std

namespace ycxx::detail {
// tuple_cat: (outer, inner) index pairs for the flattened element list.
template <class... Tuples>
struct cat_plan {
  static constexpr std::size_t sizes[] = {std::tuple_size_v<std::remove_cvref_t<Tuples>>..., 0};
  static constexpr std::size_t total = (std::tuple_size_v<std::remove_cvref_t<Tuples>> + ... + 0);
  struct entry {
    std::size_t outer, inner;
  };
  static constexpr auto entries = [] {
    struct arr {
      entry e[total ? total : 1];
    } r{};
    std::size_t k = 0;
    for (std::size_t o = 0; o < sizeof...(Tuples); ++o)
      for (std::size_t i = 0; i < sizes[o]; ++i)
        r.e[k++] = {o, i};
    return r;
  }();
};
template <class Tup, std::size_t I>
using cat_elem = std::tuple_element_t<I, std::remove_cvref_t<Tup>>;

template <class Plan, class FwdTuple, class... Tuples, std::size_t... K>
constexpr auto tuple_cat_impl(FwdTuple&& fwd, std::index_sequence<K...>) {
  using result = std::tuple<cat_elem<Tuples...[Plan::entries.e[K].outer], Plan::entries.e[K].inner>...>;
  return result(get<Plan::entries.e[K].inner>(
      std::get<Plan::entries.e[K].outer>(static_cast<FwdTuple&&>(fwd)))...);
}
} // namespace ycxx::detail

namespace std {

template <ycxx::detail::tuple_like... Tuples>
constexpr auto tuple_cat(Tuples&&... tpls) {
  using plan = ycxx::detail::cat_plan<Tuples...>;
  return ycxx::detail::tuple_cat_impl<plan, tuple<Tuples&&...>, Tuples...>(
      std::forward_as_tuple(static_cast<Tuples&&>(tpls)...), make_index_sequence<plan::total>{});
}

// ---- [tuple.apply] ----
template <class F, ycxx::detail::tuple_like Tuple>
  requires is_applicable_v<F, Tuple>
constexpr apply_result_t<F, Tuple> apply(F&& f, Tuple&& t) noexcept(is_nothrow_applicable_v<F, Tuple>) {
  return [&]<size_t... I>(index_sequence<I...>) -> apply_result_t<F, Tuple> {
    return ycxx::detail::invoke(static_cast<F&&>(f), get<I>(static_cast<Tuple&&>(t))...);
  }(ycxx::detail::tuple_indices<Tuple>{});
}

} // namespace std

namespace ycxx::detail {
template <class T, class Tuple>
consteval bool make_from_tuple_dangles() {
  if constexpr (std::tuple_size_v<std::remove_reference_t<Tuple>> == 1)
    return std::reference_constructs_from_temporary_v<T, decltype(get<0>(std::declval<Tuple>()))>;
  else
    return false;
}
} // namespace ycxx::detail

namespace std {
template <class T, ycxx::detail::tuple_like Tuple>
constexpr T make_from_tuple(Tuple&& t) {
  static_assert(!ycxx::detail::make_from_tuple_dangles<T, Tuple>(),
                "std::make_from_tuple: would bind a reference to a temporary");
  return [&]<size_t... I>(index_sequence<I...>) -> T {
    static_assert(is_constructible_v<T, decltype(get<I>(declval<Tuple>()))...>,
                  "std::make_from_tuple: T is not constructible from the tuple's elements");
    return T(get<I>(static_cast<Tuple&&>(t))...);
  }(ycxx::detail::tuple_indices<Tuple>{});
}

// ---- [tuple.rel] ----
} // namespace std

namespace ycxx::detail {
template <class T, class U, std::size_t... I>
consteval bool tuple_eq_ok(std::index_sequence<I...>*) {
  return (requires(const T& t, const U& u) {
    { get<I>(t) == get<I>(u) } -> boolean_testable;
  } && ...);
}
template <class T, class U>
concept tuple_eq_comparable = std::tuple_size_v<T> == std::tuple_size_v<U> &&
                              tuple_eq_ok<T, U>(static_cast<tuple_indices<T>*>(nullptr));

template <class T, class U, std::size_t... I>
auto tuple_cmp_cat(std::index_sequence<I...>*)
    -> std::common_comparison_category_t<synth_three_way_result<std::tuple_element_t<I, T>, std::tuple_element_t<I, U>>...>;

template <class T, class U>
using tuple_cmp_result = decltype(tuple_cmp_cat<T, U>(static_cast<tuple_indices<T>*>(nullptr)));
} // namespace ycxx::detail

namespace std {

template <class... TTypes, class... UTypes>
  requires ycxx::detail::tuple_eq_comparable<tuple<TTypes...>, tuple<UTypes...>>
constexpr bool operator==(const tuple<TTypes...>& t, const tuple<UTypes...>& u) {
  return [&]<size_t... I>(index_sequence<I...>) {
    return (static_cast<bool>(std::get<I>(t) == std::get<I>(u)) && ...);
  }(index_sequence_for<TTypes...>{});
}
template <class... TTypes, ycxx::detail::tuple_like UTuple>
  requires(!ycxx::detail::is_tuple_specialization<UTuple>) &&
          ycxx::detail::tuple_eq_comparable<tuple<TTypes...>, UTuple>
constexpr bool operator==(const tuple<TTypes...>& t, const UTuple& u) {
  return [&]<size_t... I>(index_sequence<I...>) {
    return (static_cast<bool>(std::get<I>(t) == get<I>(u)) && ...);
  }(index_sequence_for<TTypes...>{});
}

template <class... TTypes, class... UTypes>
  requires(sizeof...(TTypes) == sizeof...(UTypes))
constexpr ycxx::detail::tuple_cmp_result<tuple<TTypes...>, tuple<UTypes...>> operator<=>(const tuple<TTypes...>& t,
                                                                                        const tuple<UTypes...>& u) {
  using R = ycxx::detail::tuple_cmp_result<tuple<TTypes...>, tuple<UTypes...>>;
  R r = R::equivalent;
  [&]<size_t... I>(index_sequence<I...>) {
    (void)((r = ycxx::detail::synth_three_way(std::get<I>(t), std::get<I>(u)), r == 0) && ...);
  }(index_sequence_for<TTypes...>{});
  return r;
}
template <class... TTypes, ycxx::detail::tuple_like UTuple>
  requires(!ycxx::detail::is_tuple_specialization<UTuple>) && (sizeof...(TTypes) == tuple_size_v<UTuple>)
constexpr ycxx::detail::tuple_cmp_result<tuple<TTypes...>, UTuple> operator<=>(const tuple<TTypes...>& t,
                                                                              const UTuple& u) {
  using R = ycxx::detail::tuple_cmp_result<tuple<TTypes...>, UTuple>;
  R r = R::equivalent;
  [&]<size_t... I>(index_sequence<I...>) {
    (void)((r = ycxx::detail::synth_three_way(std::get<I>(t), get<I>(u)), r == 0) && ...);
  }(index_sequence_for<TTypes...>{});
  return r;
}

// ---- [tuple.traits], [tuple.special] ----
template <class... Types, class Alloc>
struct uses_allocator<tuple<Types...>, Alloc> : true_type {};

template <class... Types>
  requires(is_swappable_v<Types> && ...)
constexpr void swap(tuple<Types...>& x, tuple<Types...>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}
template <class... Types>
  requires(is_swappable_v<const Types> && ...)
constexpr void swap(const tuple<Types...>& x, const tuple<Types...>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [tuple.common.ref] ----
} // namespace std

namespace ycxx::detail {
template <class TT, class UT, template <class> class TQ, template <class> class UQ, class Seq>
struct tuple_common_ref;
template <class TT, class UT, template <class> class TQ, template <class> class UQ, std::size_t... I>
  requires requires {
    typename std::tuple<std::common_reference_t<TQ<std::tuple_element_t<I, TT>>, UQ<std::tuple_element_t<I, UT>>>...>;
  }
struct tuple_common_ref<TT, UT, TQ, UQ, std::index_sequence<I...>> {
  using type = std::tuple<std::common_reference_t<TQ<std::tuple_element_t<I, TT>>, UQ<std::tuple_element_t<I, UT>>>...>;
};
template <class TT, class UT, class Seq>
struct tuple_common_type;
template <class TT, class UT, std::size_t... I>
  requires requires { typename std::tuple<std::common_type_t<std::tuple_element_t<I, TT>, std::tuple_element_t<I, UT>>...>; }
struct tuple_common_type<TT, UT, std::index_sequence<I...>> {
  using type = std::tuple<std::common_type_t<std::tuple_element_t<I, TT>, std::tuple_element_t<I, UT>>...>;
};
template <class TT, class UT>
concept tuple_common_candidates =
    (is_tuple_specialization<TT> || is_tuple_specialization<UT>) && __is_same(TT, std::decay_t<TT>) &&
    __is_same(UT, std::decay_t<UT>) && std::tuple_size_v<TT> == std::tuple_size_v<UT>;
} // namespace ycxx::detail

namespace std {
template <ycxx::detail::tuple_like TTuple, ycxx::detail::tuple_like UTuple, template <class> class TQual,
          template <class> class UQual>
  requires ycxx::detail::tuple_common_candidates<TTuple, UTuple> &&
           requires { typename ycxx::detail::tuple_common_ref<TTuple, UTuple, TQual, UQual, ycxx::detail::tuple_indices<TTuple>>::type; }
struct basic_common_reference<TTuple, UTuple, TQual, UQual>
    : ycxx::detail::tuple_common_ref<TTuple, UTuple, TQual, UQual, ycxx::detail::tuple_indices<TTuple>> {};

template <ycxx::detail::tuple_like TTuple, ycxx::detail::tuple_like UTuple>
  requires ycxx::detail::tuple_common_candidates<TTuple, UTuple> &&
           requires { typename ycxx::detail::tuple_common_type<TTuple, UTuple, ycxx::detail::tuple_indices<TTuple>>::type; }
struct common_type<TTuple, UTuple>
    : ycxx::detail::tuple_common_type<TTuple, UTuple, ycxx::detail::tuple_indices<TTuple>> {};
} // namespace std

// =============================================================================================
// [allocator.uses.construction]
// =============================================================================================
namespace std {

template <class T, class Alloc, class... Args>
  requires(!ycxx::detail::is_pair_v<remove_cv_t<T>>)
constexpr auto uses_allocator_construction_args(const Alloc& alloc, Args&&... args) noexcept {
  if constexpr (!uses_allocator_v<remove_cv_t<T>, Alloc> && is_constructible_v<T, Args...>) {
    return std::forward_as_tuple(static_cast<Args&&>(args)...);
  } else if constexpr (uses_allocator_v<remove_cv_t<T>, Alloc> && is_constructible_v<T, allocator_arg_t, const Alloc&, Args...>) {
    return tuple<allocator_arg_t, const Alloc&, Args&&...>(allocator_arg, alloc, static_cast<Args&&>(args)...);
  } else if constexpr (uses_allocator_v<remove_cv_t<T>, Alloc> && is_constructible_v<T, Args..., const Alloc&>) {
    return std::forward_as_tuple(static_cast<Args&&>(args)..., alloc);
  } else {
    static_assert(ycxx::detail::always_false<T>, "uses-allocator construction: T is not constructible");
  }
}

template <class T, class Alloc, class Tuple1, class Tuple2>
  requires ycxx::detail::is_pair_v<remove_cv_t<T>>
constexpr auto uses_allocator_construction_args(const Alloc& alloc, piecewise_construct_t, Tuple1&& x,
                                                Tuple2&& y) noexcept {
  using T1 = typename remove_cv_t<T>::first_type;
  using T2 = typename remove_cv_t<T>::second_type;
  return std::make_tuple(
      piecewise_construct,
      std::apply([&alloc](auto&&... a) { return std::uses_allocator_construction_args<T1>(alloc, static_cast<decltype(a)&&>(a)...); },
                 static_cast<Tuple1&&>(x)),
      std::apply([&alloc](auto&&... a) { return std::uses_allocator_construction_args<T2>(alloc, static_cast<decltype(a)&&>(a)...); },
                 static_cast<Tuple2&&>(y)));
}
template <class T, class Alloc>
  requires ycxx::detail::is_pair_v<remove_cv_t<T>>
constexpr auto uses_allocator_construction_args(const Alloc& alloc) noexcept {
  return std::uses_allocator_construction_args<T>(alloc, piecewise_construct, tuple<>{}, tuple<>{});
}
template <class T, class Alloc, class U, class V>
  requires ycxx::detail::is_pair_v<remove_cv_t<T>>
constexpr auto uses_allocator_construction_args(const Alloc& alloc, U&& u, V&& v) noexcept {
  return std::uses_allocator_construction_args<T>(alloc, piecewise_construct,
                                                  std::forward_as_tuple(static_cast<U&&>(u)),
                                                  std::forward_as_tuple(static_cast<V&&>(v)));
}
template <class T, class Alloc, class U, class V>
  requires ycxx::detail::is_pair_v<remove_cv_t<T>>
constexpr auto uses_allocator_construction_args(const Alloc& alloc, pair<U, V>& pr) noexcept {
  return std::uses_allocator_construction_args<T>(alloc, piecewise_construct, std::forward_as_tuple(pr.first),
                                                  std::forward_as_tuple(pr.second));
}
template <class T, class Alloc, class U, class V>
  requires ycxx::detail::is_pair_v<remove_cv_t<T>>
constexpr auto uses_allocator_construction_args(const Alloc& alloc, const pair<U, V>& pr) noexcept {
  return std::uses_allocator_construction_args<T>(alloc, piecewise_construct, std::forward_as_tuple(pr.first),
                                                  std::forward_as_tuple(pr.second));
}
template <class T, class Alloc, class U, class V>
  requires ycxx::detail::is_pair_v<remove_cv_t<T>>
constexpr auto uses_allocator_construction_args(const Alloc& alloc, pair<U, V>&& pr) noexcept {
  return std::uses_allocator_construction_args<T>(alloc, piecewise_construct,
                                                  std::forward_as_tuple(static_cast<U&&>(pr.first)),
                                                  std::forward_as_tuple(static_cast<V&&>(pr.second)));
}
template <class T, class Alloc, class U, class V>
  requires ycxx::detail::is_pair_v<remove_cv_t<T>>
constexpr auto uses_allocator_construction_args(const Alloc& alloc, const pair<U, V>&& pr) noexcept {
  return std::uses_allocator_construction_args<T>(alloc, piecewise_construct,
                                                  std::forward_as_tuple(static_cast<const U&&>(pr.first)),
                                                  std::forward_as_tuple(static_cast<const V&&>(pr.second)));
}
template <class T, class Alloc, ycxx::detail::pair_like P>
  requires ycxx::detail::is_pair_v<remove_cv_t<T>> && (!ycxx::detail::is_subrange<remove_cvref_t<P>>) &&
           (!ycxx::detail::is_pair_v<remove_cvref_t<P>>)
constexpr auto uses_allocator_construction_args(const Alloc& alloc, P&& p) noexcept {
  return std::uses_allocator_construction_args<T>(alloc, piecewise_construct,
                                                  std::forward_as_tuple(get<0>(static_cast<P&&>(p))),
                                                  std::forward_as_tuple(get<1>(static_cast<P&&>(p))));
}

} // namespace std

namespace std {
template <class T, class Alloc, class... Args>
constexpr T make_obj_using_allocator(const Alloc& alloc, Args&&... args);
} // namespace std

namespace ycxx::detail {
// pair-convert: a type convertible to any pair, for the final uses_allocator_construction_args
// overload ([allocator.uses.construction]/17).
template <class T, class Alloc, class U>
class pair_converter {
  using pair_type = std::remove_cv_t<T>;
  const Alloc& alloc_;
  U& u_;

  constexpr auto do_construct(const pair_type& p) const { return std::make_obj_using_allocator<pair_type>(alloc_, p); }
  constexpr auto do_construct(pair_type&& p) const {
    return std::make_obj_using_allocator<pair_type>(alloc_, static_cast<pair_type&&>(p));
  }

public:
  constexpr pair_converter(const Alloc& a, U& u) noexcept : alloc_(a), u_(u) {}
  constexpr operator pair_type() const { return do_construct(static_cast<U&&>(u_)); }
};
// FUN(u) from [allocator.uses.construction]/19: well-formed for pair and classes derived from it.
template <class A, class B>
void pair_fun(const std::pair<A, B>&);
template <class U>
concept pair_fun_callable = requires(U&& u) { pair_fun(static_cast<U&&>(u)); };
} // namespace ycxx::detail

namespace std {
template <class T, class Alloc, class U>
  requires ycxx::detail::is_pair_v<remove_cv_t<T>> &&
           (ycxx::detail::is_subrange<remove_cvref_t<U>> ||
            (!ycxx::detail::pair_like<U> && !ycxx::detail::pair_fun_callable<U>))
constexpr auto uses_allocator_construction_args(const Alloc& alloc, U&& u) noexcept {
  return std::make_tuple(ycxx::detail::pair_converter<T, Alloc, U>(alloc, u));
}

template <class T, class Alloc, class... Args>
constexpr T make_obj_using_allocator(const Alloc& alloc, Args&&... args) {
  return std::make_from_tuple<T>(std::uses_allocator_construction_args<T>(alloc, static_cast<Args&&>(args)...));
}

template <class T, class Alloc, class... Args>
constexpr T* uninitialized_construct_using_allocator(T* p, const Alloc& alloc, Args&&... args) {
  return std::apply(
      [&]<class... U>(U&&... xs) { return std::construct_at(p, static_cast<U&&>(xs)...); },
      std::uses_allocator_construction_args<T>(alloc, static_cast<Args&&>(args)...));
}

} // namespace std

namespace ycxx::detail {
template <class T, class Alloc, class... Args>
constexpr T make_using_alloc(const Alloc& a, Args&&... args) {
  if constexpr (std::is_reference_v<T>)
    return static_cast<T>(static_cast<Args...[0] &&>(args...[0]));
  else
    return std::make_obj_using_allocator<T>(a, static_cast<Args&&>(args)...);
}
} // namespace ycxx::detail
