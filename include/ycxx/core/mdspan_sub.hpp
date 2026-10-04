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

namespace std {

// [mdspan.sub.range.slices]
template <class OffsetType, class ExtentType, class StrideType>
struct extent_slice {
  static_assert(ycxx::detail::is_signed_or_unsigned_integer<OffsetType> ||
                    ycxx::detail::integral_constant_like<OffsetType>,
                "extent_slice: OffsetType must be an integer type or integral-constant-like");
  static_assert(ycxx::detail::is_signed_or_unsigned_integer<ExtentType> ||
                    ycxx::detail::integral_constant_like<ExtentType>,
                "extent_slice: ExtentType must be an integer type or integral-constant-like");
  static_assert(ycxx::detail::is_signed_or_unsigned_integer<StrideType> ||
                    ycxx::detail::integral_constant_like<StrideType>,
                "extent_slice: StrideType must be an integer type or integral-constant-like");

  using offset_type = OffsetType;
  using extent_type = ExtentType;
  using stride_type = StrideType;
  [[no_unique_address]] OffsetType offset{};
  [[no_unique_address]] ExtentType extent{};
  [[no_unique_address]] StrideType stride{};
};

template <class FirstType, class LastType, class StrideType = constant_wrapper<1zu>>
struct range_slice {
  static_assert(ycxx::detail::is_signed_or_unsigned_integer<FirstType> ||
                    ycxx::detail::integral_constant_like<FirstType>,
                "range_slice: FirstType must be an integer type or integral-constant-like");
  static_assert(ycxx::detail::is_signed_or_unsigned_integer<LastType> || ycxx::detail::integral_constant_like<LastType>,
                "range_slice: LastType must be an integer type or integral-constant-like");
  static_assert(ycxx::detail::is_signed_or_unsigned_integer<StrideType> ||
                    ycxx::detail::integral_constant_like<StrideType>,
                "range_slice: StrideType must be an integer type or integral-constant-like");

  [[no_unique_address]] FirstType first{};
  [[no_unique_address]] LastType last{};
  [[no_unique_address]] StrideType stride{};
};

// [mdspan.sub.map.result]
template <class LayoutMapping>
struct submdspan_mapping_result {
  [[no_unique_address]] LayoutMapping mapping = LayoutMapping();
  size_t offset{};
};

struct full_extent_t {
  explicit full_extent_t() = default;
};
inline constexpr full_extent_t full_extent{};

} // namespace std

namespace ycxx::detail {

template <class T>
inline constexpr bool md_is_extent_slice = false;
template <class O, class E, class S>
inline constexpr bool md_is_extent_slice<std::extent_slice<O, E, S>> = true;
template <class T>
inline constexpr bool md_is_range_slice = false;
template <class F, class L, class S>
inline constexpr bool md_is_range_slice<std::range_slice<F, L, S>> = true;
template <class T>
inline constexpr bool md_is_cw = false;
template <auto X, class V>
inline constexpr bool md_is_cw<std::constant_wrapper<X, V>> = true;
template <class T>
inline constexpr bool md_is_mapping_result = false;
template <class M>
inline constexpr bool md_is_mapping_result<std::submdspan_mapping_result<M>> = true;

// ---------------------------------------------------------------------------------------------
// Slices of two elements ([mdspan.sub.overview]/2.5): `auto [a, b] = std::move(s);` is valid.
// Detected for the tuple protocol and for aggregates initializable from exactly two values.
// ---------------------------------------------------------------------------------------------
struct md_any_arg {
  template <class T>
  operator T() const;
};
template <class S>
concept md_tuple_protocol = requires { std::tuple_size<S>::value; };
template <class S>
concept md_two_field_aggregate = std::is_aggregate_v<S> && !std::is_array_v<S> && !md_tuple_protocol<S> && requires {
  S{md_any_arg(), md_any_arg()};
} && !requires { S{md_any_arg(), md_any_arg(), md_any_arg()}; };
template <class S>
concept md_two_bindable =
    std::is_class_v<S> && ((md_tuple_protocol<S> && std::tuple_size_v<S> == 2) || md_two_field_aggregate<S>);

template <class A, class B>
struct md_type_pair {
  using first = A;
  using second = B;
};
template <class S>
auto md_binding_types(S&& s) {
  auto [a, b] = static_cast<S&&>(s);
  return md_type_pair<decltype(std::move(a)), decltype(std::move(b))>();
}
template <class S, class IndexType>
concept md_pair_slice = md_two_bindable<S> && requires {
  requires std::is_convertible_v<typename decltype(::ycxx::detail::md_binding_types(std::declval<S>()))::first,
                                 IndexType>;
  requires std::is_convertible_v<typename decltype(::ycxx::detail::md_binding_types(std::declval<S>()))::second,
                                 IndexType>;
};

// "S is a submdspan slice type for IndexType" ([mdspan.sub.overview]/2).
template <class S, class IndexType>
consteval bool md_slice_type() {
  if constexpr (std::is_convertible_v<S, std::full_extent_t> || std::is_convertible_v<S, IndexType>)
    return true;
  else if constexpr (md_is_extent_slice<S>)
    return std::is_convertible_v<typename S::offset_type, IndexType> &&
           std::is_convertible_v<typename S::extent_type, IndexType> &&
           std::is_convertible_v<typename S::stride_type, IndexType>;
  else if constexpr (md_is_range_slice<S>)
    return std::is_convertible_v<decltype(S::first), IndexType> &&
           std::is_convertible_v<decltype(S::last), IndexType> && std::is_convertible_v<decltype(S::stride), IndexType>;
  else
    return md_pair_slice<S, IndexType>;
}

// A canonical submdspan index type for IndexType ([mdspan.sub.overview]/3).
template <class S, class IndexType>
consteval bool md_canonical_index() {
  if constexpr (std::is_same_v<S, IndexType>)
    return true;
  else if constexpr (md_is_cw<S>)
    return std::is_same_v<std::remove_cvref_t<decltype(S::value)>, IndexType> && S::value >= 0;
  else
    return false;
}
// A canonical submdspan slice type for IndexType ([mdspan.sub.overview]/4).
template <class S, class IndexType>
consteval bool md_canonical_slice() {
  if constexpr (std::is_same_v<S, std::full_extent_t>) {
    return true;
  } else if constexpr (md_is_extent_slice<S>) {
    if constexpr (md_canonical_index<typename S::offset_type, IndexType>() &&
                  md_canonical_index<typename S::extent_type, IndexType>() &&
                  md_canonical_index<typename S::stride_type, IndexType>()) {
      if constexpr (md_is_cw<typename S::stride_type> && md_is_cw<typename S::extent_type>)
        return S::stride_type::value > 0;
      else
        return true;
    } else {
      return false;
    }
  } else {
    return md_canonical_index<S, IndexType>();
  }
}

template <class S>
inline constexpr bool md_is_full = std::is_same_v<S, std::full_extent_t>;
// Collapsing and unit-stride slice types ([mdspan.sub.overview]/5-6).
template <class S>
inline constexpr bool md_collapsing = !md_is_full<S> && !md_is_extent_slice<S>;
template <class S>
consteval bool md_unit_stride() {
  if constexpr (md_is_full<S>)
    return true;
  else if constexpr (md_is_extent_slice<S>) {
    if constexpr (md_is_cw<typename S::stride_type>)
      return S::stride_type::value == 1;
    else
      return false;
  } else
    return false;
}

// The value of a constant_wrapper type, or d.
template <class T>
consteval std::size_t md_cw_value_or(std::size_t d) {
  if constexpr (md_is_cw<T>)
    return static_cast<std::size_t>(T::value);
  else
    return d;
}

// "S is a valid submdspan slice type for the kth extent of E" ([mdspan.sub.overview]/8).
template <class E, std::size_t K, class S>
consteval bool md_valid_slice_type() {
  using I = typename E::index_type;
  if constexpr (!md_canonical_slice<S, I>()) {
    return false;
  } else {
    constexpr std::size_t x = E::static_extent(K);
    if constexpr (x == std::dynamic_extent) {
      return true;
    } else if constexpr (md_is_extent_slice<S>) {
      constexpr std::size_t o = md_cw_value_or<typename S::offset_type>(0);
      constexpr std::size_t e = md_cw_value_or<typename S::extent_type>(0);
      constexpr std::size_t t = md_cw_value_or<typename S::stride_type>(1);
      if (o > x || e > x || (e > 1 && t == 0))
        return false;
      return e == 0 || o + 1 + (e - 1) * t <= x;
    } else if constexpr (md_is_cw<S>) {
      return static_cast<std::size_t>(S::value) < x;
    } else {
      return true;
    }
  }
}

// The lower bound of the submdspan slice range of a canonical slice ([mdspan.sub.overview]/7).
template <class I, class S>
constexpr I md_slice_lower(const S& s) noexcept {
  if constexpr (md_is_full<S>)
    return 0;
  else if constexpr (md_is_extent_slice<S>)
    return static_cast<I>(s.offset);
  else
    return static_cast<I>(s);
}

// "s is a valid submdspan slice for the kth extent of e" ([mdspan.sub.overview]/9), for a slice
// of a valid slice type.
template <class E, class S>
constexpr bool md_valid_slice(const E& e, std::size_t k, const S& s) noexcept {
  using I = typename E::index_type;
  I n = e.extent(k);
  if constexpr (md_is_full<S>) {
    return true;
  } else if constexpr (md_is_extent_slice<S>) {
    I o = static_cast<I>(s.offset), x = static_cast<I>(s.extent), t = static_cast<I>(s.stride);
    if (x < 0 || (x >= 2 && t <= 0) || o < 0 || o > n)
      return false;
    if (x == 0)
      return true;
    I u = 0;
    return ::ycxx::detail::md_mul(static_cast<I>(x - 1), t, u) && ::ycxx::detail::md_add(u, o, u) && u < n;
  } else {
    I i = static_cast<I>(s);
    return i >= 0 && i < n;
  }
}

// canonical-index ([mdspan.sub.helpers]/4-6).
template <class IndexType, class S>
constexpr auto md_canonical_index_of(S s) {
  if constexpr (integral_constant_like<S>) {
    static_assert(
        std::in_range<IndexType>(::ycxx::detail::md_as_int(::ycxx::detail::md_index_cast<IndexType>(S::value))),
        "submdspan: a constant slice index is not representable as index_type");
    return std::cw<IndexType(S::value)>;
  } else {
    if constexpr (md_plain_integral<S>)
      ::ycxx::detail::precondition(std::in_range<IndexType>(::ycxx::detail::md_as_int(s)),
                                   "submdspan: a slice index is not representable as index_type");
    return IndexType(std::move(s));
  }
}

template <class D, class... T>
struct md_first_type {
  using type = D;
};
template <class D, class T, class... R>
struct md_first_type<D, T, R...> {
  using type = T;
};

// canonical-range-slice ([mdspan.sub.helpers]/7-10).
template <class IndexType, class OffsetType, class SpanType, class... StrideTypes>
constexpr auto md_canonical_range_slice(OffsetType offset, SpanType span, StrideTypes... strides) {
  static_assert(sizeof...(StrideTypes) <= 1);
  constexpr bool unit = sizeof...(StrideTypes) == 0 || std::is_same_v<SpanType, std::constant_wrapper<IndexType(0)>>;
  using StrideType =
      std::conditional_t<unit, std::constant_wrapper<IndexType(1)>,
                         typename md_first_type<std::constant_wrapper<IndexType(1)>, StrideTypes...>::type>;
  StrideType stride{};
  if constexpr (!md_is_cw<StrideType>) {
    if (span == 0)
      stride = IndexType(1);
    else
      stride = (strides, ...);
    ::ycxx::detail::precondition(stride > 0, "submdspan: a range_slice stride must be positive");
  } else {
    static_assert(StrideType::value > 0, "submdspan: a range_slice stride must be positive");
  }
  if constexpr (md_is_cw<SpanType> && md_is_cw<StrideType>) {
    constexpr IndexType value =
        SpanType::value != 0 ? IndexType(1 + (SpanType::value - 1) / StrideType::value) : IndexType(0);
    return std::extent_slice<OffsetType, std::constant_wrapper<value>, StrideType>{offset, std::cw<value>, stride};
  } else {
    IndexType value = span != 0 ? IndexType(1 + (span - 1) / stride) : IndexType(0);
    return std::extent_slice<OffsetType, IndexType, StrideType>{offset, value, stride};
  }
}

// canonical-slice ([mdspan.sub.helpers]/11-12).
template <class IndexType, class S>
constexpr auto md_canonical_slice_of(S s) {
  static_assert(md_slice_type<S, IndexType>(), "submdspan: not a submdspan slice type for index_type");
  if constexpr (std::is_convertible_v<S, std::full_extent_t>) {
    return static_cast<std::full_extent_t>(std::move(s));
  } else if constexpr (std::is_convertible_v<S, IndexType>) {
    return ::ycxx::detail::md_canonical_index_of<IndexType>(std::move(s));
  } else if constexpr (md_is_extent_slice<S>) {
    auto o = ::ycxx::detail::md_canonical_index_of<IndexType>(std::move(s.offset));
    auto e = ::ycxx::detail::md_canonical_index_of<IndexType>(std::move(s.extent));
    auto t = ::ycxx::detail::md_canonical_index_of<IndexType>(std::move(s.stride));
    return std::extent_slice<decltype(o), decltype(e), decltype(t)>{o, e, t};
  } else if constexpr (md_is_range_slice<S>) {
    auto c_first = ::ycxx::detail::md_canonical_index_of<IndexType>(std::move(s.first));
    auto c_last = ::ycxx::detail::md_canonical_index_of<IndexType>(std::move(s.last));
    return ::ycxx::detail::md_canonical_range_slice<IndexType>(
        c_first, ::ycxx::detail::md_canonical_index_of<IndexType>(c_last - c_first),
        ::ycxx::detail::md_canonical_index_of<IndexType>(std::move(s.stride)));
  } else {
    auto [s_first, s_last] = std::move(s);
    auto c_first = ::ycxx::detail::md_canonical_index_of<IndexType>(std::move(s_first));
    auto c_last = ::ycxx::detail::md_canonical_index_of<IndexType>(std::move(s_last));
    return ::ycxx::detail::md_canonical_range_slice<IndexType>(
        c_first, ::ycxx::detail::md_canonical_index_of<IndexType>(c_last - c_first));
  }
}

// Mandates and preconditions shared by canonical_slices and the submdspan_mapping
// customizations, for canonical slices.
template <class E, class... Sl>
constexpr void md_check_slices(const E& e, const Sl&... slices) {
  [&]<std::size_t... K>(std::index_sequence<K...>) {
    static_assert((md_valid_slice_type<E, K, Sl>() && ...),
                  "submdspan: a slice is not a valid submdspan slice type for its extent");
    if (::ycxx::detail::md_checking())
      ::ycxx::detail::precondition((::ycxx::detail::md_valid_slice(e, K, slices) && ...),
                                   "submdspan: a slice is not a valid submdspan slice for its extent");
  }(std::index_sequence_for<Sl...>());
}

// The rank of the result and the static extents of subextents ([mdspan.sub.extents]/5).
template <class... Sl>
inline constexpr std::size_t md_sub_rank = ((md_collapsing<Sl> ? 0 : 1) + ... + 0);
template <class S>
consteval std::size_t md_sub_static(std::size_t x) {
  if constexpr (md_is_full<S>)
    return x;
  else if constexpr (md_is_extent_slice<S>)
    return md_cw_value_or<typename S::extent_type>(std::dynamic_extent);
  else
    return std::dynamic_extent;
}
template <class E, class... Sl>
consteval std::array<std::size_t, md_sub_rank<Sl...> + 1> md_sub_statics() {
  std::array<std::size_t, md_sub_rank<Sl...> + 1> r{};
  std::size_t k = 0, j = 0;
  ((md_collapsing<Sl> ? void() : void(r[j++] = md_sub_static<Sl>(E::static_extent(k))), ++k), ...);
  return r;
}
template <class E, class... Sl>
inline constexpr auto md_sub_statics_v = md_sub_statics<E, Sl...>();

// subextents of canonical slices ([mdspan.sub.extents]/5-6).
template <class E, class... Sl>
constexpr auto md_sub_extents(const E& e, const Sl&... slices) {
  using I = typename E::index_type;
  constexpr std::size_t n = md_sub_rank<Sl...>;
  std::array<I, n + 1> vals{};
  std::size_t k = 0, j = 0;
  (
      [&] {
        if constexpr (md_is_full<Sl>)
          vals[j++] = e.extent(k);
        else if constexpr (md_is_extent_slice<Sl>)
          vals[j++] = static_cast<I>(slices.extent);
        ++k;
      }(),
      ...);
  return [&]<std::size_t... J>(std::index_sequence<J...>) {
    return std::extents<I, md_sub_statics_v<E, Sl...>[J]...>(vals[J]...);
  }(std::make_index_sequence<n>());
}

// sub_strides and offset of [mdspan.sub.map.common]/6-8.
template <class SubExtents, class M, class... Sl>
constexpr std::array<typename SubExtents::index_type, SubExtents::rank()> md_sub_strides(const M& m,
                                                                                         const Sl&... slices) {
  using I = typename SubExtents::index_type;
  std::array<I, SubExtents::rank()> st{};
  std::size_t k = 0, j = 0;
  (
      [&] {
        if constexpr (!md_collapsing<Sl>) {
          I s = static_cast<I>(m.stride(k));
          if constexpr (md_is_extent_slice<Sl>)
            if (static_cast<I>(slices.extent) > 1)
              s = static_cast<I>(s * static_cast<I>(slices.stride));
          st[j++] = s;
        }
        ++k;
      }(),
      ...);
  return st;
}
template <class M, class... Sl>
constexpr std::size_t md_sub_offset(const M& m, const Sl&... slices) {
  using I = typename M::index_type;
  std::array<I, sizeof...(Sl)> ls{::ycxx::detail::md_slice_lower<I>(slices)...};
  for (std::size_t k = 0; k < sizeof...(Sl); ++k)
    if (ls[k] == m.extents().extent(k))
      return static_cast<std::size_t>(m.required_span_size());
  return [&]<std::size_t... K>(std::index_sequence<K...>) {
    return static_cast<std::size_t>(m(ls[K]...));
  }(std::index_sequence_for<Sl...>());
}

// Which mapping a submdspan_mapping customization returns ([mdspan.sub.map.left] through
// [mdspan.sub.map.rightpad]): the source mapping itself, the plain layout, the padded layout
// (with u as defined there) or layout_stride.
enum class md_sub_kind { same, plain, padded, stride };
struct md_sub_choice {
  md_sub_kind kind;
  std::size_t u;
};
template <bool Left, bool Padded, class... Sl>
consteval md_sub_choice md_sub_pick() {
  constexpr std::size_t rank = sizeof...(Sl), sr = md_sub_rank<Sl...>;
  const bool full[] = {md_is_full<Sl>..., false};
  const bool unit[] = {md_unit_stride<Sl>()..., false};
  if (rank == 0)
    return {md_sub_kind::same, 0};
  if (sr == 0 || (Padded && rank == 1))
    return {md_sub_kind::plain, 0};
  if constexpr (Left) {
    if constexpr (Padded) {
      if (sr == 1 && unit[0])
        return {md_sub_kind::plain, 0};
    } else {
      bool c = unit[sr - 1];
      for (std::size_t k = 0; k + 1 < sr; ++k)
        c = c && full[k];
      if (c)
        return {md_sub_kind::plain, 0};
    }
    // u + 1: the smallest p > 0 for which the slice is unit-stride.
    std::size_t p = 1;
    while (p < rank && !unit[p])
      ++p;
    if (p == rank)
      return {md_sub_kind::stride, 0};
    std::size_t u = p - 1;
    bool c = unit[0] && u + sr - 1 < rank && unit[u + sr - 1];
    for (std::size_t k = u + 1; k + 1 < u + sr; ++k)
      c = c && full[k];
    return {c ? md_sub_kind::padded : md_sub_kind::stride, u};
  } else {
    if constexpr (Padded) {
      if (sr == 1 && unit[rank - 1])
        return {md_sub_kind::plain, 0};
    } else {
      bool c = unit[rank - sr];
      for (std::size_t k = rank - sr + 1; k < rank; ++k)
        c = c && full[k];
      if (c)
        return {md_sub_kind::plain, 0};
    }
    // rank - u - 2: the largest p < rank - 1 for which the slice is unit-stride.
    std::size_t p = rank - 1;
    while (p > 0 && !unit[p - 1])
      --p;
    if (p == 0)
      return {md_sub_kind::stride, 0};
    std::size_t u = rank - p - 1; // p - 1 == rank - u - 2
    if (rank < sr + u)
      return {md_sub_kind::stride, u};
    bool c = unit[rank - 1] && unit[rank - sr - u];
    for (std::size_t k = rank - sr - u + 1; k + u + 1 < rank; ++k)
      c = c && full[k];
    return {c ? md_sub_kind::padded : md_sub_kind::stride, u};
  }
}

// S_static of [mdspan.sub.map.left]/1.4 and its relatives: the product of the static extents
// with rank indices in [first, last) times `factor`, or dynamic_extent.
template <class E>
consteval std::size_t md_static_product(std::size_t factor, std::size_t first, std::size_t last) {
  if (factor == std::dynamic_extent)
    return std::dynamic_extent;
  std::size_t p = factor;
  for (std::size_t k = first; k < last; ++k) {
    if (E::static_extent(k) == std::dynamic_extent)
      return std::dynamic_extent;
    p *= E::static_extent(k);
  }
  return p;
}

// The common body of the submdspan_mapping customizations. PadStride is the source's
// static-padding-stride (unused for layout_left and layout_right).
template <bool Left, bool Padded, std::size_t PadStride, class M, class... Sl>
constexpr auto md_submdspan_mapping(const M& m, const Sl&... slices) {
  using E = typename M::extents_type;
  constexpr std::size_t rank = E::rank();
  ::ycxx::detail::md_check_slices(m.extents(), slices...);
  constexpr md_sub_choice choice = md_sub_pick<Left, Padded, Sl...>();
  if constexpr (choice.kind == md_sub_kind::same) {
    return std::submdspan_mapping_result<M>{m, 0};
  } else {
    auto sub_ext = ::ycxx::detail::md_sub_extents(m.extents(), slices...);
    using Sub = decltype(sub_ext);
    std::size_t offset = ::ycxx::detail::md_sub_offset(m, slices...);
    if constexpr (choice.kind == md_sub_kind::plain) {
      using L = std::conditional_t<Left, std::layout_left, std::layout_right>;
      return std::submdspan_mapping_result<typename L::template mapping<Sub>>{
          typename L::template mapping<Sub>(sub_ext), offset};
    } else if constexpr (choice.kind == md_sub_kind::padded) {
      constexpr std::size_t u = choice.u;
      if constexpr (Left) {
        constexpr std::size_t s_static =
            Padded ? md_static_product<E>(PadStride, 1, u + 1) : md_static_product<E>(1, 0, u + 1);
        using R = typename std::layout_left_padded<s_static>::template mapping<Sub>;
        return std::submdspan_mapping_result<R>{R(sub_ext, m.stride(u + 1)), offset};
      } else {
        constexpr std::size_t s_static = Padded ? md_static_product<E>(PadStride, rank - u - 1, rank - 1)
                                                : md_static_product<E>(1, rank - u - 1, rank);
        using R = typename std::layout_right_padded<s_static>::template mapping<Sub>;
        return std::submdspan_mapping_result<R>{R(sub_ext, m.stride(rank - u - 2)), offset};
      }
    } else {
      using R = std::layout_stride::mapping<Sub>;
      return std::submdspan_mapping_result<R>{R(sub_ext, ::ycxx::detail::md_sub_strides<Sub>(m, slices...)), offset};
    }
  }
}

} // namespace ycxx::detail

namespace std {

// [mdspan.sub.map.left] ... [mdspan.sub.map.rightpad]
template <class Extents>
template <class... SliceSpecifiers>
constexpr auto layout_left::mapping<Extents>::submdspan_mapping_impl(SliceSpecifiers... slices) const {
  return ycxx::detail::md_submdspan_mapping<true, false, 0>(*this, slices...);
}
template <class Extents>
template <class... SliceSpecifiers>
constexpr auto layout_right::mapping<Extents>::submdspan_mapping_impl(SliceSpecifiers... slices) const {
  return ycxx::detail::md_submdspan_mapping<false, false, 0>(*this, slices...);
}
template <class Extents>
template <class... SliceSpecifiers>
constexpr auto layout_stride::mapping<Extents>::submdspan_mapping_impl(SliceSpecifiers... slices) const {
  ycxx::detail::md_check_slices(extents(), slices...);
  if constexpr (rank_ == 0) {
    return submdspan_mapping_result<mapping>{*this, 0};
  } else {
    auto sub_ext = ycxx::detail::md_sub_extents(extents(), slices...);
    using R = layout_stride::mapping<decltype(sub_ext)>;
    return submdspan_mapping_result<R>{R(sub_ext, ycxx::detail::md_sub_strides<decltype(sub_ext)>(*this, slices...)),
                                       ycxx::detail::md_sub_offset(*this, slices...)};
  }
}
template <size_t PaddingValue>
template <class Extents>
template <class... SliceSpecifiers>
constexpr auto
layout_left_padded<PaddingValue>::mapping<Extents>::submdspan_mapping_impl(SliceSpecifiers... slices) const {
  return ycxx::detail::md_submdspan_mapping<true, true, static_padding_stride>(*this, slices...);
}
template <size_t PaddingValue>
template <class Extents>
template <class... SliceSpecifiers>
constexpr auto
layout_right_padded<PaddingValue>::mapping<Extents>::submdspan_mapping_impl(SliceSpecifiers... slices) const {
  return ycxx::detail::md_submdspan_mapping<false, true, static_padding_stride>(*this, slices...);
}

// [mdspan.sub.canonical]
template <class IndexType, size_t... Extents, class... SliceSpecifiers>
  requires(sizeof...(SliceSpecifiers) == sizeof...(Extents))
constexpr auto canonical_slices(const extents<IndexType, Extents...>& src, SliceSpecifiers... slices) {
  auto t = std::make_tuple(ycxx::detail::md_canonical_slice_of<IndexType>(std::move(slices))...);
  [&]<size_t... K>(index_sequence<K...>) {
    ycxx::detail::md_check_slices(src, std::get<K>(t)...);
  }(index_sequence_for<SliceSpecifiers...>());
  return t;
}

// [mdspan.sub.extents]
template <class IndexType, size_t... Extents, class... SliceSpecifiers>
  requires(sizeof...(SliceSpecifiers) == sizeof...(Extents))
constexpr auto subextents(const extents<IndexType, Extents...>& src, SliceSpecifiers... raw_slices) {
  auto t = std::canonical_slices(src, std::move(raw_slices)...);
  return [&]<size_t... K>(index_sequence<K...>) {
    return ycxx::detail::md_sub_extents(src, std::get<K>(t)...);
  }(index_sequence_for<SliceSpecifiers...>());
}

} // namespace std

namespace ycxx::detail::md_adl {
// sliceable-mapping ([mdspan.sub.map.sliceable]/6): submdspan_mapping found by argument-dependent
// lookup only (no declaration of that name is visible from here).
template <class LM, std::size_t... I>
auto md_sub_map_full(const LM& lm,
                     std::index_sequence<I...>) -> decltype(submdspan_mapping(lm, ((void)I, std::full_extent)...));
template <class LM>
concept sliceable_mapping = requires(const LM& lm) {
  md_sub_map_full(lm, std::make_index_sequence<LM::extents_type::rank()>());
  requires ::ycxx::detail::md_is_mapping_result<decltype(md_sub_map_full(
      lm, std::make_index_sequence<LM::extents_type::rank()>()))>;
};
// The customization point call of submdspan ([mdspan.sub.sub]/3, Note 1).
template <class LM, class... Sl>
constexpr auto call_submdspan_mapping(const LM& lm, const Sl&... slices) {
  return submdspan_mapping(lm, slices...);
}
} // namespace ycxx::detail::md_adl

namespace std {

// [mdspan.sub.sub]
template <class ElementType, class Extents, class LayoutPolicy, class AccessorPolicy, class... SliceSpecifiers>
  requires(sizeof...(SliceSpecifiers) == Extents::rank() &&
           ycxx::detail::md_adl::sliceable_mapping<typename LayoutPolicy::template mapping<Extents>>)
constexpr auto submdspan(const mdspan<ElementType, Extents, LayoutPolicy, AccessorPolicy>& src,
                         SliceSpecifiers... raw_slices) {
  auto t = std::canonical_slices(src.extents(), std::move(raw_slices)...);
  return [&]<size_t... K>(index_sequence<K...>) {
    auto sub_map_result = ycxx::detail::md_adl::call_submdspan_mapping(src.mapping(), std::get<K>(t)...);
    using A = typename AccessorPolicy::offset_policy;
    using R = decltype(sub_map_result.mapping);
    return mdspan<typename A::element_type, typename R::extents_type, typename R::layout_type, A>(
        src.accessor().offset(src.data_handle(), sub_map_result.offset), sub_map_result.mapping, A(src.accessor()));
  }(index_sequence_for<SliceSpecifiers...>());
}

} // namespace std
