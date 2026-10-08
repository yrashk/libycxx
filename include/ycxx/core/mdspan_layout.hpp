// libycxx core: <mdspan> part 2, the layout mapping policies ([mdspan.layout]) and the accessor
// policies ([mdspan.accessor]).
//
// The submdspan_mapping customizations are hidden friends calling a private member
// submdspan_mapping_impl, which is defined in mdspan_sub.hpp (it needs the slice machinery).
// layout_left_padded/layout_right_padded store their padding stride only when it is not static
// (the recommended practice of [mdspan.layout.leftpad.expo]/2).
#pragma once

#include <ycxx/core/mdspan_extents.hpp>
#include <ycxx/core/memory_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

struct layout_left {
  template <class _Extents>
  class mapping;
};
struct layout_right {
  template <class _Extents>
  class mapping;
};
struct layout_stride {
  template <class _Extents>
  class mapping;
};
template <size_t _PaddingValue = dynamic_extent>
struct layout_left_padded {
  template <class _Extents>
  class mapping;
};
template <size_t _PaddingValue = dynamic_extent>
struct layout_right_padded {
  template <class _Extents>
  class mapping;
};

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// is-mapping-of, is-layout-left-padded-mapping-of, is-layout-right-padded-mapping-of
// ([mdspan.layout.general]/2).
template <class _Layout, class _Mapping>
concept __md_mapping_of = requires { typename _Mapping::extents_type; } &&
                        std::is_same_v<typename _Layout::template mapping<typename _Mapping::extents_type>, _Mapping>;
template <class _Mp>
concept __md_left_padded_mapping = requires { typename std::integral_constant<std::size_t, _Mp::padding_value>; } &&
                                 __md_mapping_of<std::layout_left_padded<_Mp::padding_value>, _Mp>;
template <class _Mp>
concept __md_right_padded_mapping = requires { typename std::integral_constant<std::size_t, _Mp::padding_value>; } &&
                                  __md_mapping_of<std::layout_right_padded<_Mp::padding_value>, _Mp>;

// layout-mapping-alike ([mdspan.layout.stride.expo]/4)
template <class _Mp>
concept __md_layout_mapping_alike = requires {
  requires __md_is_extents<typename _Mp::extents_type>;
  { _Mp::is_always_strided() } -> std::same_as<bool>;
  { _Mp::is_always_exhaustive() } -> std::same_as<bool>;
  { _Mp::is_always_unique() } -> std::same_as<bool>;
  std::bool_constant<_Mp::is_always_strided()>::value;
  std::bool_constant<_Mp::is_always_exhaustive()>::value;
  std::bool_constant<_Mp::is_always_unique()>::value;
};

// static-padding-stride of a padded layout of rank Rank whose padded extent (the first for
// layout_left_padded, the last for layout_right_padded) has the static value Ext. An overflow
// gives 0; the mappings' Mandates reject it.
consteval std::size_t __md_static_padding_stride(std::size_t __padding, std::size_t rank, std::size_t __ext) {
  if (rank <= 1)
    return 0;
  if (__padding == std::dynamic_extent || __ext == std::dynamic_extent)
    return std::dynamic_extent;
  std::size_t r = 0;
  if (!::__ycxx::__detail::__md_least_multiple(__padding, __ext, r))
    return 0;
  return r;
}
template <class _Mp>
consteval std::size_t __md_left_pad_stride_of() {
  using _Ep = typename _Mp::extents_type;
  return ::__ycxx::__detail::__md_static_padding_stride(_Mp::padding_value, _Ep::rank(),
                                                  _Ep::rank() == 0 ? 0 : _Ep::static_extent(0));
}
template <class _Mp>
consteval std::size_t __md_right_pad_stride_of() {
  using _Ep = typename _Mp::extents_type;
  return ::__ycxx::__detail::__md_static_padding_stride(_Mp::padding_value, _Ep::rank(),
                                                  _Ep::rank() == 0 ? 0 : _Ep::static_extent(_Ep::rank() - 1));
}

// The Mandates (5.3)/(5.4) of the padded layouts: LEAST-MULTIPLE-AT-LEAST(padding, ext) and its
// product with the other static extents are representable as size_t and IndexType.
template <class _IndexType, class _Ep, std::size_t _Padding, bool _Left>
consteval bool __md_padded_static_ok() {
  constexpr std::size_t rank = _Ep::rank();
  if constexpr (rank <= 1 || _Padding == std::dynamic_extent) {
    return true;
  } else {
    constexpr std::size_t __padded = _Left ? 0 : rank - 1;
    if (_Ep::static_extent(__padded) == std::dynamic_extent)
      return true;
    std::size_t s = 0;
    if (!::__ycxx::__detail::__md_least_multiple(_Padding, _Ep::static_extent(__padded), s) || !std::in_range<_IndexType>(s))
      return false;
    for (std::size_t k = 0; k < rank; ++k)
      if (_Ep::static_extent(k) == std::dynamic_extent)
        return true;
    for (std::size_t k = 0; k < rank; ++k)
      if (k != __padded && _Ep::static_extent(k) == 0)
        return true; // the product is 0
    for (std::size_t k = 0; k < rank; ++k)
      if (k != __padded && !::__ycxx::__detail::__md_mul(s, _Ep::static_extent(k), s))
        return false;
    return std::in_range<_IndexType>(s);
  }
}

// Preconditions of the padded constructors: the padding stride s and the product of s with
// every extent other than the padded one are representable as IndexType (and so is the size).
template <class _IndexType, class _Ep>
constexpr bool __md_padded_fits(const _Ep& e, std::size_t __padded, std::size_t stride) noexcept {
  if (!std::in_range<_IndexType>(stride) || !::__ycxx::__detail::__md_size_fits<_IndexType>(e))
    return false;
  if (stride == 0)
    return true;
  for (std::size_t k = 0; k < _Ep::rank(); ++k)
    if (e.extent(k) == 0)
      return true;
  _IndexType p = static_cast<_IndexType>(stride);
  for (std::size_t k = 0; k < _Ep::rank(); ++k)
    if (k != __padded && !::__ycxx::__detail::__md_mul(p, static_cast<_IndexType>(e.extent(k)), p))
      return false;
  return true;
}

// The padding stride, stored only when it is not static.
template <class _Ip, std::size_t _Sp>
struct __md_padding_stride {
  constexpr __md_padding_stride() noexcept = default;
  constexpr __md_padding_stride(_Ip) noexcept {}
  static constexpr _Ip get() noexcept { return static_cast<_Ip>(_Sp); }
};
template <class _Ip>
struct __md_padding_stride<_Ip, std::dynamic_extent> {
  _Ip __v{};
  constexpr __md_padding_stride() noexcept = default;
  constexpr __md_padding_stride(_Ip s) noexcept : __v(s) {}
  constexpr _Ip get() const noexcept { return __v; }
};

// OFFSET(m) ([mdspan.layout.stride.expo]/2).
template <class _Mp>
constexpr auto __md_offset(const _Mp& m) {
  using _Ep = typename _Mp::extents_type;
  if constexpr (_Ep::rank() == 0) {
    return m();
  } else {
    using _Ip = typename _Mp::index_type;
    for (std::size_t r = 0; r < _Ep::rank(); ++r)
      if (m.extents().extent(r) == 0)
        return _Ip(0);
    return [&]<std::size_t... _Kp>(std::index_sequence<_Kp...>) -> _Ip {
      return m(((void)_Kp, _Ip(0))...);
    }(std::make_index_sequence<_Ep::rank()>());
  }
}

// Converts the indices of a mapping call to index_type, checking that they form a
// multidimensional index in e.
template <class _Ep, class... _Indices>
constexpr std::array<typename _Ep::index_type, sizeof...(_Indices)> __md_indices(const _Ep& e, _Indices... i) noexcept {
  using _Ip = typename _Ep::index_type;
  bool ok = true;
  std::size_t r = 0;
  std::array<_Ip, sizeof...(_Indices)> __idx{::__ycxx::__detail::__md_cast_index<_Ip>(std::move(i), e.extent(r++), ok)...};
  ::__ycxx::__detail::__precondition(ok, "mdspan layout mapping: index is not a multidimensional index in extents()");
  return __idx;
}

// The size of the multidimensional index space e is 0.
template <class _Ep>
constexpr bool __md_empty(const _Ep& e) noexcept {
  for (std::size_t r = 0; r < _Ep::rank(); ++r)
    if (e.extent(r) == 0)
      return true;
  return false;
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.left]
// ---------------------------------------------------------------------------------------------
template <class _Extents>
class layout_left::mapping {
  static_assert(__ycxx::__detail::__md_is_extents<_Extents>,
                "layout_left::mapping: Extents must be a specialization of extents");
  static_assert(__ycxx::__detail::__md_static_size_fits<_Extents>(),
                "layout_left::mapping: the size of Extents() must be representable as index_type");

public:
  using extents_type = _Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_left;

  // [mdspan.layout.left.cons]
  constexpr mapping() noexcept = default;
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const extents_type& e) noexcept : __extents_(e) {
    __ycxx::__detail::__precondition(__ycxx::__detail::__md_size_fits<index_type>(e),
                               "layout_left::mapping: the size of e is not representable as index_type");
  }
  template <class _OtherExtents>
    requires is_constructible_v<extents_type, _OtherExtents>
  constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>) mapping(const mapping<_OtherExtents>& other) noexcept
      : __extents_(other.extents()) {
    __check_span(other.required_span_size());
  }
  template <class _OtherExtents>
    requires(extents_type::rank() <= 1 && is_constructible_v<extents_type, _OtherExtents>)
  constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>)
      mapping(const layout_right::mapping<_OtherExtents>& other) noexcept
      : __extents_(other.extents()) {
    __check_span(other.required_span_size());
  }
  template <class _LayoutLeftPaddedMapping>
    requires(__ycxx::__detail::__md_left_padded_mapping<_LayoutLeftPaddedMapping> &&
             is_constructible_v<extents_type, typename _LayoutLeftPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename _LayoutLeftPaddedMapping::extents_type, extents_type>)
      mapping(const _LayoutLeftPaddedMapping& other) noexcept
      : __extents_(other.extents()) {
    constexpr size_t __pad = __ycxx::__detail::__md_left_pad_stride_of<_LayoutLeftPaddedMapping>();
    if constexpr (extents_type::rank() > 1) {
      static_assert(extents_type::static_extent(0) == dynamic_extent || __pad == dynamic_extent ||
                        extents_type::static_extent(0) == __pad,
                    "layout_left::mapping: static extent(0) differs from the source's static padding stride");
      __ycxx::__detail::__precondition(cmp_equal(other.stride(1), other.extents().extent(0)),
                                 "layout_left::mapping: the source is padded (stride(1) != extent(0))");
    }
    __check_span(other.required_span_size());
  }
  template <class _OtherExtents>
    requires is_constructible_v<extents_type, _OtherExtents>
  // noexcept: a strengthening; the draft declares it so for layout_right only.
  constexpr explicit(!(extents_type::rank() == 0 && is_convertible_v<_OtherExtents, extents_type>))
      mapping(const layout_stride::mapping<_OtherExtents>& other) noexcept
      : __extents_(other.extents()) {
    if (__ycxx::__detail::__md_checking()) {
      for (size_t r = 0; r < extents_type::rank(); ++r)
        __ycxx::__detail::__precondition(cmp_equal(other.stride(r), __ycxx::__detail::__md_fwd_prod(other.extents(), r)),
                                   "layout_left::mapping: the layout_stride mapping is not column-major");
    }
    __check_span(other.required_span_size());
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.left.obs]
  constexpr const extents_type& extents() const noexcept { return __extents_; }
  constexpr index_type required_span_size() const noexcept {
    return static_cast<index_type>(__ycxx::__detail::__md_fwd_prod(__extents_, extents_type::rank()));
  }
  template <class... _Indices>
    requires(sizeof...(_Indices) == extents_type::rank() && (is_convertible_v<_Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _Indices> && ...))
  constexpr index_type operator()(_Indices... i) const noexcept {
    if constexpr (sizeof...(_Indices) == 0) {
      return 0;
    } else {
      auto __idx = __ycxx::__detail::__md_indices(__extents_, std::move(i)...);
      index_type __off = __idx[sizeof...(_Indices) - 1];
      for (size_t r = sizeof...(_Indices) - 1; r-- > 0;)
        __off = static_cast<index_type>(__off * __extents_.extent(r) + __idx[r]);
      return __off;
    }
  }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept { return true; }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_unique() noexcept { return true; }
  static constexpr bool is_exhaustive() noexcept { return true; }
  static constexpr bool is_strided() noexcept { return true; }
  constexpr index_type stride(rank_type i) const noexcept
    requires(extents_type::rank() > 0)
  {
    __ycxx::__detail::__precondition(i < extents_type::rank(), "layout_left::mapping::stride: index out of range");
    return static_cast<index_type>(__ycxx::__detail::__md_fwd_prod(__extents_, i));
  }
  template <class _OtherExtents>
    requires(extents_type::rank() == _OtherExtents::rank())
  friend constexpr bool operator==(const mapping& __x, const mapping<_OtherExtents>& y) noexcept {
    return __x.extents() == y.extents();
  }

private:
  constexpr void __check_span(auto n) const noexcept {
    __ycxx::__detail::__precondition(in_range<index_type>(n),
                               "mdspan layout mapping: required_span_size() not representable as index_type");
  }

  extents_type __extents_{};

  template <class... _SliceSpecifiers>
  constexpr auto __submdspan_mapping_impl(_SliceSpecifiers... __slices) const;
  template <class... _SliceSpecifiers>
    requires(sizeof...(_SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& __src, _SliceSpecifiers... __slices) {
    return __src.__submdspan_mapping_impl(__slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.right]
// ---------------------------------------------------------------------------------------------
template <class _Extents>
class layout_right::mapping {
  static_assert(__ycxx::__detail::__md_is_extents<_Extents>,
                "layout_right::mapping: Extents must be a specialization of extents");
  static_assert(__ycxx::__detail::__md_static_size_fits<_Extents>(),
                "layout_right::mapping: the size of Extents() must be representable as index_type");

public:
  using extents_type = _Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_right;

  // [mdspan.layout.right.cons]
  constexpr mapping() noexcept = default;
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const extents_type& e) noexcept : __extents_(e) {
    __ycxx::__detail::__precondition(__ycxx::__detail::__md_size_fits<index_type>(e),
                               "layout_right::mapping: the size of e is not representable as index_type");
  }
  template <class _OtherExtents>
    requires is_constructible_v<extents_type, _OtherExtents>
  constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>) mapping(const mapping<_OtherExtents>& other) noexcept
      : __extents_(other.extents()) {
    __check_span(other.required_span_size());
  }
  template <class _OtherExtents>
    requires(extents_type::rank() <= 1 && is_constructible_v<extents_type, _OtherExtents>)
  constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>)
      mapping(const layout_left::mapping<_OtherExtents>& other) noexcept
      : __extents_(other.extents()) {
    __check_span(other.required_span_size());
  }
  template <class _LayoutRightPaddedMapping>
    requires(__ycxx::__detail::__md_right_padded_mapping<_LayoutRightPaddedMapping> &&
             is_constructible_v<extents_type, typename _LayoutRightPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename _LayoutRightPaddedMapping::extents_type, extents_type>)
      mapping(const _LayoutRightPaddedMapping& other) noexcept
      : __extents_(other.extents()) {
    constexpr size_t rank = extents_type::rank();
    constexpr size_t __pad = __ycxx::__detail::__md_right_pad_stride_of<_LayoutRightPaddedMapping>();
    if constexpr (rank > 1) {
      static_assert(extents_type::static_extent(rank - 1) == dynamic_extent || __pad == dynamic_extent ||
                        extents_type::static_extent(rank - 1) == __pad,
                    "layout_right::mapping: static extent(rank - 1) differs from the source's static padding stride");
      __ycxx::__detail::__precondition(cmp_equal(other.stride(rank - 2), other.extents().extent(rank - 1)),
                                 "layout_right::mapping: the source is padded (stride(rank - 2) != extent(rank - 1))");
    }
    __check_span(other.required_span_size());
  }
  template <class _OtherExtents>
    requires is_constructible_v<extents_type, _OtherExtents>
  constexpr explicit(!(extents_type::rank() == 0 && is_convertible_v<_OtherExtents, extents_type>))
      mapping(const layout_stride::mapping<_OtherExtents>& other) noexcept
      : __extents_(other.extents()) {
    if (__ycxx::__detail::__md_checking()) {
      for (size_t r = 0; r < extents_type::rank(); ++r)
        __ycxx::__detail::__precondition(cmp_equal(other.stride(r), __ycxx::__detail::__md_rev_prod(other.extents(), r)),
                                   "layout_right::mapping: the layout_stride mapping is not row-major");
    }
    __check_span(other.required_span_size());
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.right.obs]
  constexpr const extents_type& extents() const noexcept { return __extents_; }
  constexpr index_type required_span_size() const noexcept {
    return static_cast<index_type>(__ycxx::__detail::__md_fwd_prod(__extents_, extents_type::rank()));
  }
  template <class... _Indices>
    requires(sizeof...(_Indices) == extents_type::rank() && (is_convertible_v<_Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _Indices> && ...))
  constexpr index_type operator()(_Indices... i) const noexcept {
    if constexpr (sizeof...(_Indices) == 0) {
      return 0;
    } else {
      auto __idx = __ycxx::__detail::__md_indices(__extents_, std::move(i)...);
      index_type __off = __idx[0];
      for (size_t r = 1; r < sizeof...(_Indices); ++r)
        __off = static_cast<index_type>(__off * __extents_.extent(r) + __idx[r]);
      return __off;
    }
  }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept { return true; }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_unique() noexcept { return true; }
  static constexpr bool is_exhaustive() noexcept { return true; }
  static constexpr bool is_strided() noexcept { return true; }
  constexpr index_type stride(rank_type i) const noexcept
    requires(extents_type::rank() > 0)
  {
    __ycxx::__detail::__precondition(i < extents_type::rank(), "layout_right::mapping::stride: index out of range");
    return static_cast<index_type>(__ycxx::__detail::__md_rev_prod(__extents_, i));
  }
  template <class _OtherExtents>
    requires(extents_type::rank() == _OtherExtents::rank())
  friend constexpr bool operator==(const mapping& __x, const mapping<_OtherExtents>& y) noexcept {
    return __x.extents() == y.extents();
  }

private:
  constexpr void __check_span(auto n) const noexcept {
    __ycxx::__detail::__precondition(in_range<index_type>(n),
                               "mdspan layout mapping: required_span_size() not representable as index_type");
  }

  extents_type __extents_{};

  template <class... _SliceSpecifiers>
  constexpr auto __submdspan_mapping_impl(_SliceSpecifiers... __slices) const;
  template <class... _SliceSpecifiers>
    requires(sizeof...(_SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& __src, _SliceSpecifiers... __slices) {
    return __src.__submdspan_mapping_impl(__slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.stride]
// ---------------------------------------------------------------------------------------------
template <class _Extents>
class layout_stride::mapping {
  static_assert(__ycxx::__detail::__md_is_extents<_Extents>,
                "layout_stride::mapping: Extents must be a specialization of extents");
  static_assert(__ycxx::__detail::__md_static_size_fits<_Extents>(),
                "layout_stride::mapping: the size of Extents() must be representable as index_type");

public:
  using extents_type = _Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_stride;

private:
  static constexpr rank_type __rank_ = extents_type::rank();

  // REQUIRED-SPAN-SIZE(e, strides) computed without overflow; false if it is not
  // representable as index_type ([mdspan.layout.stride.expo]/1).
  template <class _Strides>
  static constexpr bool __required_span(const extents_type& e, const _Strides& s, index_type& out) noexcept {
    if constexpr (__rank_ == 0) {
      out = 1;
      return true;
    } else {
      for (size_t r = 0; r < __rank_; ++r)
        if (e.extent(r) == 0) {
          out = 0;
          return true;
        }
      index_type sum = 1;
      for (size_t r = 0; r < __rank_; ++r) {
        index_type t = 0;
        if (!__ycxx::__detail::__md_mul(static_cast<index_type>(e.extent(r) - 1), static_cast<index_type>(s[r]), t) ||
            !__ycxx::__detail::__md_add(sum, t, sum))
          return false;
      }
      out = sum;
      return true;
    }
  }

  // The permutation of the rank indices ordering the strides ascending (ties: smaller extent
  // first), as used by [mdspan.layout.stride.cons]/4.3 and is_exhaustive().
  template <class _Strides>
  static constexpr array<size_t, __rank_> __sorted(const extents_type& e, const _Strides& s) noexcept {
    array<size_t, __rank_> p{};
    for (size_t r = 0; r < __rank_; ++r)
      p[r] = r;
    for (size_t i = 1; i < __rank_; ++i)
      for (size_t __j = i; __j > 0; --__j) {
        size_t a = p[__j - 1], b = p[__j];
        if (s[b] < s[a] || (s[b] == s[a] && e.extent(b) < e.extent(a))) {
          p[__j - 1] = b;
          p[__j] = a;
        } else {
          break;
        }
      }
    return p;
  }

  template <class _Strides>
  constexpr void __check_strides(const extents_type& e, const _Strides& s, bool __check_unique = true) const noexcept {
    if (!__ycxx::__detail::__md_checking())
      return;
    index_type n = 0;
    __ycxx::__detail::__precondition(__required_span(e, s, n),
                               "layout_stride::mapping: REQUIRED-SPAN-SIZE not representable as index_type");
    // Not diagnosed for an empty index space, where no stride is ever used: submdspan of an
    // empty mdspan can produce zero strides ([mdspan.sub.map.common]/6).
    if constexpr (__rank_ > 0) {
      bool empty = false;
      for (size_t r = 0; r < __rank_; ++r)
        empty = empty || e.extent(r) == 0;
      if (!empty) {
        bool __positive = true;
        for (size_t r = 0; r < __rank_; ++r)
          __positive = __positive && s[r] > 0;
        __ycxx::__detail::__precondition(__positive, "layout_stride::mapping: strides must be positive");
        if (!__check_unique)
          return;
        auto p = __sorted(e, s);
        bool unique = true;
        for (size_t i = 1; i < __rank_; ++i) {
          index_type m = 0;
          unique = unique && __ycxx::__detail::__md_mul(static_cast<index_type>(s[p[i - 1]]), e.extent(p[i - 1]), m) &&
                   s[p[i]] >= m;
        }
        __ycxx::__detail::__precondition(unique, "layout_stride::mapping: the strides do not give a unique layout");
      }
    }
  }

public:
  // [mdspan.layout.stride.cons]
  constexpr mapping() noexcept : __extents_(extents_type()) {
    __ycxx::__detail::__precondition(in_range<index_type>(__ycxx::__detail::__md_fwd_prod(extents_type(), __rank_)),
                               "layout_stride::mapping: the default extents' size is not representable as index_type");
    for (size_t d = 0; d < __rank_; ++d)
      __strides_[d] = static_cast<index_type>(__ycxx::__detail::__md_rev_prod(__extents_, d));
  }
  constexpr mapping(const mapping&) noexcept = default;
  template <class _OtherIndexType>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&>)
  constexpr mapping(const extents_type& e, span<_OtherIndexType, __rank_> s) noexcept : __extents_(e) {
    for (size_t d = 0; d < __rank_; ++d)
      __strides_[d] = static_cast<index_type>(as_const(s[d]));
    __check_strides(e, __strides_);
  }
  template <class _OtherIndexType>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&>)
  constexpr mapping(const extents_type& e, const array<_OtherIndexType, __rank_>& s) noexcept : __extents_(e) {
    for (size_t d = 0; d < __rank_; ++d)
      __strides_[d] = static_cast<index_type>(as_const(s[d]));
    __check_strides(e, __strides_);
  }
  // submdspan's result ([mdspan.sub.map.common]/6): the strides of a slice of a unique layout,
  // which is unique, but need not meet [mdspan.layout.stride.cons]/4.3 (that condition is
  // sufficient, not necessary, despite its Note: extents {2, 4} with strides {6, 9}, a slice of
  // {4, 4} with {2, 9}). The other preconditions are checked.
  constexpr mapping(__ycxx::__detail::__md_sub_strides_t, const extents_type& e,
                    const array<index_type, __rank_>& s) noexcept
      : __extents_(e), __strides_(s) {
    __check_strides(e, __strides_, false);
  }
  template <class _StridedLayoutMapping>
    requires(__ycxx::__detail::__md_layout_mapping_alike<_StridedLayoutMapping> &&
             is_constructible_v<extents_type, typename _StridedLayoutMapping::extents_type> &&
             _StridedLayoutMapping::is_always_unique() && _StridedLayoutMapping::is_always_strided())
  constexpr explicit(!(is_convertible_v<typename _StridedLayoutMapping::extents_type, extents_type> &&
                       (__ycxx::__detail::__md_mapping_of<layout_left, _StridedLayoutMapping> ||
                        __ycxx::__detail::__md_mapping_of<layout_right, _StridedLayoutMapping> ||
                        __ycxx::__detail::__md_left_padded_mapping<_StridedLayoutMapping> ||
                        __ycxx::__detail::__md_right_padded_mapping<_StridedLayoutMapping> ||
                        __ycxx::__detail::__md_mapping_of<layout_stride, _StridedLayoutMapping>)))
      mapping(const _StridedLayoutMapping& other) noexcept
      : __extents_(other.extents()) {
    if constexpr (__rank_ > 0)
      for (size_t d = 0; d < __rank_; ++d)
        __strides_[d] = static_cast<index_type>(other.stride(d));
    if (__ycxx::__detail::__md_checking()) {
      if constexpr (__rank_ > 0)
        if (!__ycxx::__detail::__md_empty(__extents_))
          for (size_t d = 0; d < __rank_; ++d)
            __ycxx::__detail::__precondition(other.stride(d) > 0, "layout_stride::mapping: strides must be positive");
      __ycxx::__detail::__precondition(in_range<index_type>(other.required_span_size()),
                                 "layout_stride::mapping: required_span_size() not representable as index_type");
      __ycxx::__detail::__precondition(__ycxx::__detail::__md_offset(other) == 0,
                                 "layout_stride::mapping: the source mapping has a nonzero offset");
    }
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.stride.obs]
  constexpr const extents_type& extents() const noexcept { return __extents_; }
  constexpr array<index_type, __rank_> strides() const noexcept { return __strides_; }
  constexpr index_type required_span_size() const noexcept {
    index_type n = 0;
    __required_span(__extents_, __strides_, n);
    return n;
  }
  template <class... _Indices>
    requires(sizeof...(_Indices) == __rank_ && (is_convertible_v<_Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _Indices> && ...))
  constexpr index_type operator()(_Indices... i) const noexcept {
    auto __idx = __ycxx::__detail::__md_indices(__extents_, std::move(i)...);
    index_type __off = 0;
    for (size_t r = 0; r < __rank_; ++r)
      __off = static_cast<index_type>(__off + __idx[r] * __strides_[r]);
    return __off;
  }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept {
    if constexpr (__rank_ == 0) {
      return true;
    } else {
      for (size_t r = 0; r < __rank_; ++r)
        if (extents_type::static_extent(r) == 0)
          return true;
      return false;
    }
  }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_unique() noexcept { return true; }
  constexpr bool is_exhaustive() const noexcept {
    if constexpr (__rank_ == 0) {
      return true;
    } else {
      for (size_t r = 0; r < __rank_; ++r)
        if (__extents_.extent(r) == 0)
          return true;
      auto p = __sorted(__extents_, __strides_);
      if (__strides_[p[0]] != 1)
        return false;
      for (size_t i = 1; i < __rank_; ++i) {
        index_type m = 0;
        if (!__ycxx::__detail::__md_mul(__strides_[p[i - 1]], __extents_.extent(p[i - 1]), m) || __strides_[p[i]] != m)
          return false;
      }
      return true;
    }
  }
  static constexpr bool is_strided() noexcept { return true; }
  constexpr index_type stride(rank_type i) const noexcept {
    __ycxx::__detail::__precondition(i < __rank_, "layout_stride::mapping::stride: index out of range");
    return __strides_[i];
  }
  template <class _OtherMapping>
    requires(__ycxx::__detail::__md_layout_mapping_alike<_OtherMapping> && __rank_ == _OtherMapping::extents_type::rank() &&
             _OtherMapping::is_always_strided())
  friend constexpr bool operator==(const mapping& __x, const _OtherMapping& y) noexcept {
    if (!(__x.extents() == y.extents()) || __ycxx::__detail::__md_offset(y) != 0)
      return false;
    if constexpr (__rank_ > 0)
      for (size_t r = 0; r < __rank_; ++r)
        if (!cmp_equal(__x.stride(r), y.stride(r)))
          return false;
    return true;
  }

private:
  extents_type __extents_{};
  array<index_type, __rank_> __strides_{};

  template <class... _SliceSpecifiers>
  constexpr auto __submdspan_mapping_impl(_SliceSpecifiers... __slices) const;
  template <class... _SliceSpecifiers>
    requires(sizeof...(_SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& __src, _SliceSpecifiers... __slices) {
    return __src.__submdspan_mapping_impl(__slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.leftpad]
// ---------------------------------------------------------------------------------------------
template <size_t _PaddingValue>
template <class _Extents>
class layout_left_padded<_PaddingValue>::mapping {
  static_assert(__ycxx::__detail::__md_is_extents<_Extents>,
                "layout_left_padded::mapping: Extents must be a specialization of extents");

public:
  static constexpr size_t padding_value = _PaddingValue;
  using extents_type = _Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_left_padded<_PaddingValue>;

private:
  static constexpr size_t __rank_ = extents_type::rank();
  static constexpr size_t __first_static_extent = __rank_ == 0 ? dynamic_extent : extents_type::static_extent(0);
  static constexpr size_t __static_padding_stride =
      __ycxx::__detail::__md_static_padding_stride(padding_value, __rank_, __first_static_extent);

  static_assert(__ycxx::__detail::__md_static_size_fits<_Extents>(),
                "layout_left_padded::mapping: the size of Extents() must be representable as index_type");
  static_assert(padding_value == dynamic_extent || in_range<index_type>(padding_value),
                "layout_left_padded::mapping: padding_value must be representable as index_type");
  static_assert(__ycxx::__detail::__md_padded_static_ok<index_type, extents_type, padding_value, true>(),
                "layout_left_padded::mapping: the padded static extents overflow index_type");

  // stride(1) for extents e and padding pad: LEAST-MULTIPLE-AT-LEAST(pad, e.extent(0)).
  static constexpr index_type __padded_stride(const extents_type& e, size_t __pad) noexcept {
    size_t s = 0;
    bool ok = __ycxx::__detail::__md_least_multiple(__pad, static_cast<size_t>(e.extent(0)), s);
    __ycxx::__detail::__precondition(ok && __ycxx::__detail::__md_padded_fits<index_type>(e, 0, s),
                               "layout_left_padded::mapping: the padded extents overflow index_type");
    return static_cast<index_type>(s);
  }

public:
  // [mdspan.layout.leftpad.cons]
  constexpr mapping() noexcept : mapping(extents_type{}) {}
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const extents_type& __ext) : __extents_(__ext) {
    __ycxx::__detail::__precondition(__ycxx::__detail::__md_size_fits<index_type>(__ext),
                               "layout_left_padded::mapping: the size of ext is not representable as index_type");
    if constexpr (__rank_ > 1) {
      if constexpr (padding_value == dynamic_extent)
        __stride_1_ = __ext.extent(0);
      else
        __stride_1_ = __padded_stride(__ext, padding_value);
    }
  }
  template <class _OtherIndexType>
    requires(is_convertible_v<_OtherIndexType, index_type> && is_nothrow_constructible_v<index_type, _OtherIndexType>)
  constexpr mapping(const extents_type& __ext, _OtherIndexType __padding) : __extents_(__ext) {
    auto __pad = __ycxx::__detail::__md_index_cast<index_type>(std::move(__padding));
    // pad > 0 is not diagnosed for an empty index space: submdspan of an empty mdspan can
    // pass a zero padding stride ([mdspan.sub.map.left]/1.4).
    __ycxx::__detail::__precondition((cmp_greater(__ycxx::__detail::__md_as_int(__pad), 0) || __ycxx::__detail::__md_empty(__ext)) &&
                                   in_range<index_type>(__ycxx::__detail::__md_as_int(__pad)),
                               "layout_left_padded::mapping: padding must be positive and representable");
    __ycxx::__detail::__precondition(padding_value == dynamic_extent ||
                                   cmp_equal(padding_value, __ycxx::__detail::__md_as_int(__pad)),
                               "layout_left_padded::mapping: padding differs from padding_value");
    __ycxx::__detail::__precondition(__ycxx::__detail::__md_size_fits<index_type>(__ext),
                               "layout_left_padded::mapping: the size of ext is not representable as index_type");
    if constexpr (__rank_ > 1)
      __stride_1_ = __padded_stride(__ext, static_cast<size_t>(__pad));
  }
  template <class _OtherExtents>
    requires is_constructible_v<extents_type, _OtherExtents>
  constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>)
      mapping(const layout_left::mapping<_OtherExtents>& other)
      : mapping(extents_type(other.extents())) {
    if constexpr (_OtherExtents::rank() > 1)
      static_assert(__static_padding_stride == dynamic_extent || _OtherExtents::static_extent(0) == dynamic_extent ||
                        __static_padding_stride == _OtherExtents::static_extent(0),
                    "layout_left_padded::mapping: the source's static extent(0) is not the static padding stride");
    if constexpr (__rank_ > 1 && padding_value != dynamic_extent)
      __ycxx::__detail::__precondition(cmp_equal(other.stride(1), __stride_1_.get()),
                                 "layout_left_padded::mapping: extent(0) of the source is not padded");
    __ycxx::__detail::__precondition(in_range<index_type>(other.required_span_size()),
                               "layout_left_padded::mapping: required_span_size() not representable");
  }
  template <class _OtherExtents>
    requires is_constructible_v<extents_type, _OtherExtents>
  constexpr explicit(!(__rank_ == 0 && is_convertible_v<_OtherExtents, extents_type>))
      mapping(const layout_stride::mapping<_OtherExtents>& other)
      : __extents_(other.extents()) {
    if constexpr (__rank_ > 1)
      __stride_1_ = static_cast<index_type>(other.stride(1));
    if (__ycxx::__detail::__md_checking()) {
      if constexpr (__rank_ > 1 && padding_value != dynamic_extent) {
        size_t s = 0;
        __ycxx::__detail::__precondition(
            __ycxx::__detail::__md_least_multiple(padding_value, static_cast<size_t>(__extents_.extent(0)), s) &&
                cmp_equal(other.stride(1), s),
            "layout_left_padded::mapping: stride(1) of the source is not the padded extent(0)");
      }
      if constexpr (__rank_ > 0)
        __ycxx::__detail::__precondition(other.stride(0) == 1,
                                   "layout_left_padded::mapping: stride(0) of the source is not 1");
      for (size_t r = 2; r < __rank_ && other.extents().extent(0) != 0; ++r)
        __ycxx::__detail::__precondition(
            cmp_equal(other.stride(r), (__ycxx::__detail::__md_fwd_prod(other.extents(), r) / other.extents().extent(0)) *
                                           static_cast<size_t>(other.stride(1))),
            "layout_left_padded::mapping: the source's strides are not those of a padded layout");
      __ycxx::__detail::__precondition(in_range<index_type>(other.required_span_size()),
                                 "layout_left_padded::mapping: required_span_size() not representable");
    }
  }
  template <class _LayoutLeftPaddedMapping>
    requires(__ycxx::__detail::__md_left_padded_mapping<_LayoutLeftPaddedMapping> &&
             is_constructible_v<extents_type, typename _LayoutLeftPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename _LayoutLeftPaddedMapping::extents_type, extents_type> ||
                     (__rank_ > 1 &&
                      (padding_value != dynamic_extent || _LayoutLeftPaddedMapping::padding_value == dynamic_extent)))
      mapping(const _LayoutLeftPaddedMapping& other)
      : __extents_(other.extents()) {
    static_assert(__rank_ <= 1 || padding_value == dynamic_extent ||
                      _LayoutLeftPaddedMapping::padding_value == dynamic_extent ||
                      padding_value == _LayoutLeftPaddedMapping::padding_value,
                  "layout_left_padded::mapping: the padding values differ");
    if constexpr (__rank_ > 1) {
      __stride_1_ = static_cast<index_type>(other.stride(1));
      if constexpr (padding_value != dynamic_extent) {
        size_t s = 0;
        __ycxx::__detail::__precondition(
            __ycxx::__detail::__md_least_multiple(padding_value, static_cast<size_t>(__extents_.extent(0)), s) &&
                cmp_equal(other.stride(1), s),
            "layout_left_padded::mapping: stride(1) of the source is not the padded extent(0)");
      }
    }
    __ycxx::__detail::__precondition(in_range<index_type>(other.required_span_size()),
                               "layout_left_padded::mapping: required_span_size() not representable");
  }
  template <class _LayoutRightPaddedMapping>
    requires((__ycxx::__detail::__md_right_padded_mapping<_LayoutRightPaddedMapping> ||
              __ycxx::__detail::__md_mapping_of<layout_right, _LayoutRightPaddedMapping>) &&
             __rank_ <= 1 && is_constructible_v<extents_type, typename _LayoutRightPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename _LayoutRightPaddedMapping::extents_type, extents_type>)
      mapping(const _LayoutRightPaddedMapping& other) noexcept
      : __extents_(other.extents()) {
    __ycxx::__detail::__precondition(in_range<index_type>(other.required_span_size()),
                               "layout_left_padded::mapping: required_span_size() not representable");
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.leftpad.obs]
  constexpr const extents_type& extents() const noexcept { return __extents_; }
  constexpr array<index_type, __rank_> strides() const noexcept {
    array<index_type, __rank_> s{};
    for (size_t r = 0; r < __rank_; ++r)
      s[r] = stride(r);
    return s;
  }
  constexpr index_type required_span_size() const noexcept {
    for (size_t r = 0; r < __rank_; ++r)
      if (__extents_.extent(r) == 0)
        return 0;
    index_type n = 1;
    for (size_t r = 0; r < __rank_; ++r)
      n = static_cast<index_type>(n + (__extents_.extent(r) - 1) * stride(r));
    return n;
  }
  template <class... _Indices>
    requires(sizeof...(_Indices) == __rank_ && (is_convertible_v<_Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _Indices> && ...))
  constexpr index_type operator()(_Indices... __idxs) const noexcept {
    if constexpr (__rank_ == 0) {
      return 0;
    } else {
      auto __idx = __ycxx::__detail::__md_indices(__extents_, std::move(__idxs)...);
      index_type __off = __idx[0], s = 1;
      for (size_t r = 1; r < __rank_; ++r) {
        s = static_cast<index_type>(r == 1 ? __stride_1_.get() : s * __extents_.extent(r - 1));
        __off = static_cast<index_type>(__off + __idx[r] * s);
      }
      return __off;
    }
  }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept {
    if constexpr (__rank_ <= 1)
      return true;
    else if constexpr (__static_padding_stride != dynamic_extent && __first_static_extent != dynamic_extent)
      return __static_padding_stride == __first_static_extent;
    else
      return false;
  }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_unique() noexcept { return true; }
  constexpr bool is_exhaustive() const noexcept {
    if constexpr (__rank_ <= 1)
      return true;
    else
      return __extents_.extent(0) == stride(1);
  }
  static constexpr bool is_strided() noexcept { return true; }
  constexpr index_type stride(rank_type r) const noexcept {
    __ycxx::__detail::__precondition(r < __rank_, "layout_left_padded::mapping::stride: index out of range");
    if (r == 0)
      return 1;
    // In size_t: for an empty index space the product need not be representable.
    size_t s = static_cast<size_t>(__stride_1_.get());
    for (size_t k = 1; k < r; ++k)
      s *= static_cast<size_t>(__extents_.extent(k));
    return static_cast<index_type>(s);
  }
  template <class _LayoutLeftPaddedMapping>
    requires(__ycxx::__detail::__md_left_padded_mapping<_LayoutLeftPaddedMapping> &&
             _LayoutLeftPaddedMapping::extents_type::rank() == __rank_)
  friend constexpr bool operator==(const mapping& __x, const _LayoutLeftPaddedMapping& y) noexcept {
    if constexpr (__rank_ < 2)
      return __x.extents() == y.extents();
    else
      return __x.extents() == y.extents() && cmp_equal(__x.stride(1), y.stride(1));
  }

private:
  [[no_unique_address]] __ycxx::__detail::__md_padding_stride<index_type, __static_padding_stride> __stride_1_{};
  extents_type __extents_{};

  template <class... _SliceSpecifiers>
  constexpr auto __submdspan_mapping_impl(_SliceSpecifiers... __slices) const;
  template <class... _SliceSpecifiers>
    requires(sizeof...(_SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& __src, _SliceSpecifiers... __slices) {
    return __src.__submdspan_mapping_impl(__slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.rightpad]
// ---------------------------------------------------------------------------------------------
template <size_t _PaddingValue>
template <class _Extents>
class layout_right_padded<_PaddingValue>::mapping {
  static_assert(__ycxx::__detail::__md_is_extents<_Extents>,
                "layout_right_padded::mapping: Extents must be a specialization of extents");

public:
  static constexpr size_t padding_value = _PaddingValue;
  using extents_type = _Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_right_padded<_PaddingValue>;

private:
  static constexpr size_t __rank_ = extents_type::rank();
  static constexpr size_t __last_static_extent = __rank_ == 0 ? dynamic_extent : extents_type::static_extent(__rank_ - 1);
  static constexpr size_t __static_padding_stride =
      __ycxx::__detail::__md_static_padding_stride(padding_value, __rank_, __last_static_extent);

  static_assert(__ycxx::__detail::__md_static_size_fits<_Extents>(),
                "layout_right_padded::mapping: the size of Extents() must be representable as index_type");
  static_assert(padding_value == dynamic_extent || in_range<index_type>(padding_value),
                "layout_right_padded::mapping: padding_value must be representable as index_type");
  static_assert(__ycxx::__detail::__md_padded_static_ok<index_type, extents_type, padding_value, false>(),
                "layout_right_padded::mapping: the padded static extents overflow index_type");

  static constexpr index_type __padded_stride(const extents_type& e, size_t __pad) noexcept {
    size_t s = 0;
    bool ok = __ycxx::__detail::__md_least_multiple(__pad, static_cast<size_t>(e.extent(__rank_ - 1)), s);
    __ycxx::__detail::__precondition(ok && __ycxx::__detail::__md_padded_fits<index_type>(e, __rank_ - 1, s),
                               "layout_right_padded::mapping: the padded extents overflow index_type");
    return static_cast<index_type>(s);
  }

public:
  // [mdspan.layout.rightpad.cons]
  constexpr mapping() noexcept : mapping(extents_type{}) {}
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const extents_type& __ext) : __extents_(__ext) {
    __ycxx::__detail::__precondition(__ycxx::__detail::__md_size_fits<index_type>(__ext),
                               "layout_right_padded::mapping: the size of ext is not representable as index_type");
    if constexpr (__rank_ > 1) {
      if constexpr (padding_value == dynamic_extent)
        __stride_rm2_ = __ext.extent(__rank_ - 1);
      else
        __stride_rm2_ = __padded_stride(__ext, padding_value);
    }
  }
  template <class _OtherIndexType>
    requires(is_convertible_v<_OtherIndexType, index_type> && is_nothrow_constructible_v<index_type, _OtherIndexType>)
  constexpr mapping(const extents_type& __ext, _OtherIndexType __padding) : __extents_(__ext) {
    auto __pad = __ycxx::__detail::__md_index_cast<index_type>(std::move(__padding));
    // pad > 0 is not diagnosed for an empty index space: submdspan of an empty mdspan can
    // pass a zero padding stride ([mdspan.sub.map.left]/1.4).
    __ycxx::__detail::__precondition((cmp_greater(__ycxx::__detail::__md_as_int(__pad), 0) || __ycxx::__detail::__md_empty(__ext)) &&
                                   in_range<index_type>(__ycxx::__detail::__md_as_int(__pad)),
                               "layout_right_padded::mapping: padding must be positive and representable");
    __ycxx::__detail::__precondition(padding_value == dynamic_extent ||
                                   cmp_equal(padding_value, __ycxx::__detail::__md_as_int(__pad)),
                               "layout_right_padded::mapping: padding differs from padding_value");
    __ycxx::__detail::__precondition(__ycxx::__detail::__md_size_fits<index_type>(__ext),
                               "layout_right_padded::mapping: the size of ext is not representable as index_type");
    if constexpr (__rank_ > 1)
      __stride_rm2_ = __padded_stride(__ext, static_cast<size_t>(__pad));
  }
  template <class _OtherExtents>
    requires is_constructible_v<extents_type, _OtherExtents>
  constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>)
      mapping(const layout_right::mapping<_OtherExtents>& other)
      : mapping(extents_type(other.extents())) {
    if constexpr (_OtherExtents::rank() > 1)
      static_assert(__static_padding_stride == dynamic_extent ||
                        _OtherExtents::static_extent(__rank_ - 1) == dynamic_extent ||
                        __static_padding_stride == _OtherExtents::static_extent(__rank_ - 1),
                    "layout_right_padded::mapping: the source's static last extent is not the static padding stride");
    if constexpr (__rank_ > 1 && padding_value != dynamic_extent)
      __ycxx::__detail::__precondition(cmp_equal(other.stride(__rank_ - 2), __stride_rm2_.get()),
                                 "layout_right_padded::mapping: the last extent of the source is not padded");
    __ycxx::__detail::__precondition(in_range<index_type>(other.required_span_size()),
                               "layout_right_padded::mapping: required_span_size() not representable");
  }
  template <class _OtherExtents>
    requires is_constructible_v<extents_type, _OtherExtents>
  constexpr explicit(!(__rank_ == 0 && is_convertible_v<_OtherExtents, extents_type>))
      mapping(const layout_stride::mapping<_OtherExtents>& other)
      : __extents_(other.extents()) {
    if constexpr (__rank_ > 1)
      __stride_rm2_ = static_cast<index_type>(other.stride(__rank_ - 2));
    if (__ycxx::__detail::__md_checking()) {
      if constexpr (__rank_ > 1 && padding_value != dynamic_extent) {
        size_t s = 0;
        __ycxx::__detail::__precondition(
            __ycxx::__detail::__md_least_multiple(padding_value, static_cast<size_t>(__extents_.extent(__rank_ - 1)), s) &&
                cmp_equal(other.stride(__rank_ - 2), s),
            "layout_right_padded::mapping: stride(rank - 2) of the source is not the padded last extent");
      }
      if constexpr (__rank_ > 0)
        __ycxx::__detail::__precondition(other.stride(__rank_ - 1) == 1,
                                   "layout_right_padded::mapping: stride(rank - 1) of the source is not 1");
      if constexpr (__rank_ > 2) {
        for (size_t r = 0; r < __rank_ - 2 && other.extents().extent(__rank_ - 1) != 0; ++r)
          __ycxx::__detail::__precondition(
              cmp_equal(other.stride(r),
                        (__ycxx::__detail::__md_rev_prod(other.extents(), r) / other.extents().extent(__rank_ - 1)) *
                            static_cast<size_t>(other.stride(__rank_ - 2))),
              "layout_right_padded::mapping: the source's strides are not those of a padded layout");
      }
      __ycxx::__detail::__precondition(in_range<index_type>(other.required_span_size()),
                                 "layout_right_padded::mapping: required_span_size() not representable");
    }
  }
  template <class _LayoutRightPaddedMapping>
    requires(__ycxx::__detail::__md_right_padded_mapping<_LayoutRightPaddedMapping> &&
             is_constructible_v<extents_type, typename _LayoutRightPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename _LayoutRightPaddedMapping::extents_type, extents_type> ||
                     (__rank_ > 1 &&
                      (padding_value != dynamic_extent || _LayoutRightPaddedMapping::padding_value == dynamic_extent)))
      mapping(const _LayoutRightPaddedMapping& other)
      : __extents_(other.extents()) {
    static_assert(__rank_ <= 1 || padding_value == dynamic_extent ||
                      _LayoutRightPaddedMapping::padding_value == dynamic_extent ||
                      padding_value == _LayoutRightPaddedMapping::padding_value,
                  "layout_right_padded::mapping: the padding values differ");
    if constexpr (__rank_ > 1) {
      __stride_rm2_ = static_cast<index_type>(other.stride(__rank_ - 2));
      if constexpr (padding_value != dynamic_extent) {
        size_t s = 0;
        __ycxx::__detail::__precondition(
            __ycxx::__detail::__md_least_multiple(padding_value, static_cast<size_t>(__extents_.extent(__rank_ - 1)), s) &&
                cmp_equal(other.stride(__rank_ - 2), s),
            "layout_right_padded::mapping: stride(rank - 2) of the source is not the padded last extent");
      }
    }
    __ycxx::__detail::__precondition(in_range<index_type>(other.required_span_size()),
                               "layout_right_padded::mapping: required_span_size() not representable");
  }
  template <class _LayoutLeftPaddedMapping>
    requires((__ycxx::__detail::__md_left_padded_mapping<_LayoutLeftPaddedMapping> ||
              __ycxx::__detail::__md_mapping_of<layout_left, _LayoutLeftPaddedMapping>) &&
             __rank_ <= 1 && is_constructible_v<extents_type, typename _LayoutLeftPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename _LayoutLeftPaddedMapping::extents_type, extents_type>)
      mapping(const _LayoutLeftPaddedMapping& other) noexcept
      : __extents_(other.extents()) {
    __ycxx::__detail::__precondition(in_range<index_type>(other.required_span_size()),
                               "layout_right_padded::mapping: required_span_size() not representable");
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.rightpad.obs]
  constexpr const extents_type& extents() const noexcept { return __extents_; }
  constexpr array<index_type, __rank_> strides() const noexcept {
    array<index_type, __rank_> s{};
    for (size_t r = 0; r < __rank_; ++r)
      s[r] = stride(r);
    return s;
  }
  constexpr index_type required_span_size() const noexcept {
    for (size_t r = 0; r < __rank_; ++r)
      if (__extents_.extent(r) == 0)
        return 0;
    index_type n = 1;
    for (size_t r = 0; r < __rank_; ++r)
      n = static_cast<index_type>(n + (__extents_.extent(r) - 1) * stride(r));
    return n;
  }
  template <class... _Indices>
    requires(sizeof...(_Indices) == __rank_ && (is_convertible_v<_Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _Indices> && ...))
  constexpr index_type operator()(_Indices... __idxs) const noexcept {
    if constexpr (__rank_ == 0) {
      return 0;
    } else {
      auto __idx = __ycxx::__detail::__md_indices(__extents_, std::move(__idxs)...);
      index_type __off = __idx[__rank_ - 1], s = 1;
      for (size_t r = __rank_ - 1; r-- > 0;) {
        s = static_cast<index_type>(r == __rank_ - 2 ? __stride_rm2_.get() : s * __extents_.extent(r + 1));
        __off = static_cast<index_type>(__off + __idx[r] * s);
      }
      return __off;
    }
  }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept {
    if constexpr (__rank_ <= 1)
      return true;
    else if constexpr (__static_padding_stride != dynamic_extent && __last_static_extent != dynamic_extent)
      return __static_padding_stride == __last_static_extent;
    else
      return false;
  }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_unique() noexcept { return true; }
  constexpr bool is_exhaustive() const noexcept {
    if constexpr (__rank_ <= 1)
      return true;
    else
      return __extents_.extent(__rank_ - 1) == stride(__rank_ - 2);
  }
  static constexpr bool is_strided() noexcept { return true; }
  constexpr index_type stride(rank_type r) const noexcept {
    __ycxx::__detail::__precondition(r < __rank_, "layout_right_padded::mapping::stride: index out of range");
    if (r == __rank_ - 1)
      return 1;
    // In size_t: for an empty index space the product need not be representable.
    size_t s = static_cast<size_t>(__stride_rm2_.get());
    for (size_t k = r + 1; k < __rank_ - 1; ++k)
      s *= static_cast<size_t>(__extents_.extent(k));
    return static_cast<index_type>(s);
  }
  template <class _LayoutRightPaddedMapping>
    requires(__ycxx::__detail::__md_right_padded_mapping<_LayoutRightPaddedMapping> &&
             _LayoutRightPaddedMapping::extents_type::rank() == __rank_)
  friend constexpr bool operator==(const mapping& __x, const _LayoutRightPaddedMapping& y) noexcept {
    if constexpr (__rank_ < 2)
      return __x.extents() == y.extents();
    else
      return __x.extents() == y.extents() && cmp_equal(__x.stride(__rank_ - 2), y.stride(__rank_ - 2));
  }

private:
  [[no_unique_address]] __ycxx::__detail::__md_padding_stride<index_type, __static_padding_stride> __stride_rm2_{};
  extents_type __extents_{};

  template <class... _SliceSpecifiers>
  constexpr auto __submdspan_mapping_impl(_SliceSpecifiers... __slices) const;
  template <class... _SliceSpecifiers>
    requires(sizeof...(_SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& __src, _SliceSpecifiers... __slices) {
    return __src.__submdspan_mapping_impl(__slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.accessor.default]
// ---------------------------------------------------------------------------------------------
template <class _ElementType>
struct default_accessor {
  static_assert(is_object_v<_ElementType> && !is_abstract_v<_ElementType> && !is_array_v<_ElementType>,
                "default_accessor: ElementType must be a complete object type, not abstract, not an array");

  using offset_policy = default_accessor;
  using element_type = _ElementType;
  using reference = _ElementType&;
  using data_handle_type = _ElementType*;

  constexpr default_accessor() noexcept = default;
  template <class _OtherElementType>
    requires is_convertible_v<_OtherElementType (*)[], element_type (*)[]>
  constexpr default_accessor(default_accessor<_OtherElementType>) noexcept {}

  constexpr reference access(data_handle_type p, size_t i) const noexcept { return p[i]; }
  constexpr data_handle_type offset(data_handle_type p, size_t i) const noexcept { return p + i; }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.accessor.aligned]
// ---------------------------------------------------------------------------------------------
template <class _ElementType, size_t _ByteAlignment>
struct aligned_accessor {
  static_assert(_ByteAlignment != 0 && (_ByteAlignment & (_ByteAlignment - 1)) == 0,
                "aligned_accessor: byte_alignment must be a power of two");
  static_assert(_ByteAlignment >= alignof(_ElementType),
                "aligned_accessor: byte_alignment must be at least alignof(ElementType)");
  static_assert(is_object_v<_ElementType> && !is_abstract_v<_ElementType> && !is_array_v<_ElementType>,
                "aligned_accessor: ElementType must be a complete object type, not abstract, not an array");

  using offset_policy = default_accessor<_ElementType>;
  using element_type = _ElementType;
  using reference = _ElementType&;
  using data_handle_type = _ElementType*;

  static constexpr size_t byte_alignment = _ByteAlignment;

  constexpr aligned_accessor() noexcept = default;
  template <class _OtherElementType, size_t _OtherByteAlignment>
    requires(is_convertible_v<_OtherElementType (*)[], element_type (*)[]> && _OtherByteAlignment >= byte_alignment)
  constexpr aligned_accessor(aligned_accessor<_OtherElementType, _OtherByteAlignment>) noexcept {}
  template <class _OtherElementType>
    requires is_convertible_v<_OtherElementType (*)[], element_type (*)[]>
  constexpr explicit aligned_accessor(default_accessor<_OtherElementType>) noexcept {}
  template <class _OtherElementType>
    requires is_convertible_v<element_type (*)[], _OtherElementType (*)[]>
  constexpr operator default_accessor<_OtherElementType>() const noexcept {
    return {};
  }

  constexpr reference access(data_handle_type p, size_t i) const noexcept {
    return std::assume_aligned<byte_alignment>(p)[i];
  }
  constexpr typename offset_policy::data_handle_type offset(data_handle_type p, size_t i) const noexcept {
    return std::assume_aligned<byte_alignment>(p) + i;
  }
};

}} // namespace std
