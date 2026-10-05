// libycxx core: std::pair ([pairs]).
#pragma once

#include <ycxx/core/utility_base.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class T>
void implicit_default_init_test(const T&);
// Copy-list-initialization from {} (i.e. the default constructor is not explicit).
template <class T>
concept implicitly_default_constructible = requires { implicit_default_init_test<T>({}); };

template <class T>
inline constexpr bool is_pair_v = false;
template <class A, class B>
inline constexpr bool is_pair_v<std::pair<A, B>> = true;

// ranges::subrange is excluded from pair's pair-like constructor ([pairs.pair]/14).
template <class T>
inline constexpr bool is_subrange = false;

template <class T1, class T2, class A, class B>
concept pair_constructible = __is_constructible(T1, A) && __is_constructible(T2, B);
template <class T1, class T2, class A, class B>
concept pair_dangles = pair_constructible<T1, T2, A, B> &&
                       (__reference_constructs_from_temporary(T1, A) || __reference_constructs_from_temporary(T2, B));
template <class T1, class T2, class A, class B>
concept pair_convertible = __is_convertible(A, T1) && __is_convertible(B, T2);

template <class P, class Pair>
concept pair_like_not_pair = pair_like<P> && !__is_same(__remove_cvref(P), Pair);

// decltype(get<I>(FWD(p))) for a pair-like P.
template <std::size_t I, class P>
using pair_like_get_t = decltype(get<I>(static_cast<P (*)()>(nullptr)()));

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class T1, class T2>
struct pair {
  using first_type = T1;
  using second_type = T2;

  T1 first;
  T2 second;

  pair(const pair&) = default;
  pair(pair&&) = default;

  // A template, so that the explicit-specifier is evaluated only when the constructor is used:
  // pair<int, T> may be instantiated while T is still incomplete ([vector.overview]/4 allows
  // struct T { vector<pair<int, T>> v; }).
  template <class U1 = T1, class U2 = T2>
    requires(std::is_constructible_v<U1> && std::is_constructible_v<U2>)
  constexpr explicit(!ycxx::detail::implicitly_default_constructible<U1> ||
                     !ycxx::detail::implicitly_default_constructible<U2>) pair()
      : first(), second() {}

  constexpr explicit(!std::is_convertible_v<const T1&, T1> || !std::is_convertible_v<const T2&, T2>)
      pair(const T1& x, const T2& y)
    requires(std::is_constructible_v<T1, const T1&> && std::is_constructible_v<T2, const T2&>)
      : first(x), second(y) {}

  // Converting constructors. Each comes with a deleted twin, selected (by constraint
  // subsumption) when a reference member would bind to a temporary.
  template <class U1 = T1, class U2 = T2>
    requires ycxx::detail::pair_constructible<T1, T2, U1&&, U2&&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, U1&&, U2&&>) pair(U1&& x, U2&& y)
      : first(static_cast<U1&&>(x)), second(static_cast<U2&&>(y)) {}
  template <class U1 = T1, class U2 = T2>
    requires ycxx::detail::pair_dangles<T1, T2, U1&&, U2&&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, U1&&, U2&&>) pair(U1&&, U2&&) = delete;

  template <class U1, class U2>
    requires ycxx::detail::pair_constructible<T1, T2, U1&, U2&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, U1&, U2&>) pair(pair<U1, U2>& p)
      : first(p.first), second(p.second) {}
  template <class U1, class U2>
    requires ycxx::detail::pair_dangles<T1, T2, U1&, U2&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, U1&, U2&>) pair(pair<U1, U2>&) = delete;

  template <class U1, class U2>
    requires ycxx::detail::pair_constructible<T1, T2, const U1&, const U2&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, const U1&, const U2&>) pair(const pair<U1, U2>& p)
      : first(p.first), second(p.second) {}
  template <class U1, class U2>
    requires ycxx::detail::pair_dangles<T1, T2, const U1&, const U2&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, const U1&, const U2&>)
      pair(const pair<U1, U2>&) = delete;

  template <class U1, class U2>
    requires ycxx::detail::pair_constructible<T1, T2, U1&&, U2&&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, U1&&, U2&&>) pair(pair<U1, U2>&& p)
      : first(static_cast<U1&&>(p.first)), second(static_cast<U2&&>(p.second)) {}
  template <class U1, class U2>
    requires ycxx::detail::pair_dangles<T1, T2, U1&&, U2&&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, U1&&, U2&&>) pair(pair<U1, U2>&&) = delete;

  template <class U1, class U2>
    requires ycxx::detail::pair_constructible<T1, T2, const U1&&, const U2&&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, const U1&&, const U2&&>)
      pair(const pair<U1, U2>&& p)
      : first(static_cast<const U1&&>(p.first)), second(static_cast<const U2&&>(p.second)) {}
  template <class U1, class U2>
    requires ycxx::detail::pair_dangles<T1, T2, const U1&&, const U2&&>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, const U1&&, const U2&&>)
      pair(const pair<U1, U2>&&) = delete;

  // pair-like, with the deleted twin of [pairs.pair]/17.
  template <ycxx::detail::pair_like_not_pair<pair> P>
    requires(!ycxx::detail::is_subrange<remove_cvref_t<P>>) &&
            ycxx::detail::pair_constructible<T1, T2, ycxx::detail::pair_like_get_t<0, P>,
                                             ycxx::detail::pair_like_get_t<1, P>>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, ycxx::detail::pair_like_get_t<0, P>,
                                                     ycxx::detail::pair_like_get_t<1, P>>) pair(P&& p)
      : first(get<0>(static_cast<P&&>(p))), second(get<1>(static_cast<P&&>(p))) {}
  template <ycxx::detail::pair_like_not_pair<pair> P>
    requires(!ycxx::detail::is_subrange<remove_cvref_t<P>>) &&
            ycxx::detail::pair_dangles<T1, T2, ycxx::detail::pair_like_get_t<0, P>, ycxx::detail::pair_like_get_t<1, P>>
  constexpr explicit(!ycxx::detail::pair_convertible<T1, T2, ycxx::detail::pair_like_get_t<0, P>,
                                                     ycxx::detail::pair_like_get_t<1, P>>) pair(P&&) = delete;

  // piecewise
  template <class... Args1, class... Args2>
  constexpr pair(piecewise_construct_t, tuple<Args1...> a1, tuple<Args2...> a2)
      : pair(a1, a2, index_sequence_for<Args1...>{}, index_sequence_for<Args2...>{}) {}

  // --- assignment ---
  constexpr pair& operator=(const pair& p)
    requires(std::is_assignable_v<T1&, const T1&> && std::is_assignable_v<T2&, const T2&>)
  {
    first = p.first;
    second = p.second;
    return *this;
  }
  constexpr const pair& operator=(const pair& p) const
    requires(std::is_assignable_v<const T1&, const T1&> && std::is_assignable_v<const T2&, const T2&>)
  {
    first = p.first;
    second = p.second;
    return *this;
  }
  template <class U1, class U2>
    requires(std::is_assignable_v<T1&, const U1&> && std::is_assignable_v<T2&, const U2&>)
  constexpr pair& operator=(const pair<U1, U2>& p) {
    first = p.first;
    second = p.second;
    return *this;
  }
  template <class U1, class U2>
    requires(std::is_assignable_v<const T1&, const U1&> && std::is_assignable_v<const T2&, const U2&>)
  constexpr const pair& operator=(const pair<U1, U2>& p) const {
    first = p.first;
    second = p.second;
    return *this;
  }
  constexpr pair& operator=(pair&& p) noexcept(std::is_nothrow_assignable_v<T1&, T1 &&> &&
                                               std::is_nothrow_assignable_v<T2&, T2 &&>)
    requires(std::is_assignable_v<T1&, T1 &&> && std::is_assignable_v<T2&, T2 &&>)
  {
    first = static_cast<T1&&>(p.first);
    second = static_cast<T2&&>(p.second);
    return *this;
  }
  constexpr const pair& operator=(pair&& p) const
    requires(std::is_assignable_v<const T1&, T1 &&> && std::is_assignable_v<const T2&, T2 &&>)
  {
    first = static_cast<T1&&>(p.first);
    second = static_cast<T2&&>(p.second);
    return *this;
  }
  template <class U1, class U2>
    requires(std::is_assignable_v<T1&, U1 &&> && std::is_assignable_v<T2&, U2 &&>)
  constexpr pair& operator=(pair<U1, U2>&& p) {
    first = static_cast<U1&&>(p.first);
    second = static_cast<U2&&>(p.second);
    return *this;
  }
  template <class U1, class U2>
    requires(std::is_assignable_v<const T1&, U1 &&> && std::is_assignable_v<const T2&, U2 &&>)
  constexpr const pair& operator=(pair<U1, U2>&& p) const {
    first = static_cast<U1&&>(p.first);
    second = static_cast<U2&&>(p.second);
    return *this;
  }
  template <ycxx::detail::pair_like_not_pair<pair> P>
    requires(!ycxx::detail::is_subrange<remove_cvref_t<P>>) && requires(pair& self, P&& p) {
      self.first = get<0>(static_cast<P&&>(p));
      self.second = get<1>(static_cast<P&&>(p));
    }
  constexpr pair& operator=(P&& p) {
    first = get<0>(static_cast<P&&>(p));
    second = get<1>(static_cast<P&&>(p));
    return *this;
  }
  template <ycxx::detail::pair_like_not_pair<pair> P>
    requires(!ycxx::detail::is_subrange<remove_cvref_t<P>>) && requires(const pair& self, P&& p) {
      self.first = get<0>(static_cast<P&&>(p));
      self.second = get<1>(static_cast<P&&>(p));
    }
  constexpr const pair& operator=(P&& p) const {
    first = get<0>(static_cast<P&&>(p));
    second = get<1>(static_cast<P&&>(p));
    return *this;
  }

  // --- swap ---
  constexpr void swap(pair& p) noexcept(is_nothrow_swappable_v<T1> && is_nothrow_swappable_v<T2>) {
    ycxx::detail::swap_adl::do_swap(first, p.first);
    ycxx::detail::swap_adl::do_swap(second, p.second);
  }
  constexpr void swap(const pair& p) const
      noexcept(is_nothrow_swappable_v<const T1> && is_nothrow_swappable_v<const T2>) {
    ycxx::detail::swap_adl::do_swap(first, p.first);
    ycxx::detail::swap_adl::do_swap(second, p.second);
  }

private:
  template <class A1, class A2, size_t... I1, size_t... I2>
  constexpr pair(A1& a1, A2& a2, index_sequence<I1...>, index_sequence<I2...>)
      : first(get<I1>(static_cast<A1&&>(a1))...), second(get<I2>(static_cast<A2&&>(a2))...) {}
};

template <class T1, class T2>
pair(T1, T2) -> pair<T1, T2>;

// --- comparison ---
template <class T1, class T2, class U1, class U2>
  requires requires(const pair<T1, T2>& x, const pair<U1, U2>& y) {
    { x.first == y.first } -> ycxx::detail::boolean_testable;
    { x.second == y.second } -> ycxx::detail::boolean_testable;
  }
constexpr bool operator==(const pair<T1, T2>& x, const pair<U1, U2>& y) {
  return x.first == y.first && x.second == y.second;
}

template <class T1, class T2, class U1, class U2>
constexpr common_comparison_category_t<ycxx::detail::synth_three_way_result<T1, U1>,
                                       ycxx::detail::synth_three_way_result<T2, U2>>
operator<=>(const pair<T1, T2>& x, const pair<U1, U2>& y) {
  if (auto c = ycxx::detail::synth_three_way(x.first, y.first); c != 0)
    return c;
  return ycxx::detail::synth_three_way(x.second, y.second);
}

template <class T1, class T2>
  requires(is_swappable_v<T1> && is_swappable_v<T2>)
constexpr void swap(pair<T1, T2>& x, pair<T1, T2>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}
template <class T1, class T2>
  requires(is_swappable_v<const T1> && is_swappable_v<const T2>)
constexpr void swap(const pair<T1, T2>& x, const pair<T1, T2>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

template <class T1, class T2>
constexpr pair<unwrap_ref_decay_t<T1>, unwrap_ref_decay_t<T2>> make_pair(T1&& x, T2&& y) {
  return pair<unwrap_ref_decay_t<T1>, unwrap_ref_decay_t<T2>>(static_cast<T1&&>(x), static_cast<T2&&>(y));
}

// --- common_type / basic_common_reference ---
template <class T1, class T2, class U1, class U2, template <class> class TQual, template <class> class UQual>
  requires requires {
    typename pair<common_reference_t<TQual<T1>, UQual<U1>>, common_reference_t<TQual<T2>, UQual<U2>>>;
  }
struct basic_common_reference<pair<T1, T2>, pair<U1, U2>, TQual, UQual> {
  using type = pair<common_reference_t<TQual<T1>, UQual<U1>>, common_reference_t<TQual<T2>, UQual<U2>>>;
};
template <class T1, class T2, class U1, class U2>
  requires requires { typename pair<common_type_t<T1, U1>, common_type_t<T2, U2>>; }
struct common_type<pair<T1, T2>, pair<U1, U2>> {
  using type = pair<common_type_t<T1, U1>, common_type_t<T2, U2>>;
};

// --- tuple protocol ---
template <class T1, class T2>
struct tuple_size<pair<T1, T2>> : integral_constant<size_t, 2> {};
template <size_t I, class T1, class T2>
struct tuple_element<I, pair<T1, T2>> {
  static_assert(I < 2, "pair index out of range");
  using type = conditional_t<I == 0, T1, T2>;
};

template <size_t I, class T1, class T2>
  requires(I < 2)
constexpr tuple_element_t<I, pair<T1, T2>>& get(pair<T1, T2>& p) noexcept {
  if constexpr (I == 0)
    return p.first;
  else
    return p.second;
}
template <size_t I, class T1, class T2>
  requires(I < 2)
constexpr const tuple_element_t<I, pair<T1, T2>>& get(const pair<T1, T2>& p) noexcept {
  if constexpr (I == 0)
    return p.first;
  else
    return p.second;
}
template <size_t I, class T1, class T2>
  requires(I < 2)
constexpr tuple_element_t<I, pair<T1, T2>>&& get(pair<T1, T2>&& p) noexcept {
  if constexpr (I == 0)
    return static_cast<T1&&>(p.first);
  else
    return static_cast<T2&&>(p.second);
}
template <size_t I, class T1, class T2>
  requires(I < 2)
constexpr const tuple_element_t<I, pair<T1, T2>>&& get(const pair<T1, T2>&& p) noexcept {
  if constexpr (I == 0)
    return static_cast<const T1&&>(p.first);
  else
    return static_cast<const T2&&>(p.second);
}

template <class T, class T1, class T2>
  requires(std::is_same_v<T, T1> != std::is_same_v<T, T2>)
constexpr T& get(pair<T1, T2>& p) noexcept {
  return get<std::is_same_v<T, T1> ? 0 : 1>(p);
}
template <class T, class T1, class T2>
  requires(std::is_same_v<T, T1> != std::is_same_v<T, T2>)
constexpr const T& get(const pair<T1, T2>& p) noexcept {
  return get<std::is_same_v<T, T1> ? 0 : 1>(p);
}
template <class T, class T1, class T2>
  requires(std::is_same_v<T, T1> != std::is_same_v<T, T2>)
constexpr T&& get(pair<T1, T2>&& p) noexcept {
  return get<std::is_same_v<T, T1> ? 0 : 1>(static_cast<pair<T1, T2>&&>(p));
}
template <class T, class T1, class T2>
  requires(std::is_same_v<T, T1> != std::is_same_v<T, T2>)
constexpr const T&& get(const pair<T1, T2>&& p) noexcept {
  return get<std::is_same_v<T, T1> ? 0 : 1>(static_cast<const pair<T1, T2>&&>(p));
}

} // namespace std
