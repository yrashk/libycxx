// libycxx core: <mdspan> part 1, extents ([mdspan.extents]) and the integer helpers that the
// layouts, mdspan and submdspan share.
//
// An extents object stores only its dynamic extents (an array of index_type, nothing for a
// fully static extents). Static extents and the dynamic-index tables are static constexpr
// arrays. Preconditions on index values are checked during constant evaluation and when
// hardened (DECISIONS §1.7); the checks that loop are skipped otherwise (md_checking()).
#pragma once

#include <ycxx/core/move.hpp>
#include <ycxx/core/span.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _IndexType, size_t... _Extents>
class extents;
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// True when a precondition that costs a loop is worth evaluating.
// Tags layout_stride::mapping's constructor for submdspan results (mdspan_layout.hpp).
struct __md_sub_strides_t {
  explicit __md_sub_strides_t() = default;
};

[[__gnu__::__always_inline__]] constexpr bool __md_checking() noexcept {
  if consteval {
    return true;
  } else {
    return __cfg::__hardened;
  }
}

// [mdspan.extents.overview]/1: "a signed or unsigned integer type".
template <class _Tp>
concept __md_index_type = __is_signed_or_unsigned_integer<_Tp> && std::is_same_v<_Tp, std::remove_cv_t<_Tp>>;

// An integral value as a signed or unsigned integer type (the character types map to the
// integer type of the same signedness and width), so that std::cmp_* accept it.
template <class _Tp>
constexpr auto __md_as_int(_Tp __v) noexcept {
  if constexpr (__is_signed_or_unsigned_integer<_Tp>)
    return __v;
  else if constexpr (std::is_same_v<std::remove_cv_t<_Tp>, bool>)
    return static_cast<unsigned char>(__v);
  else if constexpr (std::is_signed_v<_Tp>)
    return static_cast<std::make_signed_t<_Tp>>(__v);
  else
    return static_cast<std::make_unsigned_t<_Tp>>(__v);
}

template <class _Tp>
concept __md_plain_integral = std::is_integral_v<std::remove_cvref_t<_Tp>> && !std::is_same_v<std::remove_cvref_t<_Tp>, bool>;

// index-cast ([mdspan.extents.expo]/9).
template <class _IndexType, class _Op>
constexpr auto __md_index_cast(_Op&& i) noexcept {
  if constexpr (__md_plain_integral<_Op>)
    return i;
  else
    return static_cast<_IndexType>(static_cast<_Op&&>(i));
}

// 0 <= i < n for an integral i of any type and a nonnegative n.
template <class _Ip, class _Np>
constexpr bool __md_in_interval(_Ip i, _Np n) noexcept {
  auto __v = ::__ycxx::__detail::__md_as_int(i);
  return std::cmp_greater_equal(__v, 0) && std::cmp_less(__v, ::__ycxx::__detail::__md_as_int(n));
}

// "v is representable as a nonnegative value of type IndexType" for an integral v.
template <class _IndexType, class _Tp>
constexpr bool __md_nonneg_representable(_Tp __v) noexcept {
  auto __x = ::__ycxx::__detail::__md_as_int(__v);
  return std::cmp_greater_equal(__x, 0) && std::in_range<_IndexType>(__x);
}

// Converts one index argument to IndexType and folds "it lies in [0, n)" into ok. An integral
// argument is checked before the conversion (index-cast), any other after it.
template <class _IndexType, class _Op>
constexpr _IndexType __md_cast_index(_Op&& __v, _IndexType n, bool& ok) noexcept {
  if constexpr (__md_plain_integral<_Op>) {
    ok = ok && ::__ycxx::__detail::__md_in_interval(__v, n);
    return static_cast<_IndexType>(__v);
  } else {
    _IndexType __x = static_cast<_IndexType>(static_cast<_Op&&>(__v));
    ok = ok && ::__ycxx::__detail::__md_in_interval(__x, n);
    return __x;
  }
}

// a * b and a + b, reporting overflow of T (the operands are nonnegative).
template <class _Tp>
constexpr bool __md_mul(_Tp a, _Tp b, _Tp& r) noexcept {
  return !__builtin_mul_overflow(a, b, &r);
}
template <class _Tp>
constexpr bool __md_add(_Tp a, _Tp b, _Tp& r) noexcept {
  return !__builtin_add_overflow(a, b, &r);
}

// LEAST-MULTIPLE-AT-LEAST(x, y) ([mdspan.layout.general]/2.4) in T; false on overflow.
template <class _Tp>
constexpr bool __md_least_multiple(_Tp __x, _Tp y, _Tp& r) noexcept {
  if (__x == 0) {
    r = y;
    return true;
  }
  _Tp __q = y / __x + (y % __x != 0 ? 1 : 0);
  return ::__ycxx::__detail::__md_mul(__q, __x, r);
}

// Precondition on one extent given to an extents constructor: an integral value is
// representable as a nonnegative index_type; any other is nonnegative after conversion.
template <class _IndexType, class _Op>
constexpr bool __md_extent_ok(const _Op& __v, _IndexType converted) noexcept {
  if constexpr (__md_plain_integral<_Op>)
    return ::__ycxx::__detail::__md_nonneg_representable<_IndexType>(__v);
  else
    return std::cmp_greater_equal(converted, 0);
}

// The static extents of extents<I, E...>, dynamic-index(i) for i in [0, rank()] and
// dynamic-index-inv(i) for i in [0, rank_dynamic()).
template <std::size_t _Rank, std::size_t _RankDynamic>
struct __md_ext_index_tables {
  std::size_t index[_Rank + 1];
  std::size_t __inv[_RankDynamic + 1];
};
template <std::size_t _RankDynamic, std::size_t... _Ep>
consteval __md_ext_index_tables<sizeof...(_Ep), _RankDynamic> __md_make_ext_tables() {
  __md_ext_index_tables<sizeof...(_Ep), _RankDynamic> t{};
  const std::size_t __statics[] = {_Ep..., 0};
  std::size_t n = 0;
  for (std::size_t r = 0; r < sizeof...(_Ep); ++r) {
    t.index[r] = n;
    if (__statics[r] == std::dynamic_extent)
      t.__inv[n++] = r;
  }
  t.index[sizeof...(_Ep)] = n;
  return t;
}
template <std::size_t... _Ep>
struct __md_ext_tables {
  static constexpr std::size_t rank_dynamic = ((_Ep == std::dynamic_extent ? 1 : 0) + ... + 0);
  static constexpr std::size_t __statics[sizeof...(_Ep) + 1] = {_Ep..., 0};
  static constexpr __md_ext_index_tables<sizeof...(_Ep), rank_dynamic> __tables =
      ::__ycxx::__detail::__md_make_ext_tables<rank_dynamic, _Ep...>();
};

// The second Mandate of [mdspan.extents.overview]/1 (vacuous when the first fails, so that only
// one diagnostic is issued).
template <class _IndexType, std::size_t... _Ep>
consteval bool __md_static_extents_fit() {
  if constexpr (__md_index_type<_IndexType>)
    return ((_Ep == std::dynamic_extent || std::in_range<_IndexType>(_Ep)) && ...);
  else
    return true;
}

template <class _Tp>
inline constexpr bool __md_is_extents = false;
template <class _IndexType, std::size_t... _Ep>
inline constexpr bool __md_is_extents<std::extents<_IndexType, _Ep...>> = true;

// The dynamic extents: an array of index_type, nothing when every extent is static.
template <class _Ip, std::size_t _Np>
struct __md_dyn_store {
  _Ip __v[_Np];
};
template <class _Ip>
struct __md_dyn_store<_Ip, 0> {};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _IndexType, size_t... _Extents>
class extents {
  static_assert(__ycxx::__detail::__md_index_type<_IndexType>,
                "std::extents: IndexType must be a signed or unsigned integer type");
  static_assert(__ycxx::__detail::__md_static_extents_fit<_IndexType, _Extents...>(),
                "std::extents: every static extent must be representable as a value of IndexType");

public:
  using index_type = _IndexType;
  // (unsigned for a rejected IndexType, so that the Mandate above is the only diagnostic)
  using size_type = make_unsigned_t<conditional_t<__ycxx::__detail::__md_index_type<_IndexType>, _IndexType, unsigned>>;
  using rank_type = size_t;

private:
  using __tables_t = __ycxx::__detail::__md_ext_tables<_Extents...>;
  static constexpr const size_t* __static_exts_ = __tables_t::__statics;
  static constexpr rank_type __rank_dyn_ = __tables_t::rank_dynamic;

  static constexpr rank_type __dynamic_index(rank_type i) noexcept { return __tables_t::__tables.index[i]; }
  static constexpr rank_type __dynamic_index_inv(rank_type i) noexcept { return __tables_t::__tables.__inv[i]; }

  // Constraints and explicitness of the converting constructor ([mdspan.extents.cons]/1, /4).
  template <size_t... _Other>
  static consteval bool __compatible() {
    if constexpr (sizeof...(_Other) != sizeof...(_Extents))
      return false;
    else
      return ((_Other == dynamic_extent || _Extents == dynamic_extent || _Other == _Extents) && ...);
  }
  template <class _OtherIndexType, size_t... _Other>
  static consteval bool __explicit_from() {
    if constexpr (sizeof...(_Other) != sizeof...(_Extents))
      return false;
    else
      return (((_Extents != dynamic_extent) && (_Other == dynamic_extent)) || ...) ||
             cmp_less(numeric_limits<index_type>::max(), numeric_limits<_OtherIndexType>::max());
  }

  // Stores the dynamic extents from n values; get(r) yields the value for rank index r of the
  // full list (n == rank()) or for the r-th dynamic extent (n == rank_dynamic()).
  template <class _Get>
  constexpr void store(size_t n, _Get get) noexcept {
    if constexpr (__rank_dyn_ != 0) {
      for (size_t d = 0; d < __rank_dyn_; ++d)
        __dyn_.__v[d] = get(n == __rank_dyn_ ? d : __dynamic_index_inv(d));
    }
  }
  // Precondition: the given value for each static extent equals it.
  template <class _Get>
  static constexpr bool __statics_match(size_t n, _Get get) noexcept {
    if (n == __rank_dyn_)
      return true;
    for (size_t r = 0; r < sizeof...(_Extents); ++r)
      if (__static_exts_[r] != dynamic_extent && !cmp_equal(get(r), __static_exts_[r]))
        return false;
    return true;
  }

public:
  // [mdspan.extents.obs]
  static constexpr rank_type rank() noexcept { return sizeof...(_Extents); }
  static constexpr rank_type rank_dynamic() noexcept { return __rank_dyn_; }
  static constexpr size_t static_extent(rank_type i) noexcept {
    __ycxx::__detail::__precondition(i < rank(), "std::extents::static_extent: index out of range");
    return __static_exts_[i];
  }
  constexpr index_type extent(rank_type i) const noexcept {
    __ycxx::__detail::__precondition(i < rank(), "std::extents::extent: index out of range");
    if constexpr (__rank_dyn_ == 0) {
      return static_cast<index_type>(__static_exts_[i]);
    } else {
      if (__static_exts_[i] == dynamic_extent)
        return __dyn_.__v[__dynamic_index(i)];
      return static_cast<index_type>(__static_exts_[i]);
    }
  }

  // [mdspan.extents.cons]
  constexpr extents() noexcept = default;

  template <class _OtherIndexType, size_t... _OtherExtents>
    requires(__compatible<_OtherExtents...>())
  constexpr explicit(__explicit_from<_OtherIndexType, _OtherExtents...>())
      extents(const extents<_OtherIndexType, _OtherExtents...>& other) noexcept {
    if (__ycxx::__detail::__md_checking()) {
      for (size_t r = 0; r < rank(); ++r) {
        __ycxx::__detail::__precondition(__static_exts_[r] == dynamic_extent || cmp_equal(other.extent(r), __static_exts_[r]),
                                   "std::extents: a static extent differs from the source extent");
        __ycxx::__detail::__precondition(in_range<index_type>(other.extent(r)),
                                   "std::extents: source extent not representable as index_type");
      }
    }
    store(rank(), [&](size_t r) { return static_cast<index_type>(other.extent(r)); });
  }

  template <class... _OtherIndexTypes>
    requires((is_convertible_v<_OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _OtherIndexTypes> && ...) &&
             (sizeof...(_OtherIndexTypes) == __rank_dyn_ || sizeof...(_OtherIndexTypes) == rank()))
  constexpr explicit extents(_OtherIndexTypes... __exts) noexcept {
    constexpr size_t n = sizeof...(_OtherIndexTypes);
    array<index_type, n> __arr{static_cast<index_type>(std::move(__exts))...};
    if (__ycxx::__detail::__md_checking()) {
      if constexpr (n != 0) {
        size_t r = 0;
        bool ok = true;
        ((ok = ok && __ycxx::__detail::__md_extent_ok<index_type>(__exts, __arr[r]), ++r), ...);
        __ycxx::__detail::__precondition(ok, "std::extents: an extent is negative or not representable as index_type");
      }
      __ycxx::__detail::__precondition(__statics_match(n, [&](size_t r) { return __arr[r]; }),
                                 "std::extents: a given extent differs from the static extent");
    }
    store(n, [&](size_t r) { return __arr[r]; });
  }

  template <class _OtherIndexType, size_t _Np>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&> && (_Np == __rank_dyn_ || _Np == rank()))
  constexpr explicit(_Np != __rank_dyn_) extents(span<_OtherIndexType, _Np> __exts) noexcept {
    __init_from_array<_Np>(__exts);
  }
  template <class _OtherIndexType, size_t _Np>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&> && (_Np == __rank_dyn_ || _Np == rank()))
  constexpr explicit(_Np != __rank_dyn_) extents(const array<_OtherIndexType, _Np>& __exts) noexcept {
    __init_from_array<_Np>(__exts);
  }

  // [mdspan.extents.cmp]
  template <class _OtherIndexType, size_t... _OtherExtents>
  friend constexpr bool operator==(const extents& __lhs, const extents<_OtherIndexType, _OtherExtents...>& __rhs) noexcept {
    if constexpr (sizeof...(_OtherExtents) != sizeof...(_Extents)) {
      return false;
    } else {
      for (size_t r = 0; r < rank(); ++r)
        if (!cmp_equal(__lhs.extent(r), __rhs.extent(r)))
          return false;
      return true;
    }
  }

private:
  template <size_t n, class _Exts>
  constexpr void __init_from_array(const _Exts& __exts) noexcept {
    if (__ycxx::__detail::__md_checking()) {
      for (size_t r = 0; r < n; ++r)
        __ycxx::__detail::__precondition(
            __ycxx::__detail::__md_extent_ok<index_type>(as_const(__exts[r]), static_cast<index_type>(as_const(__exts[r]))),
            "std::extents: an extent is negative or not representable as index_type");
      __ycxx::__detail::__precondition(__statics_match(n, [&](size_t r) { return static_cast<index_type>(as_const(__exts[r])); }),
                                 "std::extents: a given extent differs from the static extent");
    }
    store(n, [&](size_t r) { return static_cast<index_type>(as_const(__exts[r])); });
  }

  [[no_unique_address]] __ycxx::__detail::__md_dyn_store<index_type, __rank_dyn_> __dyn_{};
};

template <class... _Integrals>
  requires(is_convertible_v<_Integrals, size_t> && ...)
explicit extents(_Integrals...) -> extents<size_t, __ycxx::__detail::__maybe_static_ext<_Integrals>...>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _IndexType, class _Seq>
struct __md_dextents;
template <class _IndexType, std::size_t... _Ip>
struct __md_dextents<_IndexType, std::index_sequence<_Ip...>> {
  using type = std::extents<_IndexType, ((void)_Ip, std::dynamic_extent)...>;
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [mdspan.extents.dextents], [mdspan.extents.dims]
template <class _IndexType, size_t _Rank>
using dextents = typename __ycxx::__detail::__md_dextents<_IndexType, make_index_sequence<_Rank>>::type;
template <size_t _Rank, class _IndexType = size_t>
using dims = dextents<_IndexType, _Rank>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// fwd-prod-of-extents(i) and rev-prod-of-extents(i) ([mdspan.extents.expo]/5-8).
template <class _Ep>
constexpr std::size_t __md_fwd_prod(const _Ep& e, std::size_t i) noexcept {
  std::size_t p = 1;
  for (std::size_t k = 0; k < i; ++k)
    p *= static_cast<std::size_t>(e.extent(k));
  return p;
}
template <class _Ep>
constexpr std::size_t __md_rev_prod(const _Ep& e, std::size_t i) noexcept {
  std::size_t p = 1;
  for (std::size_t k = i + 1; k < _Ep::rank(); ++k)
    p *= static_cast<std::size_t>(e.extent(k));
  return p;
}

// "The size of the multidimensional index space e is representable as a value of type T."
template <class _Tp, class _Ep>
constexpr bool __md_size_fits(const _Ep& e) noexcept {
  for (std::size_t r = 0; r < _Ep::rank(); ++r)
    if (e.extent(r) == 0)
      return true;
  _Tp p = 1;
  for (std::size_t r = 0; r < _Ep::rank(); ++r) {
    if (!std::in_range<_Tp>(e.extent(r)) || !::__ycxx::__detail::__md_mul(p, static_cast<_Tp>(e.extent(r)), p))
      return false;
  }
  return true;
}

// The Mandates of the standard layouts: a fully static index space has a representable size.
template <class _Ep>
consteval bool __md_static_size_fits() {
  if constexpr (_Ep::rank_dynamic() != 0)
    return true;
  else
    return ::__ycxx::__detail::__md_size_fits<typename _Ep::index_type>(_Ep());
}

// "I... is a multidimensional index in e" for integral (index-cast) values.
template <class _Ep, class... _Ip>
constexpr bool __md_is_index(const _Ep& e, _Ip... i) noexcept {
  std::size_t r = 0;
  bool ok = true;
  ((ok = ok && ::__ycxx::__detail::__md_in_interval(i, e.extent(r)), ++r), ...);
  return ok;
}

}} // namespace __ycxx::__detail
