namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
namespace __cfg {
inline constexpr bool __hosted = 1;
namespace __layer {
}
inline constexpr bool __integer_division_traps = true;
template <class _Sp, class _Mp>
concept __has_is_pointer_interconvertible_with_class =
    requires(_Mp _Sp::* m) { __builtin_is_pointer_interconvertible_with_class(m); };
}
using __y_int128 = __int128;
using __uint128 = unsigned __int128;
struct __fp_format_info {
  int digits;
  int __min_exp;
  int __max_exp;
};
template <class _Tp>
inline constexpr __fp_format_info __fp_format{0, 0, 0};
using __float16 = _Float16;
template <>
inline constexpr __fp_format_info __fp_format<__float16>{11, (-13), 16};
using __float32 = _Float32;
using __float64 = _Float64;
template <>
inline constexpr __fp_format_info __fp_format<__float64>{53, (-1021), 1024};
using __y_float128 = _Float128;
using __bfloat16 = decltype(0.0bf16);
template <>
inline constexpr __fp_format_info __fp_format<__bfloat16>{8, (-125), 128};
template <class _Tp>
struct __bitint_info {
};
template <class _Tp>
inline constexpr int __bitint_width = __bitint_info<__remove_cv(_Tp)>::width;
template <class _Tp>
using __remove_ref_t = __remove_reference(_Tp);
template <template <class _Up, _Up...> class _Seq, class _Tp, _Tp _Np>
using __y_make_integer_seq = _Seq<_Tp, __integer_pack(_Np)...>;
}}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp, class... _Us>
inline constexpr bool __is_any_of = (__is_same(_Tp, _Us) || ...);
template <class _Tp>
inline constexpr bool __is_lref_v = false;
template <class _Tp>
inline constexpr bool is_void_v = __is_same(__remove_cv(_Tp), void);
template <class _Tp>
inline constexpr bool is_integral_v =
    __is_any_of<__remove_cv(_Tp), bool, char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t, short,
              unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long, __y_int128, __uint128>;
template <class _Tp>
inline constexpr bool __is_floating_v =
    __is_any_of<__remove_cv(_Tp), float, double, long double, __float16, __float32, __float64, __y_float128, __bfloat16>;
template <class _Tp>
consteval bool __signed_impl() {
    return _Tp(-1) < _Tp(0);
}
template <class _Tp>
consteval bool __unsigned_impl() {
    return _Tp(0) < _Tp(-1);
}
}}
typedef decltype(sizeof(0)) size_t;
typedef decltype(static_cast<int*>(nullptr) - static_cast<int*>(nullptr)) ptrdiff_t;
namespace [[__gnu__::__visibility__("hidden")]] std {
using ::size_t;
}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp, _Tp __v>
struct integral_constant {
};
template <bool _Bp>
using bool_constant = integral_constant<bool, _Bp>;
using false_type = bool_constant<false>;
template <class _Tp, class _Up>
inline constexpr bool is_same_v = __is_same(_Tp, _Up);
template <bool _Bp, class _Tp, class _Fp>
struct conditional {
};
template <class _Tp, class _Fp>
struct conditional<false, _Tp, _Fp> {
  using type = _Fp;
};
template <bool _Bp, class _Tp, class _Fp>
using conditional_t = typename conditional<_Bp, _Tp, _Fp>::type;
template <class _Tp>
struct type_identity {
};
template <class _Tp>
using type_identity_t = typename type_identity<_Tp>::type;
template <class _Tp>
struct remove_cv {
};
template <class _Tp>
using remove_cv_t = typename remove_cv<_Tp>::type;
template <class _Tp>
struct remove_reference {
};
template <class _Tp>
using remove_reference_t = typename remove_reference<_Tp>::type;
template <class _Tp>
struct remove_cvref {
};
template <class _Tp>
using remove_cvref_t = typename remove_cvref<_Tp>::type;
template <class _Tp>
struct add_lvalue_reference {
};
template <class _Tp>
using add_lvalue_reference_t = typename add_lvalue_reference<_Tp>::type;
template <class _Tp>
struct add_rvalue_reference {
};
template <class _Tp>
using add_rvalue_reference_t = typename add_rvalue_reference<_Tp>::type;
struct decay {
};
template <class _Tp>
add_rvalue_reference_t<_Tp> declval() noexcept {
}
template <class _Tp>
inline constexpr bool is_lvalue_reference_v = ::__ycxx::__detail::__is_lref_v<_Tp>;
template <class _Tp>
inline constexpr bool is_reference_v = __is_reference(_Tp);
template <class _Tp>
inline constexpr bool is_const_v = __is_const(_Tp);
template <class _Tp>
inline constexpr bool is_volatile_v = __is_volatile(_Tp);
template <class _Tp>
inline constexpr bool is_void_v = ::__ycxx::__detail::is_void_v<_Tp>;
template <class _Tp>
inline constexpr bool is_array_v = __is_array(_Tp);
template <class _Tp>
inline constexpr bool is_function_v = __is_function(_Tp);
template <class _Tp>
inline constexpr bool is_enum_v = __is_enum(_Tp);
template <class _Tp>
inline constexpr bool is_union_v = __is_union(_Tp);
template <class _Tp>
inline constexpr bool is_class_v = __is_class(_Tp);
template <class _Tp>
inline constexpr bool is_object_v = __is_object(_Tp);
template <class _Tp, class... _Args>
struct is_constructible : bool_constant<__is_constructible(_Tp, _Args...)> {};
template <class _Tp, class... _Args>
inline constexpr bool is_constructible_v = __is_constructible(_Tp, _Args...);
template <class _Tp, class _Up>
inline constexpr bool is_assignable_v = __is_assignable(_Tp, _Up);
template <class _From, class _To>
inline constexpr bool is_convertible_v = __is_convertible(_From, _To);
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _From, class _To>
struct __copy_cv_impl {
  using type = _To;
};
template <class _From, class _To>
using __copy_cv = typename __copy_cv_impl<_From, _To>::type;
template <class _From, class _To>
struct __copy_cvref_impl {
};
template <class _From, class _To>
using __copy_cvref = typename __copy_cvref_impl<_From, _To>::type;
template <class _Tp>
inline constexpr bool __is_standard_signed_integer =
    __is_any_of<__remove_cv(_Tp), signed char, short, int, long, long long, __y_int128>;
template <class _Tp>
inline constexpr bool __is_standard_unsigned_integer =
    __is_any_of<__remove_cv(_Tp), unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long, __uint128>;
template <class _Tp>
inline constexpr bool __is_signed_or_unsigned_integer =
    __is_standard_signed_integer<_Tp> || __is_standard_unsigned_integer<_Tp>;
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
enum class __invoke_kind { __plain, __mem_fn_ref, __mem_fn_rw, __mem_fn_ptr, __mem_obj_ref, __mem_obj_rw, __mem_obj_ptr };
template <class _Fp, class... _Args>
consteval __invoke_kind __classify_invoke() {
  using _FD = __remove_cvref(_Fp);
  if constexpr (__is_member_pointer(_FD) && sizeof...(_Args) > 0) {
    return __invoke_kind::__plain;
  }
}
template <__invoke_kind _Kp>
struct __invoker;
template <>
struct __invoker<__invoke_kind::__plain> {
};
template <class _Fp, class... _Args>
using __invoker_for = __invoker<__classify_invoke<_Fp, _Args...>()>;
template <class _Fp, class... _Args>
[[__gnu__::__always_inline__]] constexpr auto invoke(_Fp&& __f, _Args&&... __args) noexcept(
    noexcept(__invoker_for<_Fp, _Args...>::__call(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...)))
    -> decltype(__invoker_for<_Fp, _Args...>::__call(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...)) {
}
template <class _Fp, class... _Args>
concept __invocable_ = requires(_Fp&& __f, _Args&&... __args) { ::__ycxx::__detail::invoke(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...); };
template <class _Fp, class... _Args>
concept __nothrow_invocable_ = requires(_Fp&& __f, _Args&&... __args) {
  { ::__ycxx::__detail::invoke(static_cast<decltype(__f)&&>(__f), static_cast<decltype(__args)&&>(__args)...) } noexcept;
};
template <class _Fp, class... _Args>
using invoke_result_t = decltype(::__ycxx::__detail::invoke(std::declval<_Fp>(), std::declval<_Args>()...));
template <class _Tp>
void __implicitly_convert_to(_Tp) noexcept;
template <class _Rp, class _Fp, class... _Args>
consteval bool __is_invocable_r_impl() {
    return requires { ::__ycxx::__detail::__implicitly_convert_to<_Rp>(::__ycxx::__detail::invoke(std::declval<_Fp>(), std::declval<_Args>()...)); } &&
           !__reference_converts_from_temporary(_Rp, invoke_result_t<_Fp, _Args...>);
}
template <class _Rp, class _Fp, class... _Args>
consteval bool __is_nothrow_invocable_r_impl() {
    return requires {
      { ::__ycxx::__detail::__implicitly_convert_to<_Rp>(::__ycxx::__detail::invoke(std::declval<_Fp>(), std::declval<_Args>()...)) } noexcept;
    } && !__reference_converts_from_temporary(_Rp, invoke_result_t<_Fp, _Args...>);
}
template <class _Rp, class _Fp, class... _Args>
[[__gnu__::__always_inline__]] constexpr _Rp invoke_r(_Fp&& __f, _Args&&... __args) noexcept(__is_nothrow_invocable_r_impl<_Rp, _Fp, _Args...>()) {
}
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp>
inline constexpr bool is_bounded_array_v = __is_bounded_array(_Tp);
template <class _Tp>
inline constexpr bool is_nothrow_move_assignable_v =
    __is_nothrow_assignable(__add_lvalue_reference(_Tp), __add_rvalue_reference(_Tp));
template <class _Tp, class _Up>
struct reference_constructs_from_temporary : bool_constant<__reference_constructs_from_temporary(_Tp, _Up)> {};
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
struct __sign_pair;
template <class _Sp, class _Up>
struct __sign_pair_def {
  using __u = _Up;
};
template <> struct __sign_pair<__y_int128> : __sign_pair_def<__y_int128, __uint128> {};
template <class _Tp>
consteval auto __same_size_signed() {
    return static_cast<__y_int128*>(nullptr);
}
template <class _Tp>
struct __sign_base {
  using type = __sign_pair<__remove_pointer(decltype(__same_size_signed<_Tp>()))>;
};
template <class _Tp>
  requires requires { typename __sign_pair<_Tp>::s; }
struct __sign_base<_Tp> {
};
template <class _Tp>
concept __sign_changeable = (is_integral_v<_Tp> && !__is_same(__remove_cv(_Tp), bool)) || __is_enum(_Tp);
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp>
struct make_unsigned {};
template <class _Tp>
  requires __ycxx::__detail::__sign_changeable<_Tp>
struct make_unsigned<_Tp> {
  using type = __ycxx::__detail::__copy_cv<_Tp, typename __ycxx::__detail::__sign_base<__remove_cv(_Tp)>::type::__u>;
};
template <class _Tp>
using make_unsigned_t = typename make_unsigned<_Tp>::type;
template <class _Tp>
struct remove_all_extents {
};
template <class _Tp>
using remove_all_extents_t = typename remove_all_extents<_Tp>::type;
struct remove_pointer {
};
template <class _Tp>
struct add_pointer {
};
template <class _Tp>
using add_pointer_t = typename add_pointer<_Tp>::type;
template <class _Tp>
struct unwrap_reference {
};
template <class _Tp>
struct unwrap_ref_decay : unwrap_reference<__decay(_Tp)> {};
template <class _Tp>
using unwrap_ref_decay_t = typename unwrap_ref_decay<_Tp>::type;
}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class... _Tp>
struct common_type {};
template <class _T1, class _T2, class _T3, class... _Rp>
struct common_type<_T1, _T2, _T3, _Rp...> : common_type<typename common_type<_T1, _T2>::type, _T3, _Rp...> {};
template <class... _Tp>
using common_type_t = typename common_type<_Tp...>::type;
}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class... _Tp>
struct common_reference {};
template <class _T1, class _T2, class _T3, class... _Rp>
struct common_reference<_T1, _T2, _T3, _Rp...> : common_reference<typename common_reference<_T1, _T2>::type, _T3, _Rp...> {};
template <class... _Tp>
using common_reference_t = typename common_reference<_Tp...>::type;
template <class _Up = void, class _Tp>
consteval bool is_within_lifetime(const _Tp* p) noexcept {
    return true;
}
}
namespace [[__gnu__::__visibility__("hidden")]] std {
class partial_ordering {
};
template <class... _Ts>
struct common_comparison_category {
};
template <class... _Ts>
using common_comparison_category_t = typename common_comparison_category<_Ts...>::type;
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
concept __boolean_testable_impl = __is_convertible(_Tp, bool);
template <class _Tp>
concept __boolean_testable = __boolean_testable_impl<_Tp> && requires(_Tp&& t) {
  { !static_cast<_Tp&&>(t) } -> __boolean_testable_impl;
};
template <class _Tp, class _Up>
concept __weakly_equality_comparable_with =
    requires(const ::__ycxx::__detail::__remove_ref_t<_Tp>& t, const ::__ycxx::__detail::__remove_ref_t<_Up>& __u) {
      { __u != t } -> __boolean_testable;
    };
template <class _Tp, class _Up>
concept __partially_ordered_with = requires(const ::__ycxx::__detail::__remove_ref_t<_Tp>& t, const ::__ycxx::__detail::__remove_ref_t<_Up>& __u) {
  { t > __u } -> __boolean_testable;
};
template <class _Tp, class _Cat>
concept __compares_as = __is_same(std::common_comparison_category_t<_Tp, _Cat>, _Cat);
template <class _Tp, class _Up>
concept __same_as_impl = __is_same(_Tp, _Up);
template <class _Tp, class _Up>
concept __same_as_ = __same_as_impl<_Tp, _Up> && __same_as_impl<_Up, _Tp>;
template <class _From, class _To>
concept __convertible_to_ = __is_convertible(_From, _To) && requires { static_cast<_To>(std::declval<_From>()); };
template <class _Tp, class _Up>
concept __common_reference_with_ =
    __convertible_to_<_Tp, std::common_reference_t<_Tp, _Up>> && __convertible_to_<_Up, std::common_reference_t<_Tp, _Up>>;
template <class _Tp, class _Up, class _Cp = std::common_reference_t<const _Tp&, const _Up&>>
concept __comparison_common_type_with_impl =
    requires {
      requires __convertible_to_<const _Up&, const _Cp&> || __convertible_to_<_Up, const _Cp&>;
    };
template <class _Tp, class _Up>
concept __comparison_common_type_with = __comparison_common_type_with_impl<__remove_cvref(_Tp), __remove_cvref(_Up)>;
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Ep>
class initializer_list {
  const _Ep* __first_;
  size_t __size_;
};
}
typedef struct
{
  union
  {
  } __value;
} __mbstate_t;
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class __charT>
struct char_traits;
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _CharT, class _IntT, class _Up, _IntT _Eof>
struct __char_traits_base {
  using char_type = _CharT;
  static constexpr int compare(const char_type* __s1, const char_type* __s2, std::size_t n) {
    if constexpr (sizeof(char_type) == 1) {
      if !consteval {
      }
    }
    return 0;
  }
  static constexpr std::size_t length(const char_type* s) noexcept {
    if constexpr (sizeof(char_type) == 1 && __ycxx::__detail::__cfg::__hosted) {
    }
    std::size_t n = 0;
    return n;
  }
  static constexpr const char_type* find(const char_type* s, std::size_t n, const char_type& a) {
    if constexpr (sizeof(char_type) == 1 && __ycxx::__detail::__cfg::__hosted) {
    }
  }
  static constexpr char_type* move(char_type* __s1, const char_type* __s2, std::size_t n) {
    if consteval {
    }
  }
};
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <>
struct char_traits<char> : __ycxx::__adl_free::__char_traits_base<char, int, unsigned char, -1> {};
template <>
struct char_traits<char8_t> : __ycxx::__adl_free::__char_traits_base<char8_t, unsigned int, char8_t, 0xFFFF'FFFFu> {};
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
    common_reference_with<add_lvalue_reference_t<common_type_t<_Tp, _Up>>,
                          common_reference_t<add_lvalue_reference_t<const _Tp>, add_lvalue_reference_t<const _Up>>>;
template <class _LHS, class _RHS>
concept assignable_from =
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
                             constructible_from<_Tp, const _Tp> && convertible_to<const _Tp, _Tp>;
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__swap_cpo {
template <class _Tp, class _Up>
concept __adl_swappable = (__is_class(__remove_cvref(_Tp)) || __is_union(__remove_cvref(_Tp)) || __is_enum(__remove_cvref(_Tp)) ||
                         __is_class(__remove_cvref(_Up)) || __is_union(__remove_cvref(_Up)) || __is_enum(__remove_cvref(_Up))) &&
                        requires(_Tp&& t, _Up&& __u) { swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)); };
struct __fn {
  template <class _Tp, class _Up>
  constexpr void operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)))) {
  }
  template <class _Tp, class _Up, std::size_t _Np>
  constexpr void operator()(_Tp& a, _Tp& b) const
      noexcept(__is_nothrow_constructible(_Tp, _Tp &&) && __is_nothrow_assignable(_Tp&, _Tp &&)) {
  }
};
}}
namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace __cpo {
inline constexpr __ycxx::__detail::__swap_cpo::__fn swap{};
}
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp>
concept swappable = requires(_Tp& a, _Tp& b) { ranges::swap(a, b); };
template <class _Tp, class _Up>
concept swappable_with = common_reference_with<_Tp, _Up> && requires(_Tp&& t, _Up&& __u) {
  ranges::swap(static_cast<_Up&&>(__u), static_cast<_Tp&&>(t));
};
template <class _Tp>
concept equality_comparable = __ycxx::__detail::__weakly_equality_comparable_with<_Tp, _Tp>;
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
template <class _Tp, _Tp... _Ip>
struct integer_sequence {
};
template <size_t... _Ip>
using index_sequence = integer_sequence<size_t, _Ip...>;
template <class _Tp, _Tp _Np>
using make_integer_sequence = __ycxx::__detail::__y_make_integer_seq<integer_sequence, _Tp, _Np>;
template <size_t _Np>
using make_index_sequence = make_integer_sequence<size_t, _Np>;
template <class _Tp>
struct tuple_size;
template <class _Tp>
constexpr size_t tuple_size_v = tuple_size<_Tp>::value;
template <size_t _Ip, class _Tp>
struct tuple_element;
template <size_t _Ip, class _Tp>
using tuple_element_t = typename tuple_element<_Ip, _Tp>::type;
template <class... _Types>
class tuple;
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
inline constexpr bool __is_tuple_like_impl = false;
template <class _Tp>
concept __tuple_like = __is_tuple_like_impl<__remove_cvref(_Tp)>;
template <class _Fn, class _Tuple, std::size_t... _Ip>
consteval bool __applicable_impl(std::index_sequence<_Ip...>*) {
  return requires { ::__ycxx::__detail::invoke(std::declval<_Fn>(), get<_Ip>(std::declval<_Tuple>())...); };
}
template <class _Fn, class _Tuple, std::size_t... _Ip>
auto __apply_result_impl(std::index_sequence<_Ip...>*)
    -> decltype(::__ycxx::__detail::invoke(std::declval<_Fn>(), get<_Ip>(std::declval<_Tuple>())...));
template <class _Tuple>
using __tuple_indices = std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<_Tuple>>>;
template <class _Fn, class _Tuple>
consteval bool is_applicable_v() {
    return __applicable_impl<_Fn, _Tuple>(static_cast<__tuple_indices<_Tuple>*>(nullptr));
}
template <class _Fn, class _Tuple>
consteval bool is_nothrow_applicable_v() {
    return __nothrow_applicable_impl<_Fn, _Tuple>(static_cast<__tuple_indices<_Tuple>*>(nullptr));
}
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Fn, class _Tuple>
constexpr bool is_nothrow_applicable_v = __ycxx::__detail::is_nothrow_applicable_v<_Fn, _Tuple>();
template <class _Fn, class _Tuple>
struct apply_result {};
template <class _Fn, class _Tuple>
using apply_result_t = typename apply_result<_Fn, _Tuple>::type;
template <class _CharT>
class allocator;
template <class _CharT, class _Traits, class _Alloc>
class basic_string;
using string = basic_string<char, char_traits<char>, allocator<char>>;
}
extern "C" {
enum ycxx_error_kind : int {
};
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
[[__gnu__::__always_inline__]] constexpr void __precondition(bool ok, const char* __msg) noexcept {
  if consteval {
  }
}
[[noreturn]] [[__gnu__::__cold__]] constexpr void __raise_std(ycxx_error_kind kind, const char* what) {
  if consteval {
  }
}
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
struct hash {
};
struct piecewise_construct_t {
};
struct in_place_t {
};
inline constexpr in_place_t in_place{};
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
consteval int __floor_log10_pow2(int e) {
  constexpr __y_int128 num = 301029995663981195LL;
  constexpr __y_int128 den = 1000000000000000000LL;
  __y_int128 p = static_cast<__y_int128>(e) * num;
  return static_cast<int>(p >= 0 ? p / den : -((-p + den - 1) / den));
}
template <class _Tp>
consteval _Tp __pow2(int e) {
}
struct __limits_base {
};
template <class _Tp>
struct __int_limits : __limits_base {
  static constexpr bool traps = __cfg::__integer_division_traps && __bitint_width<_Tp> == 0 &&
                                __is_signed_or_unsigned_integer<_Tp> && sizeof(_Tp) >= sizeof(int);
  static constexpr _Tp __max_value = [] {
  }();
};
template <class _Tp>
consteval _Tp __make_signaling_nan() {
}
template <class _Tp>
consteval auto __select_limits() {
}
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp>
class numeric_limits : public decltype(__ycxx::__detail::__select_limits<_Tp>()) {};
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
struct __first_template_arg {};
template <template <class, class...> class _Tmpl, class _Tp, class... _Rest>
struct __first_template_arg<_Tmpl<_Tp, _Rest...>> {
};
template <class _Tp, class _Up>
struct __rebind_first {};
template <class _Ptr>
struct __ptr_element : __first_template_arg<_Ptr> {};
template <class _Ptr>
  requires requires { typename _Ptr::element_type; }
struct __ptr_element<_Ptr> {
};
template <class _Ptr>
struct __ptr_difference {
};
template <class _Ptr>
  requires requires { typename _Ptr::difference_type; }
struct __ptr_difference<_Ptr> {
};
template <class _Ptr, class _Up>
struct __ptr_rebind : __rebind_first<_Ptr, _Up> {};
template <class _Ptr, class _Up>
  requires requires { typename _Ptr::template rebind<_Up>; }
struct __ptr_rebind<_Ptr, _Up> {
};
template <class _Ptr>
struct __pointer_traits_base {};
template <class _Ptr>
  requires requires { typename __ptr_element<_Ptr>::type; }
struct __pointer_traits_base<_Ptr> {
};
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Ptr>
struct pointer_traits : __ycxx::__detail::__pointer_traits_base<_Ptr> {};
template <class _Tp>
struct pointer_traits<_Tp*> {
  using pointer = _Tp*;
  using difference_type = ptrdiff_t;
  static constexpr pointer pointer_to(conditional_t<is_void_v<_Tp>, struct __ycxx_nat, _Tp>& r) noexcept
  {
  }
};
template <class _Tp>
constexpr _Tp* to_address(_Tp* p) noexcept {
}
template <class _Tp, class... _Args>
constexpr _Tp* construct_at(_Tp* location, _Args&&... __args) noexcept(noexcept(::new(static_cast<void*>(location))
                                                                              _Tp(static_cast<_Args&&>(__args)...))) {
  if constexpr (is_array_v<_Tp>) {
  }
}
struct allocator_arg_t {
};
template <class _Tp, class _Alloc>
struct uses_allocator : false_type {};
template <class _Tp, class _Alloc>
constexpr bool uses_allocator_v = uses_allocator<_Tp, _Alloc>::value;
template <class _Tp>
class allocator {
  static_assert(!is_const_v<_Tp> && !is_volatile_v<_Tp> && !is_reference_v<_Tp> && !is_function_v<_Tp>,
                "std::allocator<T>: T must be a cv-unqualified object type");
public:
  using value_type = _Tp;
  [[nodiscard]] constexpr _Tp* allocate(size_t n) {
  }
  template <class _Up>
  friend constexpr bool operator==(const allocator&, const allocator<_Up>&) noexcept {
    return true;
  }
};
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Ap, class _Default>
struct __alloc_pointer {
  using type = _Default;
};
template <class _Ap, class _Default>
  requires requires { typename _Ap::pointer; }
struct __alloc_pointer<_Ap, _Default> {
};
struct __alloc_const_pointer {
};
template <class _Ap, class _Ptr>
struct __alloc_const_void_pointer {
};
template <class _Ap, class _Ptr>
  requires requires { typename _Ap::const_void_pointer; }
struct __alloc_const_void_pointer<_Ap, _Ptr> {
};
template <class _Ap, class _Ptr>
struct __alloc_difference {
  using type = typename std::pointer_traits<_Ptr>::difference_type;
};
template <class _Ap, class _Ptr>
  requires requires { typename _Ap::difference_type; }
struct __alloc_difference<_Ap, _Ptr> {
};
template <class _Ap, class _Diff>
struct __y_alloc_size {
  using type = std::make_unsigned_t<_Diff>;
};
template <class _Ap, class _Diff>
  requires requires { typename _Ap::size_type; }
struct __y_alloc_size<_Ap, _Diff> {
};
template <class _Ap, class _Tp>
struct __alloc_rebind : __rebind_first<_Ap, _Tp> {};
template <class _Ap, class _Tp>
  requires requires { typename _Ap::template rebind<_Tp>::other; }
struct __alloc_rebind<_Ap, _Tp> {
};
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Alloc>
struct allocator_traits {
  using value_type = typename _Alloc::value_type;
  using pointer = typename __ycxx::__detail::__alloc_pointer<_Alloc, value_type*>::type;
  using difference_type = typename __ycxx::__detail::__alloc_difference<_Alloc, pointer>::type;
  using size_type = typename __ycxx::__detail::__y_alloc_size<_Alloc, difference_type>::type;
};
struct input_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
template <class>
struct incrementable_traits {};
template <class _Tp>
struct iterator_traits;
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
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
};
template <class _Tp>
concept __has_member_value_type = requires { typename _Tp::value_type; };
template <class _Tp>
concept __has_member_element_type = requires { typename _Tp::element_type; };
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Ip>
using iter_difference_t =
    typename conditional_t<__ycxx::__detail::__is_primary_iterator_traits<remove_cvref_t<_Ip>>,
                           incrementable_traits<remove_cvref_t<_Ip>>, iterator_traits<remove_cvref_t<_Ip>>>::difference_type;
template <class>
struct indirectly_readable_traits {};
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
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__iter_move_cpo {
template <class _Tp>
concept __adl_iter_move = (std::is_class_v<std::remove_cvref_t<_Tp>> || std::is_union_v<std::remove_cvref_t<_Tp>> ||
                         std::is_enum_v<std::remove_cvref_t<_Tp>>) &&
                        requires(_Tp&& t) { iter_move(static_cast<_Tp&&>(t)); };
template <class _Tp>
consteval bool __iter_move_noexcept() {
    return noexcept(iter_move(std::declval<_Tp>()));
}
template <class _Tp>
struct result {
};
template <class _Tp>
  requires(!__adl_iter_move<_Tp>) && std::is_lvalue_reference_v<decltype(*std::declval<_Tp>())>
struct result<_Tp> {
};
template <class _Tp>
  requires __adl_iter_move<_Tp>
struct result<_Tp> {
};
struct __fn {
  template <class _Tp>
  [[nodiscard]] constexpr typename result<_Tp>::type operator()(_Tp&& t) const noexcept(__iter_move_noexcept<_Tp>()) {
  }
};
}}
namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace __cpo {
inline constexpr __ycxx::__detail::__iter_move_cpo::__fn iter_move{};
}
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <__ycxx::__detail::__dereferenceable _Tp>
using iter_rvalue_reference_t = decltype(ranges::iter_move(declval<_Tp&>()));
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _In>
concept __indirectly_readable_impl =
    std::common_reference_with<std::iter_rvalue_reference_t<_In>&&, const std::iter_value_t<_In>&>;
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _In>
concept indirectly_readable = __ycxx::__detail::__indirectly_readable_impl<remove_cvref_t<_In>>;
template <indirectly_readable _Tp>
using iter_common_reference_t = common_reference_t<iter_reference_t<_Tp>, iter_value_t<_Tp>&>;
template <class _Out, class _Tp>
concept indirectly_writable = requires(_Out&& __o, _Tp&& t) {
  const_cast<const iter_reference_t<_Out>&&>(*static_cast<_Out&&>(__o)) = static_cast<_Tp&&>(t);
};
template <class _Ip>
concept weakly_incrementable = movable<_Ip> && requires(_Ip i) {
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
concept sized_sentinel_for =
    requires(const _Ip& i, const _Sp& s) {
      { i - s } -> same_as<iter_difference_t<_Ip>>;
    };
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
consteval auto __iter_concept_impl() {
}
template <class _Ip>
using __iter_concept = typename decltype(__iter_concept_impl<_Ip>())::type;
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Ip>
concept input_iterator = input_or_output_iterator<_Ip> && indirectly_readable<_Ip> &&
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
                                 requires(_Ip i) {
      { std::to_address(i) } -> same_as<add_pointer_t<iter_reference_t<_Ip>>>;
    };
template <class _In, class _Out>
concept indirectly_movable_storable =
    assignable_from<iter_value_t<_In>&, iter_reference_t<_In>>;
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__iter_swap_cpo {
template <class _I1, class _I2>
void iter_swap(_I1, _I2) = delete;
template <class _Tp, class _Up>
consteval bool __iter_swap_noexcept() {
    return noexcept((void)iter_swap(std::declval<_Tp>(), std::declval<_Up>()));
};
};
}
namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
template <class _Tp>
constexpr bool enable_borrowed_range = false;
}}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_access {
template <class _Tp>
concept __class_or_enum = std::is_class_v<std::remove_cvref_t<_Tp>> || std::is_union_v<std::remove_cvref_t<_Tp>> ||
                        std::is_enum_v<std::remove_cvref_t<_Tp>>;
template <class _Tp>
concept __complete_array_elem = requires { sizeof(std::remove_all_extents_t<std::remove_reference_t<_Tp>>); };
namespace __begin_ns {
template <class _Tp>
concept __member = requires(_Tp& t) {
  { auto(t.begin()) } -> std::input_or_output_iterator;
};
template <class _Tp>
concept __adl = __class_or_enum<_Tp> && requires(_Tp& t) {
  { auto(begin(t)) } -> std::input_or_output_iterator;
};
struct __fn {
  static consteval bool nothrow() {
      return true;
  }
  template <class _Tp>
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
  }
};
}
}}
namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace __cpo {
inline constexpr __ycxx::__detail::__range_access::__begin_ns::__fn begin{};
}
template <class _Tp>
using iterator_t = decltype(ranges::begin(std::declval<_Tp&>()));
}}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_access {
namespace __end_ns {
template <class _Tp>
concept __member = requires(_Tp& t) {
  { auto(t.end()) } -> std::sentinel_for<std::ranges::iterator_t<_Tp>>;
};
template <class _Tp>
concept __adl = __class_or_enum<_Tp> && requires(_Tp& t) {
  { auto(end(t)) } -> std::sentinel_for<std::ranges::iterator_t<_Tp>>;
};
struct __fn {
  static consteval bool nothrow() {
      return true;
  }
  template <class _Tp>
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
    if constexpr (std::is_bounded_array_v<std::remove_reference_t<_Tp>>) {
    }
  }
};
}
}}
namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace __cpo {
inline constexpr __ycxx::__detail::__range_access::__end_ns::__fn end{};
}
template <class _Tp>
concept range = requires(_Tp& t) {
  ranges::end(t);
};
template <class _Tp>
concept borrowed_range = range<_Tp> && (is_lvalue_reference_v<_Tp> || enable_borrowed_range<remove_cvref_t<_Tp>>);
template <range _Rp>
using range_value_t = iter_value_t<iterator_t<_Rp>>;
template <range _Rp>
using range_common_reference_t = iter_common_reference_t<iterator_t<_Rp>>;
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class __charT, class __traits = char_traits<__charT>>
class basic_string_view;
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Dp, class __charT, class __traits>
concept __has_string_view_conversion =
    requires(_Dp& d) { d.operator ::std::basic_string_view<__charT, __traits>(); };
template <class __traits>
struct __sv_comparison_category {
};
template <class __traits>
  requires requires { typename __traits::comparison_category; }
struct __sv_comparison_category<__traits> {
};
}}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _It, class _End>
basic_string_view(_It, _End) -> basic_string_view<iter_value_t<_It>>;
template <class _Rp>
basic_string_view(_Rp&&) -> basic_string_view<ranges::range_value_t<_Rp>>;
template <class __charT, class __traits>
constexpr typename __ycxx::__detail::__sv_comparison_category<__traits>::type operator<=>(
    basic_string_view<__charT, __traits> __lhs, type_identity_t<basic_string_view<__charT, __traits>> __rhs) noexcept {
}
template <class __charT, class __traits = char_traits<__charT>, class _Allocator = allocator<__charT>>
class basic_string {
  static_assert(is_same_v<typename __traits::char_type, __charT>,
                "std::basic_string: Allocator::value_type must be charT ([string.require])");
  using __alloc_traits = allocator_traits<_Allocator>;
  using __sv_type = basic_string_view<__charT, __traits>;
  using size_type = typename __alloc_traits::size_type;
  using difference_type = typename __alloc_traits::difference_type;
  template <class _Tp>
  static constexpr bool __sv_like = is_convertible_v<const _Tp&, __sv_type> && !is_convertible_v<const _Tp&, const __charT*> &&
                                  !is_convertible_v<const _Tp*, const basic_string*>;
  static constexpr bool __pocma = __alloc_traits::propagate_on_container_move_assignment::value;
  static constexpr bool __pocs = __alloc_traits::propagate_on_container_swap::value;
  static constexpr bool __always_equal = __alloc_traits::is_always_equal::value;
  static constexpr size_t __buf_len = 16 / sizeof(__charT) > 1 ? 16 / sizeof(__charT) : 1;
  __charT* __ptr_;
  size_type __size_;
  union {
    __charT __buf_[__buf_len];
  };
  [[no_unique_address]] _Allocator __alloc_;
  constexpr void __activate_buf() noexcept {
  }
public:
  template <class _Tp>
  constexpr explicit basic_string(const _Tp& t, const _Allocator& a = _Allocator()) : __ptr_(nullptr), __size_(0), __alloc_(a) {
  }
  constexpr basic_string(const __charT* s, const _Allocator& a = _Allocator())
      : __ptr_(nullptr), __size_(0), __alloc_(a) {
  }
  constexpr basic_string(initializer_list<__charT> il, const _Allocator& a = _Allocator())
      : __ptr_(nullptr), __size_(0), __alloc_(a) {
  }
  constexpr basic_string& operator=(basic_string&& str) noexcept(__pocma || __always_equal) {
    if constexpr (__pocma || __always_equal) {
      if (__alloc_ == str.__alloc_) {
      }
    }
  }
  constexpr size_type size() const noexcept { return __size_; }
  constexpr size_type max_size() const noexcept {
    const auto __diff_max = static_cast<make_unsigned_t<difference_type>>(numeric_limits<difference_type>::max()) /
                          sizeof(__charT);
    const size_type __by_diff = __diff_max < numeric_limits<size_type>::max() ? static_cast<size_type>(__diff_max)
                                                                          : numeric_limits<size_type>::max();
  }
  constexpr void resize(size_type n, __charT c) {
    if (n <= __size_) {
    }
  }
  constexpr void swap(basic_string& s) noexcept(__pocs || __always_equal) {
      __ycxx::__detail::__precondition(__always_equal || __alloc_ == s.__alloc_,
                                 "std::basic_string::swap: unequal allocators that do not propagate");
  }
};
template <class _InputIterator, class _Allocator = allocator<typename iterator_traits<_InputIterator>::value_type>>
basic_string(_InputIterator, _InputIterator, _Allocator = _Allocator())
    -> basic_string<typename iterator_traits<_InputIterator>::value_type,
                    char_traits<typename iterator_traits<_InputIterator>::value_type>, _Allocator>;
using u8string = basic_string<char8_t>;
namespace pmr {
template <class _Sp>
struct __string_hash {
  [[nodiscard]] std::size_t operator()(const _Sp& s) const noexcept {
  }
};
}}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
inline constexpr bool __is_tuple_specialization = false;
struct __alloc_tag_t {};
template <class _Tp, class _Alloc, class... _Args>
constexpr _Tp __make_using_alloc(const _Alloc& a, _Args&&... __args);
template <std::size_t _Ip, class _Tp>
struct __tuple_leaf {
  [[no_unique_address]] _Tp value;
  template <class _Alloc, class... _Args>
    requires std::uses_allocator_v<std::remove_cv_t<_Tp>, _Alloc> &&
             std::is_constructible_v<_Tp, std::allocator_arg_t, const _Alloc&, _Args...>
  constexpr __tuple_leaf(__alloc_tag_t, const _Alloc& a, _Args&&... __args)
      : value(__make_using_alloc<_Tp>(a, static_cast<_Args&&>(__args)...)) {}
};
template <class _Seq, class... _Ts>
struct __tuple_storage;
template <std::size_t... _Ip, class... _Ts>
struct __tuple_storage<std::index_sequence<_Ip...>, _Ts...> : __tuple_leaf<_Ip, _Ts>... {
  template <class... _Args>
  constexpr explicit __tuple_storage(std::in_place_t, _Args&&... __args)
      : __tuple_leaf<_Ip, _Ts>(std::in_place, static_cast<_Args&&>(__args))... {}
  template <class _Alloc, class... _Args>
  constexpr __tuple_storage(__alloc_tag_t, const _Alloc& a, std::in_place_t) : __tuple_leaf<_Ip, _Ts>(__alloc_tag_t{}, a)... {}
};
template <std::size_t _Ip, class _Tp>
constexpr _Tp& __leaf_get(__tuple_leaf<_Ip, _Tp>& __l) noexcept {
}
template <class _Tp>
concept __implicit_default = requires { __implicit_copy_list_init<_Tp>({}); };
template <template <class, class> class _Pred, class _TT, class _UT>
struct __all_pairs {
};
template <template <class, class> class _Pred, class... _Ts, class... _Us>
struct __all_pairs<_Pred, std::tuple<_Ts...>, std::tuple<_Us...>> {
};
template <template <class, class> class _Pred, class _TT, class _UT>
struct __any_pair {
};
template <template <class, class> class _Pred, class... _Ts, class... _Us>
struct __any_pair<_Pred, std::tuple<_Ts...>, std::tuple<_Us...>> {
};
template <class _Tp, class _Up>
struct __const_assignable_from_ : std::bool_constant<std::is_assignable_v<const _Tp&, _Up>> {};
template <class _TT, class _UT>
concept __elems_constructible = __all_pairs<std::is_constructible, _TT, _UT>::value;
template <class _TT, class _UT>
struct __get_types;
template <class _Up, std::size_t... _Ip>
using __get_types_t =
    typename __get_types<_Up, std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<_Up>>>>::type;
template <class _TT, class... _Us>
concept __tuple_args_ok =
    (sizeof...(_Us) == 1 || sizeof...(_Us) > 3 || !__is_same(std::remove_cvref_t<_Us...[0]>, std::allocator_arg_t) ||
     __is_same(std::remove_cvref_t<std::tuple_element_t<0, _TT>>, std::allocator_arg_t));
template <class _Tp, class _Up, class _Src>
concept __tuple_conv_single_ok1 =
    !__is_same(_Tp, _Up) && !std::is_convertible_v<_Src, _Tp> && !std::is_constructible_v<_Tp, _Src>;
template <class _TT, class _Src>
concept __tuple_conv_single_ok =
    __tuple_conv_single_ok1<std::tuple_element_t<0, _TT>, std::tuple_element_t<0, std::remove_cvref_t<_Src>>, _Src>;
template <class _TT, class _Src>
concept __tuple_like_single_ok = std::tuple_size_v<_TT> != 1 ||
                               (!std::is_convertible_v<_Src, std::tuple_element_t<0, _TT>> &&
                                !std::is_constructible_v<std::tuple_element_t<0, _TT>, _Src>);
template <class _TT, class _Src>
concept __tuple_from_tuple = __is_tuple_specialization<std::remove_cvref_t<_Src>> &&
                           __tuple_conv_single_ok<_TT, _Src> && __elems_constructible<_TT, __get_types_t<_Src>>;
template <class _TT, class _Src>
concept __tuple_from_other = __tuple_like<_Src> && !__is_tuple_specialization<std::remove_cvref_t<_Src>> &&
                           __tuple_like_single_ok<_TT, _Src> && __elems_constructible<_TT, __get_types_t<_Src>>;
};
}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class... _TTypes>
constexpr tuple<unwrap_ref_decay_t<_TTypes>...> make_tuple(_TTypes&&... t) {
}
template <class _Fp, __ycxx::__detail::__tuple_like _Tuple>
constexpr apply_result_t<_Fp, _Tuple> apply(_Fp&& __f, _Tuple&& t) noexcept(is_nothrow_applicable_v<_Fp, _Tuple>) {
}
}
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp, class _Up, std::size_t... _Ip>
consteval bool __tuple_eq_ok(std::index_sequence<_Ip...>*) {
  return (requires(const _Tp& t, const _Up& __u) {
    { get<_Ip>(t) == get<_Ip>(__u) } -> __boolean_testable;
  } && ...);
}
}
}
namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp, class _Alloc, class _Tuple1, class _Tuple2>
constexpr auto uses_allocator_construction_args(const _Alloc& __alloc, piecewise_construct_t, _Tuple1&& __x,
                                                _Tuple2&& y) noexcept {
}
}
inline int g(const std::u8string &s) { return s.size(); }
struct P { int a; std::string b; };
static const P v[] = { {g(u8"x"), std::string{'5', '6'}} };