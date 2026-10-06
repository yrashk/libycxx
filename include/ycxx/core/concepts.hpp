// libycxx core: <concepts>
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/compare.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, class _Up>
concept same_as = __ycxx::__detail::__same_as_<_Tp, _Up>;

template <class _Derived, class _Base>
concept derived_from =
    __is_base_of(_Base, _Derived) && __is_convertible(const volatile _Derived*, const volatile _Base*);

template <class _From, class _To>
concept convertible_to = __ycxx::__detail::__convertible_to_<_From, _To>;

template <class _Tp, class _Up>
concept common_reference_with = __ycxx::__detail::__common_reference_with_<_Tp, _Up>;

template <class _Tp, class _Up>
concept common_with =
    same_as<common_type_t<_Tp, _Up>, common_type_t<_Up, _Tp>> && requires {
      static_cast<common_type_t<_Tp, _Up>>(std::declval<_Tp>());
      static_cast<common_type_t<_Tp, _Up>>(std::declval<_Up>());
    } && common_reference_with<add_lvalue_reference_t<const _Tp>, add_lvalue_reference_t<const _Up>> &&
    common_reference_with<add_lvalue_reference_t<common_type_t<_Tp, _Up>>,
                          common_reference_t<add_lvalue_reference_t<const _Tp>, add_lvalue_reference_t<const _Up>>>;

template <class _Tp>
concept integral = ::__ycxx::__detail::is_integral_v<_Tp>;
template <class _Tp>
concept signed_integral = integral<_Tp> && ::__ycxx::__detail::is_signed_v<_Tp>;
template <class _Tp>
concept unsigned_integral = integral<_Tp> && !signed_integral<_Tp>;
template <class _Tp>
concept floating_point = ::__ycxx::__detail::__is_floating_v<_Tp>;

template <class _LHS, class _RHS>
concept assignable_from =
    ::__ycxx::__detail::__is_lref_v<_LHS> &&
    common_reference_with<const ::__ycxx::__detail::__remove_ref_t<_LHS>&, const ::__ycxx::__detail::__remove_ref_t<_RHS>&> &&
    requires(_LHS __lhs, _RHS&& __rhs) {
      { __lhs = static_cast<_RHS&&>(__rhs) } -> same_as<_LHS>;
    };

template <class _Tp>
concept destructible = __is_nothrow_destructible(_Tp);

template <class _Tp, class... _Args>
concept constructible_from = destructible<_Tp> && __is_constructible(_Tp, _Args...);

template <class _Tp>
concept default_initializable = constructible_from<_Tp> && requires { _Tp{}; } && requires { ::new _Tp; };

template <class _Tp>
concept move_constructible = constructible_from<_Tp, _Tp> && convertible_to<_Tp, _Tp>;

template <class _Tp>
concept copy_constructible = move_constructible<_Tp> && constructible_from<_Tp, _Tp&> && convertible_to<_Tp&, _Tp> &&
                             constructible_from<_Tp, const _Tp&> && convertible_to<const _Tp&, _Tp> &&
                             constructible_from<_Tp, const _Tp> && convertible_to<const _Tp, _Tp>;

} // namespace std

// ranges::swap customization point object
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__swap_cpo {

template <class _Tp>
void swap(_Tp&, _Tp&) = delete;

template <class _Tp, class _Up>
concept __adl_swappable = (__is_class(__remove_cvref(_Tp)) || __is_union(__remove_cvref(_Tp)) || __is_enum(__remove_cvref(_Tp)) ||
                         __is_class(__remove_cvref(_Up)) || __is_union(__remove_cvref(_Up)) || __is_enum(__remove_cvref(_Up))) &&
                        requires(_Tp&& t, _Up&& __u) { swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)); };

template <class _Tp>
concept __move_and_assign = std::move_constructible<_Tp> && std::assignable_from<_Tp&, _Tp>;

struct __fn {
  template <class _Tp, class _Up>
    requires __adl_swappable<_Tp, _Up>
  constexpr void operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)))) {
    swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u));
  }

  template <class _Tp, class _Up, std::size_t _Np>
    requires(!__adl_swappable<_Tp (&)[_Np], _Up (&)[_Np]>) && requires(const __fn& __f, _Tp& a, _Up& b) { __f(a, b); }
  constexpr void operator()(_Tp (&t)[_Np], _Up (&__u)[_Np]) const noexcept(noexcept((*this)(*t, *__u))) {
    for (std::size_t i = 0; i < _Np; ++i)
      (*this)(t[i], __u[i]);
  }

  template <class _Tp>
    requires(!__adl_swappable<_Tp&, _Tp&>) && __move_and_assign<_Tp>
  constexpr void operator()(_Tp& a, _Tp& b) const
      noexcept(__is_nothrow_constructible(_Tp, _Tp &&) && __is_nothrow_assignable(_Tp&, _Tp &&)) {
    _Tp __tmp(static_cast<_Tp&&>(a));
    a = static_cast<_Tp&&>(b);
    b = static_cast<_Tp&&>(__tmp);
  }
};
}} // namespace __ycxx::__detail::__swap_cpo

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace __cpo {
inline constexpr __ycxx::__detail::__swap_cpo::__fn swap{};
}
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp>
concept swappable = requires(_Tp& a, _Tp& b) { ranges::swap(a, b); };

template <class _Tp, class _Up>
concept swappable_with = common_reference_with<_Tp, _Up> && requires(_Tp&& t, _Up&& __u) {
  ranges::swap(static_cast<_Tp&&>(t), static_cast<_Tp&&>(t));
  ranges::swap(static_cast<_Up&&>(__u), static_cast<_Up&&>(__u));
  ranges::swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u));
  ranges::swap(static_cast<_Up&&>(__u), static_cast<_Tp&&>(t));
};

template <class _Tp>
concept equality_comparable = __ycxx::__detail::__weakly_equality_comparable_with<_Tp, _Tp>;

template <class _Tp, class _Up>
concept equality_comparable_with =
    equality_comparable<_Tp> && equality_comparable<_Up> && __ycxx::__detail::__comparison_common_type_with<_Tp, _Up> &&
    equality_comparable<common_reference_t<const ::__ycxx::__detail::__remove_ref_t<_Tp>&, const ::__ycxx::__detail::__remove_ref_t<_Up>&>> &&
    __ycxx::__detail::__weakly_equality_comparable_with<_Tp, _Up>;

template <class _Tp>
concept totally_ordered = equality_comparable<_Tp> && __ycxx::__detail::__partially_ordered_with<_Tp, _Tp>;

template <class _Tp, class _Up>
concept totally_ordered_with =
    totally_ordered<_Tp> && totally_ordered<_Up> && equality_comparable_with<_Tp, _Up> &&
    totally_ordered<common_reference_t<const ::__ycxx::__detail::__remove_ref_t<_Tp>&, const ::__ycxx::__detail::__remove_ref_t<_Up>&>> &&
    __ycxx::__detail::__partially_ordered_with<_Tp, _Up>;

template <class _Tp>
concept movable = is_object_v<_Tp> && move_constructible<_Tp> && assignable_from<_Tp&, _Tp> && swappable<_Tp>;
template <class _Tp>
concept copyable = copy_constructible<_Tp> && movable<_Tp> && assignable_from<_Tp&, _Tp&> &&
                   assignable_from<_Tp&, const _Tp&> && assignable_from<_Tp&, const _Tp>;
template <class _Tp>
concept semiregular = copyable<_Tp> && default_initializable<_Tp>;
template <class _Tp>
concept regular = semiregular<_Tp> && equality_comparable<_Tp>;

template <class _Fp, class... _Args>
concept invocable = __ycxx::__detail::__invocable_<_Fp, _Args...>;
template <class _Fp, class... _Args>
concept regular_invocable = invocable<_Fp, _Args...>;

template <class _Fp, class... _Args>
concept predicate =
    regular_invocable<_Fp, _Args...> && __ycxx::__detail::__boolean_testable<invoke_result_t<_Fp, _Args...>>;
template <class _Rp, class _Tp, class _Up>
concept relation = predicate<_Rp, _Tp, _Tp> && predicate<_Rp, _Up, _Up> && predicate<_Rp, _Tp, _Up> && predicate<_Rp, _Up, _Tp>;
template <class _Rp, class _Tp, class _Up>
concept equivalence_relation = relation<_Rp, _Tp, _Up>;
template <class _Rp, class _Tp, class _Up>
concept strict_weak_order = relation<_Rp, _Tp, _Up>;

} // namespace std

