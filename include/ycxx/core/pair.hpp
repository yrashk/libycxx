// libycxx core: std::pair ([pairs]).
#pragma once

#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Tp>
void __implicit_default_init_test(const _Tp&);
// Copy-list-initialization from {} (i.e. the default constructor is not explicit).
template <class _Tp>
concept __implicitly_default_constructible = requires { __implicit_default_init_test<_Tp>({}); };

template <class _Tp>
inline constexpr bool __is_pair_v = false;
template <class _Ap, class _Bp>
inline constexpr bool __is_pair_v<std::pair<_Ap, _Bp>> = true;

// ranges::subrange is excluded from pair's pair-like constructor ([pairs.pair]/14).
template <class _Tp>
inline constexpr bool __is_subrange = false;

template <class _T1, class _T2, class _Ap, class _Bp>
concept __pair_constructible = __is_constructible(_T1, _Ap) && __is_constructible(_T2, _Bp);
template <class _T1, class _T2, class _Ap, class _Bp>
concept __pair_dangles = __pair_constructible<_T1, _T2, _Ap, _Bp> &&
                       (__reference_constructs_from_temporary(_T1, _Ap) || __reference_constructs_from_temporary(_T2, _Bp));
template <class _T1, class _T2, class _Ap, class _Bp>
concept __pair_convertible = __is_convertible(_Ap, _T1) && __is_convertible(_Bp, _T2);

template <class _Pp, class _Pair>
concept __pair_like_not_pair = __pair_like<_Pp> && !__is_same(__remove_cvref(_Pp), _Pair);

// decltype(get<I>(FWD(p))) for a pair-like P.
template <std::size_t _Ip, class _Pp>
using __pair_like_get_t = decltype(get<_Ip>(static_cast<_Pp (*)()>(nullptr)()));

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _T1, class _T2>
struct pair {
  using first_type = _T1;
  using second_type = _T2;

  _T1 first;
  _T2 second;

  pair(const pair&) = default;
  pair(pair&&) = default;

  // A template, so that the explicit-specifier is evaluated only when the constructor is used:
  // pair<int, T> may be instantiated while T is still incomplete ([vector.overview]/4 allows
  // struct T { vector<pair<int, T>> v; }).
  template <class _U1 = _T1, class _U2 = _T2>
    requires(std::is_constructible_v<_U1> && std::is_constructible_v<_U2>)
  constexpr explicit(!__ycxx::__detail::__implicitly_default_constructible<_U1> ||
                     !__ycxx::__detail::__implicitly_default_constructible<_U2>) pair()
      : first(), second() {}

  constexpr explicit(!std::is_convertible_v<const _T1&, _T1> || !std::is_convertible_v<const _T2&, _T2>)
      pair(const _T1& __x, const _T2& y)
    requires(std::is_constructible_v<_T1, const _T1&> && std::is_constructible_v<_T2, const _T2&>)
      : first(__x), second(y) {}

  // Converting constructors. Each comes with a deleted twin, selected (by constraint
  // subsumption) when a reference member would bind to a temporary.
  template <class _U1 = _T1, class _U2 = _T2>
    requires __ycxx::__detail::__pair_constructible<_T1, _T2, _U1&&, _U2&&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, _U1&&, _U2&&>) pair(_U1&& __x, _U2&& y)
      : first(static_cast<_U1&&>(__x)), second(static_cast<_U2&&>(y)) {}
  template <class _U1 = _T1, class _U2 = _T2>
    requires __ycxx::__detail::__pair_dangles<_T1, _T2, _U1&&, _U2&&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, _U1&&, _U2&&>) pair(_U1&&, _U2&&) = delete;

  template <class _U1, class _U2>
    requires __ycxx::__detail::__pair_constructible<_T1, _T2, _U1&, _U2&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, _U1&, _U2&>) pair(pair<_U1, _U2>& p)
      : first(p.first), second(p.second) {}
  template <class _U1, class _U2>
    requires __ycxx::__detail::__pair_dangles<_T1, _T2, _U1&, _U2&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, _U1&, _U2&>) pair(pair<_U1, _U2>&) = delete;

  template <class _U1, class _U2>
    requires __ycxx::__detail::__pair_constructible<_T1, _T2, const _U1&, const _U2&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, const _U1&, const _U2&>) pair(const pair<_U1, _U2>& p)
      : first(p.first), second(p.second) {}
  template <class _U1, class _U2>
    requires __ycxx::__detail::__pair_dangles<_T1, _T2, const _U1&, const _U2&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, const _U1&, const _U2&>)
      pair(const pair<_U1, _U2>&) = delete;

  template <class _U1, class _U2>
    requires __ycxx::__detail::__pair_constructible<_T1, _T2, _U1&&, _U2&&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, _U1&&, _U2&&>) pair(pair<_U1, _U2>&& p)
      : first(static_cast<_U1&&>(p.first)), second(static_cast<_U2&&>(p.second)) {}
  template <class _U1, class _U2>
    requires __ycxx::__detail::__pair_dangles<_T1, _T2, _U1&&, _U2&&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, _U1&&, _U2&&>) pair(pair<_U1, _U2>&&) = delete;

  template <class _U1, class _U2>
    requires __ycxx::__detail::__pair_constructible<_T1, _T2, const _U1&&, const _U2&&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, const _U1&&, const _U2&&>)
      pair(const pair<_U1, _U2>&& p)
      : first(static_cast<const _U1&&>(p.first)), second(static_cast<const _U2&&>(p.second)) {}
  template <class _U1, class _U2>
    requires __ycxx::__detail::__pair_dangles<_T1, _T2, const _U1&&, const _U2&&>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, const _U1&&, const _U2&&>)
      pair(const pair<_U1, _U2>&&) = delete;

  // pair-like, with the deleted twin of [pairs.pair]/17.
  template <__ycxx::__detail::__pair_like_not_pair<pair> _Pp>
    requires(!__ycxx::__detail::__is_subrange<remove_cvref_t<_Pp>>) &&
            __ycxx::__detail::__pair_constructible<_T1, _T2, __ycxx::__detail::__pair_like_get_t<0, _Pp>,
                                             __ycxx::__detail::__pair_like_get_t<1, _Pp>> &&
            (!__ycxx::__detail::__pair_dangles<_T1, _T2, __ycxx::__detail::__pair_like_get_t<0, _Pp>,
                                              __ycxx::__detail::__pair_like_get_t<1, _Pp>>)
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, __ycxx::__detail::__pair_like_get_t<0, _Pp>,
                                                     __ycxx::__detail::__pair_like_get_t<1, _Pp>>) pair(_Pp&& p)
      : first(get<0>(static_cast<_Pp&&>(p))), second(get<1>(static_cast<_Pp&&>(p))) {}
  template <__ycxx::__detail::__pair_like_not_pair<pair> _Pp>
    requires(!__ycxx::__detail::__is_subrange<remove_cvref_t<_Pp>>) &&
            __ycxx::__detail::__pair_dangles<_T1, _T2, __ycxx::__detail::__pair_like_get_t<0, _Pp>, __ycxx::__detail::__pair_like_get_t<1, _Pp>>
  constexpr explicit(!__ycxx::__detail::__pair_convertible<_T1, _T2, __ycxx::__detail::__pair_like_get_t<0, _Pp>,
                                                     __ycxx::__detail::__pair_like_get_t<1, _Pp>>) pair(_Pp&&) = delete;

  // piecewise
  template <class... _Args1, class... _Args2>
  constexpr pair(piecewise_construct_t, tuple<_Args1...> __a1, tuple<_Args2...> __a2)
      : pair(__a1, __a2, index_sequence_for<_Args1...>{}, index_sequence_for<_Args2...>{}) {}

  // --- assignment ---
  constexpr pair& operator=(const pair& p)
    requires(std::is_assignable_v<_T1&, const _T1&> && std::is_assignable_v<_T2&, const _T2&>)
  {
    first = p.first;
    second = p.second;
    return *this;
  }
  constexpr const pair& operator=(const pair& p) const
    requires(std::is_assignable_v<const _T1&, const _T1&> && std::is_assignable_v<const _T2&, const _T2&>)
  {
    first = p.first;
    second = p.second;
    return *this;
  }
  template <class _U1, class _U2>
    requires(std::is_assignable_v<_T1&, const _U1&> && std::is_assignable_v<_T2&, const _U2&>)
  constexpr pair& operator=(const pair<_U1, _U2>& p) {
    first = p.first;
    second = p.second;
    return *this;
  }
  template <class _U1, class _U2>
    requires(std::is_assignable_v<const _T1&, const _U1&> && std::is_assignable_v<const _T2&, const _U2&>)
  constexpr const pair& operator=(const pair<_U1, _U2>& p) const {
    first = p.first;
    second = p.second;
    return *this;
  }
  constexpr pair& operator=(pair&& p) noexcept(std::is_nothrow_assignable_v<_T1&, _T1 &&> &&
                                               std::is_nothrow_assignable_v<_T2&, _T2 &&>)
    requires(std::is_assignable_v<_T1&, _T1 &&> && std::is_assignable_v<_T2&, _T2 &&>)
  {
    first = static_cast<_T1&&>(p.first);
    second = static_cast<_T2&&>(p.second);
    return *this;
  }
  constexpr const pair& operator=(pair&& p) const
    requires(std::is_assignable_v<const _T1&, _T1 &&> && std::is_assignable_v<const _T2&, _T2 &&>)
  {
    first = static_cast<_T1&&>(p.first);
    second = static_cast<_T2&&>(p.second);
    return *this;
  }
  template <class _U1, class _U2>
    requires(std::is_assignable_v<_T1&, _U1 &&> && std::is_assignable_v<_T2&, _U2 &&>)
  constexpr pair& operator=(pair<_U1, _U2>&& p) {
    first = static_cast<_U1&&>(p.first);
    second = static_cast<_U2&&>(p.second);
    return *this;
  }
  template <class _U1, class _U2>
    requires(std::is_assignable_v<const _T1&, _U1 &&> && std::is_assignable_v<const _T2&, _U2 &&>)
  constexpr const pair& operator=(pair<_U1, _U2>&& p) const {
    first = static_cast<_U1&&>(p.first);
    second = static_cast<_U2&&>(p.second);
    return *this;
  }
  template <__ycxx::__detail::__pair_like_not_pair<pair> _Pp>
    requires(!__ycxx::__detail::__is_subrange<remove_cvref_t<_Pp>>) && requires(pair& __self, _Pp&& p) {
      __self.first = get<0>(static_cast<_Pp&&>(p));
      __self.second = get<1>(static_cast<_Pp&&>(p));
    }
  constexpr pair& operator=(_Pp&& p) {
    first = get<0>(static_cast<_Pp&&>(p));
    second = get<1>(static_cast<_Pp&&>(p));
    return *this;
  }
  template <__ycxx::__detail::__pair_like_not_pair<pair> _Pp>
    requires(!__ycxx::__detail::__is_subrange<remove_cvref_t<_Pp>>) && requires(const pair& __self, _Pp&& p) {
      __self.first = get<0>(static_cast<_Pp&&>(p));
      __self.second = get<1>(static_cast<_Pp&&>(p));
    }
  constexpr const pair& operator=(_Pp&& p) const {
    first = get<0>(static_cast<_Pp&&>(p));
    second = get<1>(static_cast<_Pp&&>(p));
    return *this;
  }

  // --- swap ---
  constexpr void swap(pair& p) noexcept(is_nothrow_swappable_v<_T1> && is_nothrow_swappable_v<_T2>) {
    __ycxx::__detail::__swap_adl::__do_swap(first, p.first);
    __ycxx::__detail::__swap_adl::__do_swap(second, p.second);
  }
  constexpr void swap(const pair& p) const
      noexcept(is_nothrow_swappable_v<const _T1> && is_nothrow_swappable_v<const _T2>) {
    __ycxx::__detail::__swap_adl::__do_swap(first, p.first);
    __ycxx::__detail::__swap_adl::__do_swap(second, p.second);
  }

private:
  template <class _A1, class _A2, size_t... _I1, size_t... _I2>
  constexpr pair(_A1& __a1, _A2& __a2, index_sequence<_I1...>, index_sequence<_I2...>)
      : first(get<_I1>(static_cast<_A1&&>(__a1))...), second(get<_I2>(static_cast<_A2&&>(__a2))...) {}
};

template <class _T1, class _T2>
pair(_T1, _T2) -> pair<_T1, _T2>;

// --- comparison ---
template <class _T1, class _T2, class _U1, class _U2>
  requires requires(const pair<_T1, _T2>& __x, const pair<_U1, _U2>& y) {
    { __x.first == y.first } -> __ycxx::__detail::__boolean_testable;
    { __x.second == y.second } -> __ycxx::__detail::__boolean_testable;
  }
constexpr bool operator==(const pair<_T1, _T2>& __x, const pair<_U1, _U2>& y) {
  return __x.first == y.first && __x.second == y.second;
}

template <class _T1, class _T2, class _U1, class _U2>
constexpr common_comparison_category_t<__ycxx::__detail::__synth_three_way_result<_T1, _U1>,
                                       __ycxx::__detail::__synth_three_way_result<_T2, _U2>>
operator<=>(const pair<_T1, _T2>& __x, const pair<_U1, _U2>& y) {
  if (auto c = __ycxx::__detail::__synth_three_way(__x.first, y.first); c != 0)
    return c;
  return __ycxx::__detail::__synth_three_way(__x.second, y.second);
}

template <class _T1, class _T2>
  requires(is_swappable_v<_T1> && is_swappable_v<_T2>)
constexpr void swap(pair<_T1, _T2>& __x, pair<_T1, _T2>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}
template <class _T1, class _T2>
  requires(is_swappable_v<const _T1> && is_swappable_v<const _T2>)
constexpr void swap(const pair<_T1, _T2>& __x, const pair<_T1, _T2>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

template <class _T1, class _T2>
constexpr pair<unwrap_ref_decay_t<_T1>, unwrap_ref_decay_t<_T2>> make_pair(_T1&& __x, _T2&& y) {
  return pair<unwrap_ref_decay_t<_T1>, unwrap_ref_decay_t<_T2>>(static_cast<_T1&&>(__x), static_cast<_T2&&>(y));
}

// --- common_type / basic_common_reference ---
template <class _T1, class _T2, class _U1, class _U2, template <class> class _TQual, template <class> class _UQual>
  requires requires {
    typename pair<common_reference_t<_TQual<_T1>, _UQual<_U1>>, common_reference_t<_TQual<_T2>, _UQual<_U2>>>;
  }
struct basic_common_reference<pair<_T1, _T2>, pair<_U1, _U2>, _TQual, _UQual> {
  using type = pair<common_reference_t<_TQual<_T1>, _UQual<_U1>>, common_reference_t<_TQual<_T2>, _UQual<_U2>>>;
};
template <class _T1, class _T2, class _U1, class _U2>
  requires requires { typename pair<common_type_t<_T1, _U1>, common_type_t<_T2, _U2>>; }
struct common_type<pair<_T1, _T2>, pair<_U1, _U2>> {
  using type = pair<common_type_t<_T1, _U1>, common_type_t<_T2, _U2>>;
};

// --- tuple protocol ---
template <class _T1, class _T2>
struct tuple_size<pair<_T1, _T2>> : integral_constant<size_t, 2> {};
template <size_t _Ip, class _T1, class _T2>
struct tuple_element<_Ip, pair<_T1, _T2>> {
  static_assert(_Ip < 2, "pair index out of range");
  using type = conditional_t<_Ip == 0, _T1, _T2>;
};

template <size_t _Ip, class _T1, class _T2>
  requires(_Ip < 2)
constexpr tuple_element_t<_Ip, pair<_T1, _T2>>& get(pair<_T1, _T2>& p) noexcept {
  if constexpr (_Ip == 0)
    return p.first;
  else
    return p.second;
}
template <size_t _Ip, class _T1, class _T2>
  requires(_Ip < 2)
constexpr const tuple_element_t<_Ip, pair<_T1, _T2>>& get(const pair<_T1, _T2>& p) noexcept {
  if constexpr (_Ip == 0)
    return p.first;
  else
    return p.second;
}
template <size_t _Ip, class _T1, class _T2>
  requires(_Ip < 2)
constexpr tuple_element_t<_Ip, pair<_T1, _T2>>&& get(pair<_T1, _T2>&& p) noexcept {
  if constexpr (_Ip == 0)
    return static_cast<_T1&&>(p.first);
  else
    return static_cast<_T2&&>(p.second);
}
template <size_t _Ip, class _T1, class _T2>
  requires(_Ip < 2)
constexpr const tuple_element_t<_Ip, pair<_T1, _T2>>&& get(const pair<_T1, _T2>&& p) noexcept {
  if constexpr (_Ip == 0)
    return static_cast<const _T1&&>(p.first);
  else
    return static_cast<const _T2&&>(p.second);
}

template <class _Tp, class _T1, class _T2>
  requires(std::is_same_v<_Tp, _T1> != std::is_same_v<_Tp, _T2>)
constexpr _Tp& get(pair<_T1, _T2>& p) noexcept {
  return get<std::is_same_v<_Tp, _T1> ? 0 : 1>(p);
}
template <class _Tp, class _T1, class _T2>
  requires(std::is_same_v<_Tp, _T1> != std::is_same_v<_Tp, _T2>)
constexpr const _Tp& get(const pair<_T1, _T2>& p) noexcept {
  return get<std::is_same_v<_Tp, _T1> ? 0 : 1>(p);
}
template <class _Tp, class _T1, class _T2>
  requires(std::is_same_v<_Tp, _T1> != std::is_same_v<_Tp, _T2>)
constexpr _Tp&& get(pair<_T1, _T2>&& p) noexcept {
  return get<std::is_same_v<_Tp, _T1> ? 0 : 1>(static_cast<pair<_T1, _T2>&&>(p));
}
template <class _Tp, class _T1, class _T2>
  requires(std::is_same_v<_Tp, _T1> != std::is_same_v<_Tp, _T2>)
constexpr const _Tp&& get(const pair<_T1, _T2>&& p) noexcept {
  return get<std::is_same_v<_Tp, _T1> ? 0 : 1>(static_cast<const pair<_T1, _T2>&&>(p));
}

}} // namespace std
