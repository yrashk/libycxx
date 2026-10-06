// libycxx core: <mdspan> part 4, submdspan ([mdspan.sub]): the slice types, slice
// canonicalization, subextents, the submdspan_mapping customizations of the standard layouts
// and submdspan itself. Included at the end of mdspan.hpp.
//
// Slices are canonicalized once (canonical_slices) into full_extent_t, an index (IndexType or
// constant_wrapper) or an extent_slice of canonical index types; everything after that works on
// canonical slices only. Which layout a slice pattern keeps is decided at compile time from the
// slice types alone (md_sub_pick).
#pragma once

#include <ycxx/core/constant_wrapper.hpp>
#include <ycxx/core/mdspan.hpp>
#include <ycxx/core/tuple.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// [mdspan.sub.range.slices]
template <class _OffsetType, class _ExtentType, class _StrideType>
struct extent_slice {
  static_assert(__ycxx::__detail::__is_signed_or_unsigned_integer<_OffsetType> ||
                    __ycxx::__detail::__integral_constant_like<_OffsetType>,
                "extent_slice: OffsetType must be an integer type or integral-constant-like");
  static_assert(__ycxx::__detail::__is_signed_or_unsigned_integer<_ExtentType> ||
                    __ycxx::__detail::__integral_constant_like<_ExtentType>,
                "extent_slice: ExtentType must be an integer type or integral-constant-like");
  static_assert(__ycxx::__detail::__is_signed_or_unsigned_integer<_StrideType> ||
                    __ycxx::__detail::__integral_constant_like<_StrideType>,
                "extent_slice: StrideType must be an integer type or integral-constant-like");

  using offset_type = _OffsetType;
  using extent_type = _ExtentType;
  using stride_type = _StrideType;
  [[no_unique_address]] _OffsetType offset{};
  [[no_unique_address]] _ExtentType extent{};
  [[no_unique_address]] _StrideType stride{};
};

template <class _FirstType, class _LastType, class _StrideType = constant_wrapper<1zu>>
struct range_slice {
  static_assert(__ycxx::__detail::__is_signed_or_unsigned_integer<_FirstType> ||
                    __ycxx::__detail::__integral_constant_like<_FirstType>,
                "range_slice: FirstType must be an integer type or integral-constant-like");
  static_assert(__ycxx::__detail::__is_signed_or_unsigned_integer<_LastType> || __ycxx::__detail::__integral_constant_like<_LastType>,
                "range_slice: LastType must be an integer type or integral-constant-like");
  static_assert(__ycxx::__detail::__is_signed_or_unsigned_integer<_StrideType> ||
                    __ycxx::__detail::__integral_constant_like<_StrideType>,
                "range_slice: StrideType must be an integer type or integral-constant-like");

  [[no_unique_address]] _FirstType first{};
  [[no_unique_address]] _LastType last{};
  [[no_unique_address]] _StrideType stride{};
};

// [mdspan.sub.map.result]
template <class _LayoutMapping>
struct submdspan_mapping_result {
  [[no_unique_address]] _LayoutMapping mapping = _LayoutMapping();
  size_t offset{};
};

struct full_extent_t {
  explicit full_extent_t() = default;
};
inline constexpr full_extent_t full_extent{};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __md_is_extent_slice = false;
template <class _Op, class _Ep, class _Sp>
inline constexpr bool __md_is_extent_slice<std::extent_slice<_Op, _Ep, _Sp>> = true;
template <class _Tp>
inline constexpr bool __md_is_range_slice = false;
template <class _Fp, class _Lp, class _Sp>
inline constexpr bool __md_is_range_slice<std::range_slice<_Fp, _Lp, _Sp>> = true;
template <class _Tp>
inline constexpr bool __md_is_cw = false;
template <auto _Xp, class _Vp>
inline constexpr bool __md_is_cw<std::constant_wrapper<_Xp, _Vp>> = true;
template <class _Tp>
inline constexpr bool __md_is_mapping_result = false;
template <class _Mp>
inline constexpr bool __md_is_mapping_result<std::submdspan_mapping_result<_Mp>> = true;

// ---------------------------------------------------------------------------------------------
// Slices of two elements ([mdspan.sub.overview]/2.5): `auto [a, b] = std::move(s);` is valid.
// Detected for the tuple protocol and for aggregates initializable from exactly two values.
// ---------------------------------------------------------------------------------------------
struct __md_any_arg {
  template <class _Tp>
  operator _Tp() const;
};
template <class _Sp>
concept __md_tuple_protocol = requires { std::tuple_size<_Sp>::value; };
template <class _Sp>
concept __md_two_field_aggregate = std::is_aggregate_v<_Sp> && !std::is_array_v<_Sp> && !__md_tuple_protocol<_Sp> && requires {
  _Sp{__md_any_arg(), __md_any_arg()};
} && !requires { _Sp{__md_any_arg(), __md_any_arg(), __md_any_arg()}; };
template <class _Sp>
concept __md_two_bindable =
    std::is_class_v<_Sp> && ((__md_tuple_protocol<_Sp> && std::tuple_size_v<_Sp> == 2) || __md_two_field_aggregate<_Sp>);

template <class _Ap, class _Bp>
struct __md_type_pair {
  using first = _Ap;
  using second = _Bp;
};
template <class _Sp>
auto __md_binding_types(_Sp&& s) {
  auto [a, b] = static_cast<_Sp&&>(s);
  return __md_type_pair<decltype(std::move(a)), decltype(std::move(b))>();
}
template <class _Sp, class _IndexType>
concept __md_pair_slice = __md_two_bindable<_Sp> && requires {
  requires std::is_convertible_v<typename decltype(::__ycxx::__detail::__md_binding_types(std::declval<_Sp>()))::first,
                                 _IndexType>;
  requires std::is_convertible_v<typename decltype(::__ycxx::__detail::__md_binding_types(std::declval<_Sp>()))::second,
                                 _IndexType>;
};

// "S is a submdspan slice type for IndexType" ([mdspan.sub.overview]/2).
template <class _Sp, class _IndexType>
consteval bool __md_slice_type() {
  if constexpr (std::is_convertible_v<_Sp, std::full_extent_t> || std::is_convertible_v<_Sp, _IndexType>)
    return true;
  else if constexpr (__md_is_extent_slice<_Sp>)
    return std::is_convertible_v<typename _Sp::offset_type, _IndexType> &&
           std::is_convertible_v<typename _Sp::extent_type, _IndexType> &&
           std::is_convertible_v<typename _Sp::stride_type, _IndexType>;
  else if constexpr (__md_is_range_slice<_Sp>)
    return std::is_convertible_v<decltype(_Sp::first), _IndexType> &&
           std::is_convertible_v<decltype(_Sp::last), _IndexType> && std::is_convertible_v<decltype(_Sp::stride), _IndexType>;
  else
    return __md_pair_slice<_Sp, _IndexType>;
}

// A canonical submdspan index type for IndexType ([mdspan.sub.overview]/3).
template <class _Sp, class _IndexType>
consteval bool __md_canonical_index() {
  if constexpr (std::is_same_v<_Sp, _IndexType>)
    return true;
  else if constexpr (__md_is_cw<_Sp>)
    return std::is_same_v<std::remove_cvref_t<decltype(_Sp::value)>, _IndexType> && _Sp::value >= 0;
  else
    return false;
}
// A canonical submdspan slice type for IndexType ([mdspan.sub.overview]/4).
template <class _Sp, class _IndexType>
consteval bool __md_canonical_slice() {
  if constexpr (std::is_same_v<_Sp, std::full_extent_t>) {
    return true;
  } else if constexpr (__md_is_extent_slice<_Sp>) {
    if constexpr (__md_canonical_index<typename _Sp::offset_type, _IndexType>() &&
                  __md_canonical_index<typename _Sp::extent_type, _IndexType>() &&
                  __md_canonical_index<typename _Sp::stride_type, _IndexType>()) {
      if constexpr (__md_is_cw<typename _Sp::stride_type> && __md_is_cw<typename _Sp::extent_type>)
        return _Sp::stride_type::value > 0;
      else
        return true;
    } else {
      return false;
    }
  } else {
    return __md_canonical_index<_Sp, _IndexType>();
  }
}

template <class _Sp>
inline constexpr bool __md_is_full = std::is_same_v<_Sp, std::full_extent_t>;
// Collapsing and unit-stride slice types ([mdspan.sub.overview]/5-6).
template <class _Sp>
inline constexpr bool __md_collapsing = !__md_is_full<_Sp> && !__md_is_extent_slice<_Sp>;
template <class _Sp>
consteval bool __md_unit_stride() {
  if constexpr (__md_is_full<_Sp>)
    return true;
  else if constexpr (__md_is_extent_slice<_Sp>) {
    if constexpr (__md_is_cw<typename _Sp::stride_type>)
      return _Sp::stride_type::value == 1;
    else
      return false;
  } else
    return false;
}

// The value of a constant_wrapper type, or d.
template <class _Tp>
consteval std::size_t __md_cw_value_or(std::size_t d) {
  if constexpr (__md_is_cw<_Tp>)
    return static_cast<std::size_t>(_Tp::value);
  else
    return d;
}

// "S is a valid submdspan slice type for the kth extent of E" ([mdspan.sub.overview]/8).
template <class _Ep, std::size_t _Kp, class _Sp>
consteval bool __md_valid_slice_type() {
  using _Ip = typename _Ep::index_type;
  if constexpr (!__md_canonical_slice<_Sp, _Ip>()) {
    return false;
  } else {
    constexpr std::size_t __x = _Ep::static_extent(_Kp);
    if constexpr (__x == std::dynamic_extent) {
      return true;
    } else if constexpr (__md_is_extent_slice<_Sp>) {
      constexpr std::size_t __o = __md_cw_value_or<typename _Sp::offset_type>(0);
      constexpr std::size_t e = __md_cw_value_or<typename _Sp::extent_type>(0);
      constexpr std::size_t t = __md_cw_value_or<typename _Sp::stride_type>(1);
      if (__o > __x || e > __x || (e > 1 && t == 0))
        return false;
      return e == 0 || __o + 1 + (e - 1) * t <= __x;
    } else if constexpr (__md_is_cw<_Sp>) {
      return static_cast<std::size_t>(_Sp::value) < __x;
    } else {
      return true;
    }
  }
}

// The lower bound of the submdspan slice range of a canonical slice ([mdspan.sub.overview]/7).
template <class _Ip, class _Sp>
constexpr _Ip __md_slice_lower(const _Sp& s) noexcept {
  if constexpr (__md_is_full<_Sp>)
    return 0;
  else if constexpr (__md_is_extent_slice<_Sp>)
    return static_cast<_Ip>(s.offset);
  else
    return static_cast<_Ip>(s);
}

// "s is a valid submdspan slice for the kth extent of e" ([mdspan.sub.overview]/9), for a slice
// of a valid slice type.
template <class _Ep, class _Sp>
constexpr bool __md_valid_slice(const _Ep& e, std::size_t k, const _Sp& s) noexcept {
  using _Ip = typename _Ep::index_type;
  _Ip n = e.extent(k);
  if constexpr (__md_is_full<_Sp>) {
    return true;
  } else if constexpr (__md_is_extent_slice<_Sp>) {
    _Ip __o = static_cast<_Ip>(s.offset), __x = static_cast<_Ip>(s.extent), t = static_cast<_Ip>(s.stride);
    if (__x < 0 || (__x >= 2 && t <= 0) || __o < 0 || __o > n)
      return false;
    if (__x == 0)
      return true;
    _Ip __u = 0;
    return ::__ycxx::__detail::__md_mul(static_cast<_Ip>(__x - 1), t, __u) && ::__ycxx::__detail::__md_add(__u, __o, __u) && __u < n;
  } else {
    _Ip i = static_cast<_Ip>(s);
    return i >= 0 && i < n;
  }
}

// canonical-index ([mdspan.sub.helpers]/4-6).
template <class _IndexType, class _Sp>
constexpr auto __md_canonical_index_of(_Sp s) {
  if constexpr (__integral_constant_like<_Sp>) {
    static_assert(
        std::in_range<_IndexType>(::__ycxx::__detail::__md_as_int(::__ycxx::__detail::__md_index_cast<_IndexType>(_Sp::value))),
        "submdspan: a constant slice index is not representable as index_type");
    return std::cw<_IndexType(_Sp::value)>;
  } else {
    if constexpr (__md_plain_integral<_Sp>)
      ::__ycxx::__detail::__precondition(std::in_range<_IndexType>(::__ycxx::__detail::__md_as_int(s)),
                                   "submdspan: a slice index is not representable as index_type");
    return _IndexType(std::move(s));
  }
}

template <class _Dp, class... _Tp>
struct __md_first_type {
  using type = _Dp;
};
template <class _Dp, class _Tp, class... _Rp>
struct __md_first_type<_Dp, _Tp, _Rp...> {
  using type = _Tp;
};

// canonical-range-slice ([mdspan.sub.helpers]/7-10).
template <class _IndexType, class _OffsetType, class _SpanType, class... _StrideTypes>
constexpr auto __md_canonical_range_slice(_OffsetType offset, _SpanType span, _StrideTypes... strides) {
  static_assert(sizeof...(_StrideTypes) <= 1);
  constexpr bool __unit = sizeof...(_StrideTypes) == 0 || std::is_same_v<_SpanType, std::constant_wrapper<_IndexType(0)>>;
  using _StrideType =
      std::conditional_t<__unit, std::constant_wrapper<_IndexType(1)>,
                         typename __md_first_type<std::constant_wrapper<_IndexType(1)>, _StrideTypes...>::type>;
  _StrideType stride{};
  if constexpr (!__md_is_cw<_StrideType>) {
    if (span == 0)
      stride = _IndexType(1);
    else
      stride = (strides, ...);
    ::__ycxx::__detail::__precondition(stride > 0, "submdspan: a range_slice stride must be positive");
  } else {
    static_assert(_StrideType::value > 0, "submdspan: a range_slice stride must be positive");
  }
  if constexpr (__md_is_cw<_SpanType> && __md_is_cw<_StrideType>) {
    constexpr _IndexType value =
        _SpanType::value != 0 ? _IndexType(1 + (_SpanType::value - 1) / _StrideType::value) : _IndexType(0);
    return std::extent_slice<_OffsetType, std::constant_wrapper<value>, _StrideType>{offset, std::cw<value>, stride};
  } else {
    _IndexType value = span != 0 ? _IndexType(1 + (span - 1) / stride) : _IndexType(0);
    return std::extent_slice<_OffsetType, _IndexType, _StrideType>{offset, value, stride};
  }
}

// canonical-slice ([mdspan.sub.helpers]/11-12).
template <class _IndexType, class _Sp>
constexpr auto __md_canonical_slice_of(_Sp s) {
  static_assert(__md_slice_type<_Sp, _IndexType>(), "submdspan: not a submdspan slice type for index_type");
  if constexpr (std::is_convertible_v<_Sp, std::full_extent_t>) {
    return static_cast<std::full_extent_t>(std::move(s));
  } else if constexpr (std::is_convertible_v<_Sp, _IndexType>) {
    return ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(std::move(s));
  } else if constexpr (__md_is_extent_slice<_Sp>) {
    auto __o = ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(std::move(s.offset));
    auto e = ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(std::move(s.extent));
    auto t = ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(std::move(s.stride));
    return std::extent_slice<decltype(__o), decltype(e), decltype(t)>{__o, e, t};
  } else if constexpr (__md_is_range_slice<_Sp>) {
    auto __c_first = ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(std::move(s.first));
    auto __c_last = ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(std::move(s.last));
    return ::__ycxx::__detail::__md_canonical_range_slice<_IndexType>(
        __c_first, ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(__c_last - __c_first),
        ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(std::move(s.stride)));
  } else {
    auto [__s_first, __s_last] = std::move(s);
    auto __c_first = ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(std::move(__s_first));
    auto __c_last = ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(std::move(__s_last));
    return ::__ycxx::__detail::__md_canonical_range_slice<_IndexType>(
        __c_first, ::__ycxx::__detail::__md_canonical_index_of<_IndexType>(__c_last - __c_first));
  }
}

// Mandates and preconditions shared by canonical_slices and the submdspan_mapping
// customizations, for canonical slices.
template <class _Ep, class... _Sl>
constexpr void __md_check_slices(const _Ep& e, const _Sl&... __slices) {
  [&]<std::size_t... _Kp>(std::index_sequence<_Kp...>) {
    static_assert((__md_valid_slice_type<_Ep, _Kp, _Sl>() && ...),
                  "submdspan: a slice is not a valid submdspan slice type for its extent");
    if (::__ycxx::__detail::__md_checking())
      ::__ycxx::__detail::__precondition((::__ycxx::__detail::__md_valid_slice(e, _Kp, __slices) && ...),
                                   "submdspan: a slice is not a valid submdspan slice for its extent");
  }(std::index_sequence_for<_Sl...>());
}

// The rank of the result and the static extents of subextents ([mdspan.sub.extents]/5).
template <class... _Sl>
inline constexpr std::size_t __md_sub_rank = ((__md_collapsing<_Sl> ? 0 : 1) + ... + 0);
template <class _Sp>
consteval std::size_t __md_sub_static(std::size_t __x) {
  if constexpr (__md_is_full<_Sp>)
    return __x;
  else if constexpr (__md_is_extent_slice<_Sp>)
    return __md_cw_value_or<typename _Sp::extent_type>(std::dynamic_extent);
  else
    return std::dynamic_extent;
}
template <class _Ep, class... _Sl>
consteval std::array<std::size_t, __md_sub_rank<_Sl...> + 1> __md_sub_statics() {
  std::array<std::size_t, __md_sub_rank<_Sl...> + 1> r{};
  std::size_t k = 0, __j = 0;
  ((__md_collapsing<_Sl> ? void() : void(r[__j++] = __md_sub_static<_Sl>(_Ep::static_extent(k))), ++k), ...);
  return r;
}
template <class _Ep, class... _Sl>
inline constexpr auto __md_sub_statics_v = __md_sub_statics<_Ep, _Sl...>();

// subextents of canonical slices ([mdspan.sub.extents]/5-6).
template <class _Ep, class... _Sl>
constexpr auto __md_sub_extents(const _Ep& e, const _Sl&... __slices) {
  using _Ip = typename _Ep::index_type;
  constexpr std::size_t n = __md_sub_rank<_Sl...>;
  std::array<_Ip, n + 1> __vals{};
  std::size_t k = 0, __j = 0;
  (
      [&] {
        if constexpr (__md_is_full<_Sl>)
          __vals[__j++] = e.extent(k);
        else if constexpr (__md_is_extent_slice<_Sl>)
          __vals[__j++] = static_cast<_Ip>(__slices.extent);
        ++k;
      }(),
      ...);
  return [&]<std::size_t... _Jp>(std::index_sequence<_Jp...>) {
    return std::extents<_Ip, __md_sub_statics_v<_Ep, _Sl...>[_Jp]...>(__vals[_Jp]...);
  }(std::make_index_sequence<n>());
}

// sub_strides and offset of [mdspan.sub.map.common]/6-8.
template <class _SubExtents, class _Mp, class... _Sl>
constexpr std::array<typename _SubExtents::index_type, _SubExtents::rank()> __md_sub_strides(const _Mp& m,
                                                                                         const _Sl&... __slices) {
  using _Ip = typename _SubExtents::index_type;
  std::array<_Ip, _SubExtents::rank()> __st{};
  std::size_t k = 0, __j = 0;
  (
      [&] {
        if constexpr (!__md_collapsing<_Sl>) {
          _Ip s = static_cast<_Ip>(m.stride(k));
          if constexpr (__md_is_extent_slice<_Sl>)
            if (static_cast<_Ip>(__slices.extent) > 1)
              s = static_cast<_Ip>(s * static_cast<_Ip>(__slices.stride));
          __st[__j++] = s;
        }
        ++k;
      }(),
      ...);
  return __st;
}
template <class _Mp, class... _Sl>
constexpr std::size_t __md_sub_offset(const _Mp& m, const _Sl&... __slices) {
  using _Ip = typename _Mp::index_type;
  std::array<_Ip, sizeof...(_Sl)> __ls{::__ycxx::__detail::__md_slice_lower<_Ip>(__slices)...};
  for (std::size_t k = 0; k < sizeof...(_Sl); ++k)
    if (__ls[k] == m.extents().extent(k))
      return static_cast<std::size_t>(m.required_span_size());
  return [&]<std::size_t... _Kp>(std::index_sequence<_Kp...>) {
    return static_cast<std::size_t>(m(__ls[_Kp]...));
  }(std::index_sequence_for<_Sl...>());
}

// Which mapping a submdspan_mapping customization returns ([mdspan.sub.map.left] through
// [mdspan.sub.map.rightpad]): the source mapping itself, the plain layout, the padded layout
// (with u as defined there) or layout_stride.
enum class __md_sub_kind { __same, __plain, __padded, stride };
struct __md_sub_choice {
  __md_sub_kind kind;
  std::size_t __u;
};
template <bool _Left, bool _Padded, class... _Sl>
consteval __md_sub_choice __md_sub_pick() {
  constexpr std::size_t rank = sizeof...(_Sl), __sr = __md_sub_rank<_Sl...>;
  const bool __full[] = {__md_is_full<_Sl>..., false};
  const bool __unit[] = {__md_unit_stride<_Sl>()..., false};
  if (rank == 0)
    return {__md_sub_kind::__same, 0};
  if (__sr == 0 || (_Padded && rank == 1))
    return {__md_sub_kind::__plain, 0};
  if constexpr (_Left) {
    if constexpr (_Padded) {
      if (__sr == 1 && __unit[0])
        return {__md_sub_kind::__plain, 0};
    } else {
      bool c = __unit[__sr - 1];
      for (std::size_t k = 0; k + 1 < __sr; ++k)
        c = c && __full[k];
      if (c)
        return {__md_sub_kind::__plain, 0};
    }
    // u + 1: the smallest p > 0 for which the slice is unit-stride.
    std::size_t p = 1;
    while (p < rank && !__unit[p])
      ++p;
    if (p == rank)
      return {__md_sub_kind::stride, 0};
    std::size_t __u = p - 1;
    bool c = __unit[0] && __u + __sr - 1 < rank && __unit[__u + __sr - 1];
    for (std::size_t k = __u + 1; k + 1 < __u + __sr; ++k)
      c = c && __full[k];
    return {c ? __md_sub_kind::__padded : __md_sub_kind::stride, __u};
  } else {
    if constexpr (_Padded) {
      if (__sr == 1 && __unit[rank - 1])
        return {__md_sub_kind::__plain, 0};
    } else {
      bool c = __unit[rank - __sr];
      for (std::size_t k = rank - __sr + 1; k < rank; ++k)
        c = c && __full[k];
      if (c)
        return {__md_sub_kind::__plain, 0};
    }
    // rank - u - 2: the largest p < rank - 1 for which the slice is unit-stride.
    std::size_t p = rank - 1;
    while (p > 0 && !__unit[p - 1])
      --p;
    if (p == 0)
      return {__md_sub_kind::stride, 0};
    std::size_t __u = rank - p - 1; // p - 1 == rank - u - 2
    if (rank < __sr + __u)
      return {__md_sub_kind::stride, __u};
    bool c = __unit[rank - 1] && __unit[rank - __sr - __u];
    for (std::size_t k = rank - __sr - __u + 1; k + __u + 1 < rank; ++k)
      c = c && __full[k];
    return {c ? __md_sub_kind::__padded : __md_sub_kind::stride, __u};
  }
}

// S_static of [mdspan.sub.map.left]/1.4 and its relatives: the product of the static extents
// with rank indices in [first, last) times `__factor`, or dynamic_extent.
template <class _Ep>
consteval std::size_t __md_static_product(std::size_t __factor, std::size_t first, std::size_t last) {
  if (__factor == std::dynamic_extent)
    return std::dynamic_extent;
  std::size_t p = __factor;
  for (std::size_t k = first; k < last; ++k) {
    if (_Ep::static_extent(k) == std::dynamic_extent)
      return std::dynamic_extent;
    p *= _Ep::static_extent(k);
  }
  return p;
}

// The common body of the submdspan_mapping customizations. PadStride is the source's
// static-padding-stride (unused for layout_left and layout_right).
template <bool _Left, bool _Padded, std::size_t _PadStride, class _Mp, class... _Sl>
constexpr auto __md_submdspan_mapping(const _Mp& m, const _Sl&... __slices) {
  using _Ep = typename _Mp::extents_type;
  constexpr std::size_t rank = _Ep::rank();
  ::__ycxx::__detail::__md_check_slices(m.extents(), __slices...);
  constexpr __md_sub_choice __choice = __md_sub_pick<_Left, _Padded, _Sl...>();
  if constexpr (__choice.kind == __md_sub_kind::__same) {
    return std::submdspan_mapping_result<_Mp>{m, 0};
  } else {
    auto __sub_ext = ::__ycxx::__detail::__md_sub_extents(m.extents(), __slices...);
    using _Sub = decltype(__sub_ext);
    std::size_t offset = ::__ycxx::__detail::__md_sub_offset(m, __slices...);
    if constexpr (__choice.kind == __md_sub_kind::__plain) {
      using _Lp = std::conditional_t<_Left, std::layout_left, std::layout_right>;
      return std::submdspan_mapping_result<typename _Lp::template mapping<_Sub>>{
          typename _Lp::template mapping<_Sub>(__sub_ext), offset};
    } else if constexpr (__choice.kind == __md_sub_kind::__padded) {
      constexpr std::size_t __u = __choice.__u;
      if constexpr (_Left) {
        constexpr std::size_t __s_static =
            _Padded ? __md_static_product<_Ep>(_PadStride, 1, __u + 1) : __md_static_product<_Ep>(1, 0, __u + 1);
        using _Rp = typename std::layout_left_padded<__s_static>::template mapping<_Sub>;
        return std::submdspan_mapping_result<_Rp>{_Rp(__sub_ext, m.stride(__u + 1)), offset};
      } else {
        constexpr std::size_t __s_static = _Padded ? __md_static_product<_Ep>(_PadStride, rank - __u - 1, rank - 1)
                                                : __md_static_product<_Ep>(1, rank - __u - 1, rank);
        using _Rp = typename std::layout_right_padded<__s_static>::template mapping<_Sub>;
        return std::submdspan_mapping_result<_Rp>{_Rp(__sub_ext, m.stride(rank - __u - 2)), offset};
      }
    } else {
      using _Rp = std::layout_stride::mapping<_Sub>;
      return std::submdspan_mapping_result<_Rp>{
          _Rp(__md_sub_strides_t{}, __sub_ext, ::__ycxx::__detail::__md_sub_strides<_Sub>(m, __slices...)), offset};
    }
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [mdspan.sub.map.left] ... [mdspan.sub.map.rightpad]
template <class _Extents>
template <class... _SliceSpecifiers>
constexpr auto layout_left::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  return __ycxx::__detail::__md_submdspan_mapping<true, false, 0>(*this, __slices...);
}
template <class _Extents>
template <class... _SliceSpecifiers>
constexpr auto layout_right::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  return __ycxx::__detail::__md_submdspan_mapping<false, false, 0>(*this, __slices...);
}
template <class _Extents>
template <class... _SliceSpecifiers>
constexpr auto layout_stride::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  __ycxx::__detail::__md_check_slices(extents(), __slices...);
  if constexpr (__rank_ == 0) {
    return submdspan_mapping_result<mapping>{*this, 0};
  } else {
    auto __sub_ext = __ycxx::__detail::__md_sub_extents(extents(), __slices...);
    using _Rp = layout_stride::mapping<decltype(__sub_ext)>;
    return submdspan_mapping_result<_Rp>{_Rp(__ycxx::__detail::__md_sub_strides_t{}, __sub_ext,
                                         __ycxx::__detail::__md_sub_strides<decltype(__sub_ext)>(*this, __slices...)),
                                       __ycxx::__detail::__md_sub_offset(*this, __slices...)};
  }
}
template <size_t _PaddingValue>
template <class _Extents>
template <class... _SliceSpecifiers>
constexpr auto
layout_left_padded<_PaddingValue>::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  return __ycxx::__detail::__md_submdspan_mapping<true, true, __static_padding_stride>(*this, __slices...);
}
template <size_t _PaddingValue>
template <class _Extents>
template <class... _SliceSpecifiers>
constexpr auto
layout_right_padded<_PaddingValue>::mapping<_Extents>::__submdspan_mapping_impl(_SliceSpecifiers... __slices) const {
  return __ycxx::__detail::__md_submdspan_mapping<false, true, __static_padding_stride>(*this, __slices...);
}

// [mdspan.sub.canonical]
template <class _IndexType, size_t... _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == sizeof...(_Extents))
constexpr auto canonical_slices(const extents<_IndexType, _Extents...>& __src, _SliceSpecifiers... __slices) {
  auto t = std::make_tuple(__ycxx::__detail::__md_canonical_slice_of<_IndexType>(std::move(__slices))...);
  [&]<size_t... _Kp>(index_sequence<_Kp...>) {
    __ycxx::__detail::__md_check_slices(__src, std::get<_Kp>(t)...);
  }(index_sequence_for<_SliceSpecifiers...>());
  return t;
}

// [mdspan.sub.extents]
template <class _IndexType, size_t... _Extents, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == sizeof...(_Extents))
constexpr auto subextents(const extents<_IndexType, _Extents...>& __src, _SliceSpecifiers... __raw_slices) {
  auto t = std::canonical_slices(__src, std::move(__raw_slices)...);
  return [&]<size_t... _Kp>(index_sequence<_Kp...>) {
    return __ycxx::__detail::__md_sub_extents(__src, std::get<_Kp>(t)...);
  }(index_sequence_for<_SliceSpecifiers...>());
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__md_adl {
// sliceable-mapping ([mdspan.sub.map.sliceable]/6): submdspan_mapping found by argument-dependent
// lookup only (no declaration of that name is visible from here).
template <class _LM, std::size_t... _Ip>
auto __md_sub_map_full(const _LM& __lm,
                     std::index_sequence<_Ip...>) -> decltype(submdspan_mapping(__lm, ((void)_Ip, std::full_extent)...));
template <class _LM>
concept __sliceable_mapping = requires(const _LM& __lm) {
  __md_sub_map_full(__lm, std::make_index_sequence<_LM::extents_type::rank()>());
  requires ::__ycxx::__detail::__md_is_mapping_result<decltype(__md_sub_map_full(
      __lm, std::make_index_sequence<_LM::extents_type::rank()>()))>;
};
// The customization point call of submdspan ([mdspan.sub.sub]/3, Note 1).
template <class _LM, class... _Sl>
constexpr auto __call_submdspan_mapping(const _LM& __lm, const _Sl&... __slices) {
  return submdspan_mapping(__lm, __slices...);
}
}} // namespace __ycxx::__detail::__md_adl

namespace [[__gnu__::__visibility__("hidden")]] std {

// [mdspan.sub.sub]
template <class _ElementType, class _Extents, class _LayoutPolicy, class _AccessorPolicy, class... _SliceSpecifiers>
  requires(sizeof...(_SliceSpecifiers) == _Extents::rank() &&
           __ycxx::__detail::__md_adl::__sliceable_mapping<typename _LayoutPolicy::template mapping<_Extents>>)
constexpr auto submdspan(const mdspan<_ElementType, _Extents, _LayoutPolicy, _AccessorPolicy>& __src,
                         _SliceSpecifiers... __raw_slices) {
  auto t = std::canonical_slices(__src.extents(), std::move(__raw_slices)...);
  return [&]<size_t... _Kp>(index_sequence<_Kp...>) {
    auto __sub_map_result = __ycxx::__detail::__md_adl::__call_submdspan_mapping(__src.mapping(), std::get<_Kp>(t)...);
    using _Ap = typename _AccessorPolicy::offset_policy;
    using _Rp = decltype(__sub_map_result.mapping);
    return mdspan<typename _Ap::element_type, typename _Rp::extents_type, typename _Rp::layout_type, _Ap>(
        __src.accessor().offset(__src.data_handle(), __sub_map_result.offset), __sub_map_result.mapping, _Ap(__src.accessor()));
  }(index_sequence_for<_SliceSpecifiers...>());
}

} // namespace std
