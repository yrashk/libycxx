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

namespace std {

struct layout_left {
  template <class Extents>
  class mapping;
};
struct layout_right {
  template <class Extents>
  class mapping;
};
struct layout_stride {
  template <class Extents>
  class mapping;
};
template <size_t PaddingValue = dynamic_extent>
struct layout_left_padded {
  template <class Extents>
  class mapping;
};
template <size_t PaddingValue = dynamic_extent>
struct layout_right_padded {
  template <class Extents>
  class mapping;
};

} // namespace std

namespace ycxx::detail {

// is-mapping-of, is-layout-left-padded-mapping-of, is-layout-right-padded-mapping-of
// ([mdspan.layout.general]/2).
template <class Layout, class Mapping>
concept md_mapping_of = requires { typename Mapping::extents_type; } &&
                        std::is_same_v<typename Layout::template mapping<typename Mapping::extents_type>, Mapping>;
template <class M>
concept md_left_padded_mapping =
    requires { typename std::integral_constant<std::size_t, M::padding_value>; } &&
    md_mapping_of<std::layout_left_padded<M::padding_value>, M>;
template <class M>
concept md_right_padded_mapping =
    requires { typename std::integral_constant<std::size_t, M::padding_value>; } &&
    md_mapping_of<std::layout_right_padded<M::padding_value>, M>;

// layout-mapping-alike ([mdspan.layout.stride.expo]/4)
template <class M>
concept md_layout_mapping_alike = requires {
  requires md_is_extents<typename M::extents_type>;
  { M::is_always_strided() } -> std::same_as<bool>;
  { M::is_always_exhaustive() } -> std::same_as<bool>;
  { M::is_always_unique() } -> std::same_as<bool>;
  std::bool_constant<M::is_always_strided()>::value;
  std::bool_constant<M::is_always_exhaustive()>::value;
  std::bool_constant<M::is_always_unique()>::value;
};

// static-padding-stride of a padded layout of rank Rank whose padded extent (the first for
// layout_left_padded, the last for layout_right_padded) has the static value Ext. An overflow
// gives 0; the mappings' Mandates reject it.
consteval std::size_t md_static_padding_stride(std::size_t padding, std::size_t rank, std::size_t ext) {
  if (rank <= 1)
    return 0;
  if (padding == std::dynamic_extent || ext == std::dynamic_extent)
    return std::dynamic_extent;
  std::size_t r = 0;
  if (!::ycxx::detail::md_least_multiple(padding, ext, r))
    return 0;
  return r;
}
template <class M>
consteval std::size_t md_left_pad_stride_of() {
  using E = typename M::extents_type;
  return ::ycxx::detail::md_static_padding_stride(M::padding_value, E::rank(),
                                                  E::rank() == 0 ? 0 : E::static_extent(0));
}
template <class M>
consteval std::size_t md_right_pad_stride_of() {
  using E = typename M::extents_type;
  return ::ycxx::detail::md_static_padding_stride(M::padding_value, E::rank(),
                                                  E::rank() == 0 ? 0 : E::static_extent(E::rank() - 1));
}

// The Mandates (5.3)/(5.4) of the padded layouts: LEAST-MULTIPLE-AT-LEAST(padding, ext) and its
// product with the other static extents are representable as size_t and IndexType.
template <class IndexType, class E, std::size_t Padding, bool Left>
consteval bool md_padded_static_ok() {
  constexpr std::size_t rank = E::rank();
  if constexpr (rank <= 1 || Padding == std::dynamic_extent) {
    return true;
  } else {
    constexpr std::size_t padded = Left ? 0 : rank - 1;
    if (E::static_extent(padded) == std::dynamic_extent)
      return true;
    std::size_t s = 0;
    if (!::ycxx::detail::md_least_multiple(Padding, E::static_extent(padded), s) || !std::in_range<IndexType>(s))
      return false;
    for (std::size_t k = 0; k < rank; ++k)
      if (E::static_extent(k) == std::dynamic_extent)
        return true;
    for (std::size_t k = 0; k < rank; ++k)
      if (k != padded && E::static_extent(k) == 0)
        return true; // the product is 0
    for (std::size_t k = 0; k < rank; ++k)
      if (k != padded && !::ycxx::detail::md_mul(s, E::static_extent(k), s))
        return false;
    return std::in_range<IndexType>(s);
  }
}

// Preconditions of the padded constructors: the padding stride s and the product of s with
// every extent other than the padded one are representable as IndexType (and so is the size).
template <class IndexType, class E>
constexpr bool md_padded_fits(const E& e, std::size_t padded, std::size_t stride) noexcept {
  if (!std::in_range<IndexType>(stride) || !::ycxx::detail::md_size_fits<IndexType>(e))
    return false;
  if (stride == 0)
    return true;
  for (std::size_t k = 0; k < E::rank(); ++k)
    if (e.extent(k) == 0)
      return true;
  IndexType p = static_cast<IndexType>(stride);
  for (std::size_t k = 0; k < E::rank(); ++k)
    if (k != padded && !::ycxx::detail::md_mul(p, static_cast<IndexType>(e.extent(k)), p))
      return false;
  return true;
}

// The padding stride, stored only when it is not static.
template <class I, std::size_t S>
struct md_padding_stride {
  constexpr md_padding_stride() noexcept = default;
  constexpr md_padding_stride(I) noexcept {}
  static constexpr I get() noexcept { return static_cast<I>(S); }
};
template <class I>
struct md_padding_stride<I, std::dynamic_extent> {
  I v{};
  constexpr md_padding_stride() noexcept = default;
  constexpr md_padding_stride(I s) noexcept : v(s) {}
  constexpr I get() const noexcept { return v; }
};

// OFFSET(m) ([mdspan.layout.stride.expo]/2).
template <class M>
constexpr auto md_offset(const M& m) {
  using E = typename M::extents_type;
  if constexpr (E::rank() == 0) {
    return m();
  } else {
    using I = typename M::index_type;
    for (std::size_t r = 0; r < E::rank(); ++r)
      if (m.extents().extent(r) == 0)
        return I(0);
    return [&]<std::size_t... K>(std::index_sequence<K...>) -> I {
      return m(((void)K, I(0))...);
    }(std::make_index_sequence<E::rank()>());
  }
}

// Converts the indices of a mapping call to index_type, checking that they form a
// multidimensional index in e.
template <class E, class... Indices>
constexpr std::array<typename E::index_type, sizeof...(Indices)> md_indices(const E& e, Indices... i) noexcept {
  using I = typename E::index_type;
  bool ok = true;
  std::size_t r = 0;
  std::array<I, sizeof...(Indices)> idx{::ycxx::detail::md_cast_index<I>(std::move(i), e.extent(r++), ok)...};
  ::ycxx::detail::precondition(ok, "mdspan layout mapping: index is not a multidimensional index in extents()");
  return idx;
}

// The size of the multidimensional index space e is 0.
template <class E>
constexpr bool md_empty(const E& e) noexcept {
  for (std::size_t r = 0; r < E::rank(); ++r)
    if (e.extent(r) == 0)
      return true;
  return false;
}

} // namespace ycxx::detail

namespace std {

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.left]
// ---------------------------------------------------------------------------------------------
template <class Extents>
class layout_left::mapping {
  static_assert(ycxx::detail::md_is_extents<Extents>, "layout_left::mapping: Extents must be a specialization of extents");
  static_assert(ycxx::detail::md_static_size_fits<Extents>(),
                "layout_left::mapping: the size of Extents() must be representable as index_type");

public:
  using extents_type = Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_left;

  // [mdspan.layout.left.cons]
  constexpr mapping() noexcept = default;
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const extents_type& e) noexcept : extents_(e) {
    ycxx::detail::precondition(ycxx::detail::md_size_fits<index_type>(e),
                               "layout_left::mapping: the size of e is not representable as index_type");
  }
  template <class OtherExtents>
    requires is_constructible_v<extents_type, OtherExtents>
  constexpr explicit(!is_convertible_v<OtherExtents, extents_type>) mapping(const mapping<OtherExtents>& other) noexcept
      : extents_(other.extents()) {
    check_span(other.required_span_size());
  }
  template <class OtherExtents>
    requires(extents_type::rank() <= 1 && is_constructible_v<extents_type, OtherExtents>)
  constexpr explicit(!is_convertible_v<OtherExtents, extents_type>)
      mapping(const layout_right::mapping<OtherExtents>& other) noexcept
      : extents_(other.extents()) {
    check_span(other.required_span_size());
  }
  template <class LayoutLeftPaddedMapping>
    requires(ycxx::detail::md_left_padded_mapping<LayoutLeftPaddedMapping> &&
             is_constructible_v<extents_type, typename LayoutLeftPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename LayoutLeftPaddedMapping::extents_type, extents_type>)
      mapping(const LayoutLeftPaddedMapping& other) noexcept
      : extents_(other.extents()) {
    constexpr size_t pad = ycxx::detail::md_left_pad_stride_of<LayoutLeftPaddedMapping>();
    if constexpr (extents_type::rank() > 1) {
      static_assert(extents_type::static_extent(0) == dynamic_extent || pad == dynamic_extent ||
                        extents_type::static_extent(0) == pad,
                    "layout_left::mapping: static extent(0) differs from the source's static padding stride");
      ycxx::detail::precondition(cmp_equal(other.stride(1), other.extents().extent(0)),
                                 "layout_left::mapping: the source is padded (stride(1) != extent(0))");
    }
    check_span(other.required_span_size());
  }
  template <class OtherExtents>
    requires is_constructible_v<extents_type, OtherExtents>
  // noexcept: a strengthening; the draft declares it so for layout_right only.
  constexpr explicit(!(extents_type::rank() == 0 && is_convertible_v<OtherExtents, extents_type>))
      mapping(const layout_stride::mapping<OtherExtents>& other) noexcept
      : extents_(other.extents()) {
    if (ycxx::detail::md_checking()) {
      for (size_t r = 0; r < extents_type::rank(); ++r)
        ycxx::detail::precondition(cmp_equal(other.stride(r), ycxx::detail::md_fwd_prod(other.extents(), r)),
                                   "layout_left::mapping: the layout_stride mapping is not column-major");
    }
    check_span(other.required_span_size());
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.left.obs]
  constexpr const extents_type& extents() const noexcept { return extents_; }
  constexpr index_type required_span_size() const noexcept {
    return static_cast<index_type>(ycxx::detail::md_fwd_prod(extents_, extents_type::rank()));
  }
  template <class... Indices>
    requires(sizeof...(Indices) == extents_type::rank() && (is_convertible_v<Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, Indices> && ...))
  constexpr index_type operator()(Indices... i) const noexcept {
    if constexpr (sizeof...(Indices) == 0) {
      return 0;
    } else {
      auto idx = ycxx::detail::md_indices(extents_, std::move(i)...);
      index_type off = idx[sizeof...(Indices) - 1];
      for (size_t r = sizeof...(Indices) - 1; r-- > 0;)
        off = static_cast<index_type>(off * extents_.extent(r) + idx[r]);
      return off;
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
    ycxx::detail::precondition(i < extents_type::rank(), "layout_left::mapping::stride: index out of range");
    return static_cast<index_type>(ycxx::detail::md_fwd_prod(extents_, i));
  }
  template <class OtherExtents>
    requires(extents_type::rank() == OtherExtents::rank())
  friend constexpr bool operator==(const mapping& x, const mapping<OtherExtents>& y) noexcept {
    return x.extents() == y.extents();
  }

private:
  constexpr void check_span(auto n) const noexcept {
    ycxx::detail::precondition(in_range<index_type>(n),
                               "mdspan layout mapping: required_span_size() not representable as index_type");
  }

  extents_type extents_{};

  template <class... SliceSpecifiers>
  constexpr auto submdspan_mapping_impl(SliceSpecifiers... slices) const;
  template <class... SliceSpecifiers>
    requires(sizeof...(SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& src, SliceSpecifiers... slices) {
    return src.submdspan_mapping_impl(slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.right]
// ---------------------------------------------------------------------------------------------
template <class Extents>
class layout_right::mapping {
  static_assert(ycxx::detail::md_is_extents<Extents>, "layout_right::mapping: Extents must be a specialization of extents");
  static_assert(ycxx::detail::md_static_size_fits<Extents>(),
                "layout_right::mapping: the size of Extents() must be representable as index_type");

public:
  using extents_type = Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_right;

  // [mdspan.layout.right.cons]
  constexpr mapping() noexcept = default;
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const extents_type& e) noexcept : extents_(e) {
    ycxx::detail::precondition(ycxx::detail::md_size_fits<index_type>(e),
                               "layout_right::mapping: the size of e is not representable as index_type");
  }
  template <class OtherExtents>
    requires is_constructible_v<extents_type, OtherExtents>
  constexpr explicit(!is_convertible_v<OtherExtents, extents_type>) mapping(const mapping<OtherExtents>& other) noexcept
      : extents_(other.extents()) {
    check_span(other.required_span_size());
  }
  template <class OtherExtents>
    requires(extents_type::rank() <= 1 && is_constructible_v<extents_type, OtherExtents>)
  constexpr explicit(!is_convertible_v<OtherExtents, extents_type>)
      mapping(const layout_left::mapping<OtherExtents>& other) noexcept
      : extents_(other.extents()) {
    check_span(other.required_span_size());
  }
  template <class LayoutRightPaddedMapping>
    requires(ycxx::detail::md_right_padded_mapping<LayoutRightPaddedMapping> &&
             is_constructible_v<extents_type, typename LayoutRightPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename LayoutRightPaddedMapping::extents_type, extents_type>)
      mapping(const LayoutRightPaddedMapping& other) noexcept
      : extents_(other.extents()) {
    constexpr size_t rank = extents_type::rank();
    constexpr size_t pad = ycxx::detail::md_right_pad_stride_of<LayoutRightPaddedMapping>();
    if constexpr (rank > 1) {
      static_assert(extents_type::static_extent(rank - 1) == dynamic_extent || pad == dynamic_extent ||
                        extents_type::static_extent(rank - 1) == pad,
                    "layout_right::mapping: static extent(rank - 1) differs from the source's static padding stride");
      ycxx::detail::precondition(cmp_equal(other.stride(rank - 2), other.extents().extent(rank - 1)),
                                 "layout_right::mapping: the source is padded (stride(rank - 2) != extent(rank - 1))");
    }
    check_span(other.required_span_size());
  }
  template <class OtherExtents>
    requires is_constructible_v<extents_type, OtherExtents>
  constexpr explicit(!(extents_type::rank() == 0 && is_convertible_v<OtherExtents, extents_type>))
      mapping(const layout_stride::mapping<OtherExtents>& other) noexcept
      : extents_(other.extents()) {
    if (ycxx::detail::md_checking()) {
      for (size_t r = 0; r < extents_type::rank(); ++r)
        ycxx::detail::precondition(cmp_equal(other.stride(r), ycxx::detail::md_rev_prod(other.extents(), r)),
                                   "layout_right::mapping: the layout_stride mapping is not row-major");
    }
    check_span(other.required_span_size());
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.right.obs]
  constexpr const extents_type& extents() const noexcept { return extents_; }
  constexpr index_type required_span_size() const noexcept {
    return static_cast<index_type>(ycxx::detail::md_fwd_prod(extents_, extents_type::rank()));
  }
  template <class... Indices>
    requires(sizeof...(Indices) == extents_type::rank() && (is_convertible_v<Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, Indices> && ...))
  constexpr index_type operator()(Indices... i) const noexcept {
    if constexpr (sizeof...(Indices) == 0) {
      return 0;
    } else {
      auto idx = ycxx::detail::md_indices(extents_, std::move(i)...);
      index_type off = idx[0];
      for (size_t r = 1; r < sizeof...(Indices); ++r)
        off = static_cast<index_type>(off * extents_.extent(r) + idx[r]);
      return off;
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
    ycxx::detail::precondition(i < extents_type::rank(), "layout_right::mapping::stride: index out of range");
    return static_cast<index_type>(ycxx::detail::md_rev_prod(extents_, i));
  }
  template <class OtherExtents>
    requires(extents_type::rank() == OtherExtents::rank())
  friend constexpr bool operator==(const mapping& x, const mapping<OtherExtents>& y) noexcept {
    return x.extents() == y.extents();
  }

private:
  constexpr void check_span(auto n) const noexcept {
    ycxx::detail::precondition(in_range<index_type>(n),
                               "mdspan layout mapping: required_span_size() not representable as index_type");
  }

  extents_type extents_{};

  template <class... SliceSpecifiers>
  constexpr auto submdspan_mapping_impl(SliceSpecifiers... slices) const;
  template <class... SliceSpecifiers>
    requires(sizeof...(SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& src, SliceSpecifiers... slices) {
    return src.submdspan_mapping_impl(slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.stride]
// ---------------------------------------------------------------------------------------------
template <class Extents>
class layout_stride::mapping {
  static_assert(ycxx::detail::md_is_extents<Extents>, "layout_stride::mapping: Extents must be a specialization of extents");
  static_assert(ycxx::detail::md_static_size_fits<Extents>(),
                "layout_stride::mapping: the size of Extents() must be representable as index_type");

public:
  using extents_type = Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_stride;

private:
  static constexpr rank_type rank_ = extents_type::rank();

  // REQUIRED-SPAN-SIZE(e, strides) computed without overflow; false if it is not
  // representable as index_type ([mdspan.layout.stride.expo]/1).
  template <class Strides>
  static constexpr bool required_span(const extents_type& e, const Strides& s, index_type& out) noexcept {
    if constexpr (rank_ == 0) {
      out = 1;
      return true;
    } else {
      for (size_t r = 0; r < rank_; ++r)
        if (e.extent(r) == 0) {
          out = 0;
          return true;
        }
      index_type sum = 1;
      for (size_t r = 0; r < rank_; ++r) {
        index_type t = 0;
        if (!ycxx::detail::md_mul(static_cast<index_type>(e.extent(r) - 1), static_cast<index_type>(s[r]), t) ||
            !ycxx::detail::md_add(sum, t, sum))
          return false;
      }
      out = sum;
      return true;
    }
  }

  // The permutation of the rank indices ordering the strides ascending (ties: smaller extent
  // first), as used by [mdspan.layout.stride.cons]/4.3 and is_exhaustive().
  template <class Strides>
  static constexpr array<size_t, rank_> sorted(const extents_type& e, const Strides& s) noexcept {
    array<size_t, rank_> p{};
    for (size_t r = 0; r < rank_; ++r)
      p[r] = r;
    for (size_t i = 1; i < rank_; ++i)
      for (size_t j = i; j > 0; --j) {
        size_t a = p[j - 1], b = p[j];
        if (s[b] < s[a] || (s[b] == s[a] && e.extent(b) < e.extent(a))) {
          p[j - 1] = b;
          p[j] = a;
        } else {
          break;
        }
      }
    return p;
  }

  template <class Strides>
  constexpr void check_strides(const extents_type& e, const Strides& s) const noexcept {
    if (!ycxx::detail::md_checking())
      return;
    index_type n = 0;
    ycxx::detail::precondition(required_span(e, s, n),
                               "layout_stride::mapping: REQUIRED-SPAN-SIZE not representable as index_type");
    // Not diagnosed for an empty index space, where no stride is ever used: submdspan of an
    // empty mdspan can produce zero strides ([mdspan.sub.map.common]/6).
    if constexpr (rank_ > 0) {
      bool empty = false;
      for (size_t r = 0; r < rank_; ++r)
        empty = empty || e.extent(r) == 0;
      if (!empty) {
        bool positive = true;
        for (size_t r = 0; r < rank_; ++r)
          positive = positive && s[r] > 0;
        ycxx::detail::precondition(positive, "layout_stride::mapping: strides must be positive");
        auto p = sorted(e, s);
        bool unique = true;
        for (size_t i = 1; i < rank_; ++i) {
          index_type m = 0;
          unique = unique && ycxx::detail::md_mul(static_cast<index_type>(s[p[i - 1]]), e.extent(p[i - 1]), m) &&
                   s[p[i]] >= m;
        }
        ycxx::detail::precondition(unique, "layout_stride::mapping: the strides do not give a unique layout");
      }
    }
  }

public:
  // [mdspan.layout.stride.cons]
  constexpr mapping() noexcept : extents_(extents_type()) {
    ycxx::detail::precondition(
        in_range<index_type>(ycxx::detail::md_fwd_prod(extents_type(), rank_)),
        "layout_stride::mapping: the default extents' size is not representable as index_type");
    for (size_t d = 0; d < rank_; ++d)
      strides_[d] = static_cast<index_type>(ycxx::detail::md_rev_prod(extents_, d));
  }
  constexpr mapping(const mapping&) noexcept = default;
  template <class OtherIndexType>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&>)
  constexpr mapping(const extents_type& e, span<OtherIndexType, rank_> s) noexcept : extents_(e) {
    for (size_t d = 0; d < rank_; ++d)
      strides_[d] = static_cast<index_type>(as_const(s[d]));
    check_strides(e, strides_);
  }
  template <class OtherIndexType>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&>)
  constexpr mapping(const extents_type& e, const array<OtherIndexType, rank_>& s) noexcept : extents_(e) {
    for (size_t d = 0; d < rank_; ++d)
      strides_[d] = static_cast<index_type>(as_const(s[d]));
    check_strides(e, strides_);
  }
  template <class StridedLayoutMapping>
    requires(ycxx::detail::md_layout_mapping_alike<StridedLayoutMapping> &&
             is_constructible_v<extents_type, typename StridedLayoutMapping::extents_type> &&
             StridedLayoutMapping::is_always_unique() && StridedLayoutMapping::is_always_strided())
  constexpr explicit(!(is_convertible_v<typename StridedLayoutMapping::extents_type, extents_type> &&
                       (ycxx::detail::md_mapping_of<layout_left, StridedLayoutMapping> ||
                        ycxx::detail::md_mapping_of<layout_right, StridedLayoutMapping> ||
                        ycxx::detail::md_left_padded_mapping<StridedLayoutMapping> ||
                        ycxx::detail::md_right_padded_mapping<StridedLayoutMapping> ||
                        ycxx::detail::md_mapping_of<layout_stride, StridedLayoutMapping>)))
      mapping(const StridedLayoutMapping& other) noexcept
      : extents_(other.extents()) {
    if constexpr (rank_ > 0)
      for (size_t d = 0; d < rank_; ++d)
        strides_[d] = static_cast<index_type>(other.stride(d));
    if (ycxx::detail::md_checking()) {
      if constexpr (rank_ > 0)
        if (!ycxx::detail::md_empty(extents_))
          for (size_t d = 0; d < rank_; ++d)
            ycxx::detail::precondition(other.stride(d) > 0, "layout_stride::mapping: strides must be positive");
      ycxx::detail::precondition(in_range<index_type>(other.required_span_size()),
                                 "layout_stride::mapping: required_span_size() not representable as index_type");
      ycxx::detail::precondition(ycxx::detail::md_offset(other) == 0,
                                 "layout_stride::mapping: the source mapping has a nonzero offset");
    }
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.stride.obs]
  constexpr const extents_type& extents() const noexcept { return extents_; }
  constexpr array<index_type, rank_> strides() const noexcept { return strides_; }
  constexpr index_type required_span_size() const noexcept {
    index_type n = 0;
    required_span(extents_, strides_, n);
    return n;
  }
  template <class... Indices>
    requires(sizeof...(Indices) == rank_ && (is_convertible_v<Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, Indices> && ...))
  constexpr index_type operator()(Indices... i) const noexcept {
    auto idx = ycxx::detail::md_indices(extents_, std::move(i)...);
    index_type off = 0;
    for (size_t r = 0; r < rank_; ++r)
      off = static_cast<index_type>(off + idx[r] * strides_[r]);
    return off;
  }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept {
    if constexpr (rank_ == 0) {
      return true;
    } else {
      for (size_t r = 0; r < rank_; ++r)
        if (extents_type::static_extent(r) == 0)
          return true;
      return false;
    }
  }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_unique() noexcept { return true; }
  constexpr bool is_exhaustive() const noexcept {
    if constexpr (rank_ == 0) {
      return true;
    } else {
      for (size_t r = 0; r < rank_; ++r)
        if (extents_.extent(r) == 0)
          return true;
      auto p = sorted(extents_, strides_);
      if (strides_[p[0]] != 1)
        return false;
      for (size_t i = 1; i < rank_; ++i) {
        index_type m = 0;
        if (!ycxx::detail::md_mul(strides_[p[i - 1]], extents_.extent(p[i - 1]), m) || strides_[p[i]] != m)
          return false;
      }
      return true;
    }
  }
  static constexpr bool is_strided() noexcept { return true; }
  constexpr index_type stride(rank_type i) const noexcept {
    ycxx::detail::precondition(i < rank_, "layout_stride::mapping::stride: index out of range");
    return strides_[i];
  }
  template <class OtherMapping>
    requires(ycxx::detail::md_layout_mapping_alike<OtherMapping> && rank_ == OtherMapping::extents_type::rank() &&
             OtherMapping::is_always_strided())
  friend constexpr bool operator==(const mapping& x, const OtherMapping& y) noexcept {
    if (!(x.extents() == y.extents()) || ycxx::detail::md_offset(y) != 0)
      return false;
    if constexpr (rank_ > 0)
      for (size_t r = 0; r < rank_; ++r)
        if (!cmp_equal(x.stride(r), y.stride(r)))
          return false;
    return true;
  }

private:
  extents_type extents_{};
  array<index_type, rank_> strides_{};

  template <class... SliceSpecifiers>
  constexpr auto submdspan_mapping_impl(SliceSpecifiers... slices) const;
  template <class... SliceSpecifiers>
    requires(sizeof...(SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& src, SliceSpecifiers... slices) {
    return src.submdspan_mapping_impl(slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.leftpad]
// ---------------------------------------------------------------------------------------------
template <size_t PaddingValue>
template <class Extents>
class layout_left_padded<PaddingValue>::mapping {
  static_assert(ycxx::detail::md_is_extents<Extents>,
                "layout_left_padded::mapping: Extents must be a specialization of extents");

public:
  static constexpr size_t padding_value = PaddingValue;
  using extents_type = Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_left_padded<PaddingValue>;

private:
  static constexpr size_t rank_ = extents_type::rank();
  static constexpr size_t first_static_extent = rank_ == 0 ? dynamic_extent : extents_type::static_extent(0);
  static constexpr size_t static_padding_stride =
      ycxx::detail::md_static_padding_stride(padding_value, rank_, first_static_extent);

  static_assert(ycxx::detail::md_static_size_fits<Extents>(),
                "layout_left_padded::mapping: the size of Extents() must be representable as index_type");
  static_assert(padding_value == dynamic_extent || in_range<index_type>(padding_value),
                "layout_left_padded::mapping: padding_value must be representable as index_type");
  static_assert(ycxx::detail::md_padded_static_ok<index_type, extents_type, padding_value, true>(),
                "layout_left_padded::mapping: the padded static extents overflow index_type");

  // stride(1) for extents e and padding pad: LEAST-MULTIPLE-AT-LEAST(pad, e.extent(0)).
  static constexpr index_type padded_stride(const extents_type& e, size_t pad) noexcept {
    size_t s = 0;
    bool ok = ycxx::detail::md_least_multiple(pad, static_cast<size_t>(e.extent(0)), s);
    ycxx::detail::precondition(ok && ycxx::detail::md_padded_fits<index_type>(e, 0, s),
                               "layout_left_padded::mapping: the padded extents overflow index_type");
    return static_cast<index_type>(s);
  }

public:
  // [mdspan.layout.leftpad.cons]
  constexpr mapping() noexcept : mapping(extents_type{}) {}
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const extents_type& ext) : extents_(ext) {
    ycxx::detail::precondition(ycxx::detail::md_size_fits<index_type>(ext),
                               "layout_left_padded::mapping: the size of ext is not representable as index_type");
    if constexpr (rank_ > 1) {
      if constexpr (padding_value == dynamic_extent)
        stride_1_ = ext.extent(0);
      else
        stride_1_ = padded_stride(ext, padding_value);
    }
  }
  template <class OtherIndexType>
    requires(is_convertible_v<OtherIndexType, index_type> && is_nothrow_constructible_v<index_type, OtherIndexType>)
  constexpr mapping(const extents_type& ext, OtherIndexType padding) : extents_(ext) {
    auto pad = ycxx::detail::md_index_cast<index_type>(std::move(padding));
    // pad > 0 is not diagnosed for an empty index space: submdspan of an empty mdspan can
    // pass a zero padding stride ([mdspan.sub.map.left]/1.4).
    ycxx::detail::precondition((cmp_greater(ycxx::detail::md_as_int(pad), 0) || ycxx::detail::md_empty(ext)) &&
                                   in_range<index_type>(ycxx::detail::md_as_int(pad)),
                               "layout_left_padded::mapping: padding must be positive and representable");
    ycxx::detail::precondition(padding_value == dynamic_extent || cmp_equal(padding_value, ycxx::detail::md_as_int(pad)),
                               "layout_left_padded::mapping: padding differs from padding_value");
    ycxx::detail::precondition(ycxx::detail::md_size_fits<index_type>(ext),
                               "layout_left_padded::mapping: the size of ext is not representable as index_type");
    if constexpr (rank_ > 1)
      stride_1_ = padded_stride(ext, static_cast<size_t>(pad));
  }
  template <class OtherExtents>
    requires is_constructible_v<extents_type, OtherExtents>
  constexpr explicit(!is_convertible_v<OtherExtents, extents_type>)
      mapping(const layout_left::mapping<OtherExtents>& other)
      : mapping(extents_type(other.extents())) {
    if constexpr (OtherExtents::rank() > 1)
      static_assert(static_padding_stride == dynamic_extent || OtherExtents::static_extent(0) == dynamic_extent ||
                        static_padding_stride == OtherExtents::static_extent(0),
                    "layout_left_padded::mapping: the source's static extent(0) is not the static padding stride");
    if constexpr (rank_ > 1 && padding_value != dynamic_extent)
      ycxx::detail::precondition(cmp_equal(other.stride(1), stride_1_.get()),
                                 "layout_left_padded::mapping: extent(0) of the source is not padded");
    ycxx::detail::precondition(in_range<index_type>(other.required_span_size()),
                               "layout_left_padded::mapping: required_span_size() not representable");
  }
  template <class OtherExtents>
    requires is_constructible_v<extents_type, OtherExtents>
  constexpr explicit(!(rank_ == 0 && is_convertible_v<OtherExtents, extents_type>))
      mapping(const layout_stride::mapping<OtherExtents>& other)
      : extents_(other.extents()) {
    if constexpr (rank_ > 1)
      stride_1_ = static_cast<index_type>(other.stride(1));
    if (ycxx::detail::md_checking()) {
      if constexpr (rank_ > 1 && padding_value != dynamic_extent) {
        size_t s = 0;
        ycxx::detail::precondition(
            ycxx::detail::md_least_multiple(padding_value, static_cast<size_t>(extents_.extent(0)), s) &&
                cmp_equal(other.stride(1), s),
            "layout_left_padded::mapping: stride(1) of the source is not the padded extent(0)");
      }
      if constexpr (rank_ > 0)
        ycxx::detail::precondition(other.stride(0) == 1, "layout_left_padded::mapping: stride(0) of the source is not 1");
      for (size_t r = 2; r < rank_ && other.extents().extent(0) != 0; ++r)
        ycxx::detail::precondition(
            cmp_equal(other.stride(r), (ycxx::detail::md_fwd_prod(other.extents(), r) / other.extents().extent(0)) *
                                           static_cast<size_t>(other.stride(1))),
            "layout_left_padded::mapping: the source's strides are not those of a padded layout");
      ycxx::detail::precondition(in_range<index_type>(other.required_span_size()),
                                 "layout_left_padded::mapping: required_span_size() not representable");
    }
  }
  template <class LayoutLeftPaddedMapping>
    requires(ycxx::detail::md_left_padded_mapping<LayoutLeftPaddedMapping> &&
             is_constructible_v<extents_type, typename LayoutLeftPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename LayoutLeftPaddedMapping::extents_type, extents_type> ||
                     (rank_ > 1 && (padding_value != dynamic_extent ||
                                    LayoutLeftPaddedMapping::padding_value == dynamic_extent)))
      mapping(const LayoutLeftPaddedMapping& other)
      : extents_(other.extents()) {
    static_assert(rank_ <= 1 || padding_value == dynamic_extent ||
                      LayoutLeftPaddedMapping::padding_value == dynamic_extent ||
                      padding_value == LayoutLeftPaddedMapping::padding_value,
                  "layout_left_padded::mapping: the padding values differ");
    if constexpr (rank_ > 1) {
      stride_1_ = static_cast<index_type>(other.stride(1));
      if constexpr (padding_value != dynamic_extent) {
        size_t s = 0;
        ycxx::detail::precondition(
            ycxx::detail::md_least_multiple(padding_value, static_cast<size_t>(extents_.extent(0)), s) &&
                cmp_equal(other.stride(1), s),
            "layout_left_padded::mapping: stride(1) of the source is not the padded extent(0)");
      }
    }
    ycxx::detail::precondition(in_range<index_type>(other.required_span_size()),
                               "layout_left_padded::mapping: required_span_size() not representable");
  }
  template <class LayoutRightPaddedMapping>
    requires((ycxx::detail::md_right_padded_mapping<LayoutRightPaddedMapping> ||
              ycxx::detail::md_mapping_of<layout_right, LayoutRightPaddedMapping>) &&
             rank_ <= 1 && is_constructible_v<extents_type, typename LayoutRightPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename LayoutRightPaddedMapping::extents_type, extents_type>)
      mapping(const LayoutRightPaddedMapping& other) noexcept
      : extents_(other.extents()) {
    ycxx::detail::precondition(in_range<index_type>(other.required_span_size()),
                               "layout_left_padded::mapping: required_span_size() not representable");
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.leftpad.obs]
  constexpr const extents_type& extents() const noexcept { return extents_; }
  constexpr array<index_type, rank_> strides() const noexcept {
    array<index_type, rank_> s{};
    for (size_t r = 0; r < rank_; ++r)
      s[r] = stride(r);
    return s;
  }
  constexpr index_type required_span_size() const noexcept {
    for (size_t r = 0; r < rank_; ++r)
      if (extents_.extent(r) == 0)
        return 0;
    index_type n = 1;
    for (size_t r = 0; r < rank_; ++r)
      n = static_cast<index_type>(n + (extents_.extent(r) - 1) * stride(r));
    return n;
  }
  template <class... Indices>
    requires(sizeof...(Indices) == rank_ && (is_convertible_v<Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, Indices> && ...))
  constexpr index_type operator()(Indices... idxs) const noexcept {
    if constexpr (rank_ == 0) {
      return 0;
    } else {
      auto idx = ycxx::detail::md_indices(extents_, std::move(idxs)...);
      index_type off = idx[0], s = 1;
      for (size_t r = 1; r < rank_; ++r) {
        s = static_cast<index_type>(r == 1 ? stride_1_.get() : s * extents_.extent(r - 1));
        off = static_cast<index_type>(off + idx[r] * s);
      }
      return off;
    }
  }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept {
    if constexpr (rank_ <= 1)
      return true;
    else if constexpr (static_padding_stride != dynamic_extent && first_static_extent != dynamic_extent)
      return static_padding_stride == first_static_extent;
    else
      return false;
  }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_unique() noexcept { return true; }
  constexpr bool is_exhaustive() const noexcept {
    if constexpr (rank_ <= 1)
      return true;
    else
      return extents_.extent(0) == stride(1);
  }
  static constexpr bool is_strided() noexcept { return true; }
  constexpr index_type stride(rank_type r) const noexcept {
    ycxx::detail::precondition(r < rank_, "layout_left_padded::mapping::stride: index out of range");
    if (r == 0)
      return 1;
    // In size_t: for an empty index space the product need not be representable.
    size_t s = static_cast<size_t>(stride_1_.get());
    for (size_t k = 1; k < r; ++k)
      s *= static_cast<size_t>(extents_.extent(k));
    return static_cast<index_type>(s);
  }
  template <class LayoutLeftPaddedMapping>
    requires(ycxx::detail::md_left_padded_mapping<LayoutLeftPaddedMapping> &&
             LayoutLeftPaddedMapping::extents_type::rank() == rank_)
  friend constexpr bool operator==(const mapping& x, const LayoutLeftPaddedMapping& y) noexcept {
    if constexpr (rank_ < 2)
      return x.extents() == y.extents();
    else
      return x.extents() == y.extents() && cmp_equal(x.stride(1), y.stride(1));
  }

private:
  [[no_unique_address]] ycxx::detail::md_padding_stride<index_type, static_padding_stride> stride_1_{};
  extents_type extents_{};

  template <class... SliceSpecifiers>
  constexpr auto submdspan_mapping_impl(SliceSpecifiers... slices) const;
  template <class... SliceSpecifiers>
    requires(sizeof...(SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& src, SliceSpecifiers... slices) {
    return src.submdspan_mapping_impl(slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.layout.rightpad]
// ---------------------------------------------------------------------------------------------
template <size_t PaddingValue>
template <class Extents>
class layout_right_padded<PaddingValue>::mapping {
  static_assert(ycxx::detail::md_is_extents<Extents>,
                "layout_right_padded::mapping: Extents must be a specialization of extents");

public:
  static constexpr size_t padding_value = PaddingValue;
  using extents_type = Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = layout_right_padded<PaddingValue>;

private:
  static constexpr size_t rank_ = extents_type::rank();
  static constexpr size_t last_static_extent = rank_ == 0 ? dynamic_extent : extents_type::static_extent(rank_ - 1);
  static constexpr size_t static_padding_stride =
      ycxx::detail::md_static_padding_stride(padding_value, rank_, last_static_extent);

  static_assert(ycxx::detail::md_static_size_fits<Extents>(),
                "layout_right_padded::mapping: the size of Extents() must be representable as index_type");
  static_assert(padding_value == dynamic_extent || in_range<index_type>(padding_value),
                "layout_right_padded::mapping: padding_value must be representable as index_type");
  static_assert(ycxx::detail::md_padded_static_ok<index_type, extents_type, padding_value, false>(),
                "layout_right_padded::mapping: the padded static extents overflow index_type");

  static constexpr index_type padded_stride(const extents_type& e, size_t pad) noexcept {
    size_t s = 0;
    bool ok = ycxx::detail::md_least_multiple(pad, static_cast<size_t>(e.extent(rank_ - 1)), s);
    ycxx::detail::precondition(ok && ycxx::detail::md_padded_fits<index_type>(e, rank_ - 1, s),
                               "layout_right_padded::mapping: the padded extents overflow index_type");
    return static_cast<index_type>(s);
  }

public:
  // [mdspan.layout.rightpad.cons]
  constexpr mapping() noexcept : mapping(extents_type{}) {}
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const extents_type& ext) : extents_(ext) {
    ycxx::detail::precondition(ycxx::detail::md_size_fits<index_type>(ext),
                               "layout_right_padded::mapping: the size of ext is not representable as index_type");
    if constexpr (rank_ > 1) {
      if constexpr (padding_value == dynamic_extent)
        stride_rm2_ = ext.extent(rank_ - 1);
      else
        stride_rm2_ = padded_stride(ext, padding_value);
    }
  }
  template <class OtherIndexType>
    requires(is_convertible_v<OtherIndexType, index_type> && is_nothrow_constructible_v<index_type, OtherIndexType>)
  constexpr mapping(const extents_type& ext, OtherIndexType padding) : extents_(ext) {
    auto pad = ycxx::detail::md_index_cast<index_type>(std::move(padding));
    // pad > 0 is not diagnosed for an empty index space: submdspan of an empty mdspan can
    // pass a zero padding stride ([mdspan.sub.map.left]/1.4).
    ycxx::detail::precondition((cmp_greater(ycxx::detail::md_as_int(pad), 0) || ycxx::detail::md_empty(ext)) &&
                                   in_range<index_type>(ycxx::detail::md_as_int(pad)),
                               "layout_right_padded::mapping: padding must be positive and representable");
    ycxx::detail::precondition(padding_value == dynamic_extent || cmp_equal(padding_value, ycxx::detail::md_as_int(pad)),
                               "layout_right_padded::mapping: padding differs from padding_value");
    ycxx::detail::precondition(ycxx::detail::md_size_fits<index_type>(ext),
                               "layout_right_padded::mapping: the size of ext is not representable as index_type");
    if constexpr (rank_ > 1)
      stride_rm2_ = padded_stride(ext, static_cast<size_t>(pad));
  }
  template <class OtherExtents>
    requires is_constructible_v<extents_type, OtherExtents>
  constexpr explicit(!is_convertible_v<OtherExtents, extents_type>)
      mapping(const layout_right::mapping<OtherExtents>& other)
      : mapping(extents_type(other.extents())) {
    if constexpr (OtherExtents::rank() > 1)
      static_assert(static_padding_stride == dynamic_extent ||
                        OtherExtents::static_extent(rank_ - 1) == dynamic_extent ||
                        static_padding_stride == OtherExtents::static_extent(rank_ - 1),
                    "layout_right_padded::mapping: the source's static last extent is not the static padding stride");
    if constexpr (rank_ > 1 && padding_value != dynamic_extent)
      ycxx::detail::precondition(cmp_equal(other.stride(rank_ - 2), stride_rm2_.get()),
                                 "layout_right_padded::mapping: the last extent of the source is not padded");
    ycxx::detail::precondition(in_range<index_type>(other.required_span_size()),
                               "layout_right_padded::mapping: required_span_size() not representable");
  }
  template <class OtherExtents>
    requires is_constructible_v<extents_type, OtherExtents>
  constexpr explicit(!(rank_ == 0 && is_convertible_v<OtherExtents, extents_type>))
      mapping(const layout_stride::mapping<OtherExtents>& other)
      : extents_(other.extents()) {
    if constexpr (rank_ > 1)
      stride_rm2_ = static_cast<index_type>(other.stride(rank_ - 2));
    if (ycxx::detail::md_checking()) {
      if constexpr (rank_ > 1 && padding_value != dynamic_extent) {
        size_t s = 0;
        ycxx::detail::precondition(
            ycxx::detail::md_least_multiple(padding_value, static_cast<size_t>(extents_.extent(rank_ - 1)), s) &&
                cmp_equal(other.stride(rank_ - 2), s),
            "layout_right_padded::mapping: stride(rank - 2) of the source is not the padded last extent");
      }
      if constexpr (rank_ > 0)
        ycxx::detail::precondition(other.stride(rank_ - 1) == 1,
                                   "layout_right_padded::mapping: stride(rank - 1) of the source is not 1");
      if constexpr (rank_ > 2) {
        for (size_t r = 0; r < rank_ - 2 && other.extents().extent(rank_ - 1) != 0; ++r)
          ycxx::detail::precondition(
              cmp_equal(other.stride(r), (ycxx::detail::md_rev_prod(other.extents(), r) /
                                          other.extents().extent(rank_ - 1)) *
                                             static_cast<size_t>(other.stride(rank_ - 2))),
              "layout_right_padded::mapping: the source's strides are not those of a padded layout");
      }
      ycxx::detail::precondition(in_range<index_type>(other.required_span_size()),
                                 "layout_right_padded::mapping: required_span_size() not representable");
    }
  }
  template <class LayoutRightPaddedMapping>
    requires(ycxx::detail::md_right_padded_mapping<LayoutRightPaddedMapping> &&
             is_constructible_v<extents_type, typename LayoutRightPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename LayoutRightPaddedMapping::extents_type, extents_type> ||
                     (rank_ > 1 && (padding_value != dynamic_extent ||
                                    LayoutRightPaddedMapping::padding_value == dynamic_extent)))
      mapping(const LayoutRightPaddedMapping& other)
      : extents_(other.extents()) {
    static_assert(rank_ <= 1 || padding_value == dynamic_extent ||
                      LayoutRightPaddedMapping::padding_value == dynamic_extent ||
                      padding_value == LayoutRightPaddedMapping::padding_value,
                  "layout_right_padded::mapping: the padding values differ");
    if constexpr (rank_ > 1) {
      stride_rm2_ = static_cast<index_type>(other.stride(rank_ - 2));
      if constexpr (padding_value != dynamic_extent) {
        size_t s = 0;
        ycxx::detail::precondition(
            ycxx::detail::md_least_multiple(padding_value, static_cast<size_t>(extents_.extent(rank_ - 1)), s) &&
                cmp_equal(other.stride(rank_ - 2), s),
            "layout_right_padded::mapping: stride(rank - 2) of the source is not the padded last extent");
      }
    }
    ycxx::detail::precondition(in_range<index_type>(other.required_span_size()),
                               "layout_right_padded::mapping: required_span_size() not representable");
  }
  template <class LayoutLeftPaddedMapping>
    requires((ycxx::detail::md_left_padded_mapping<LayoutLeftPaddedMapping> ||
              ycxx::detail::md_mapping_of<layout_left, LayoutLeftPaddedMapping>) &&
             rank_ <= 1 && is_constructible_v<extents_type, typename LayoutLeftPaddedMapping::extents_type>)
  constexpr explicit(!is_convertible_v<typename LayoutLeftPaddedMapping::extents_type, extents_type>)
      mapping(const LayoutLeftPaddedMapping& other) noexcept
      : extents_(other.extents()) {
    ycxx::detail::precondition(in_range<index_type>(other.required_span_size()),
                               "layout_right_padded::mapping: required_span_size() not representable");
  }
  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.rightpad.obs]
  constexpr const extents_type& extents() const noexcept { return extents_; }
  constexpr array<index_type, rank_> strides() const noexcept {
    array<index_type, rank_> s{};
    for (size_t r = 0; r < rank_; ++r)
      s[r] = stride(r);
    return s;
  }
  constexpr index_type required_span_size() const noexcept {
    for (size_t r = 0; r < rank_; ++r)
      if (extents_.extent(r) == 0)
        return 0;
    index_type n = 1;
    for (size_t r = 0; r < rank_; ++r)
      n = static_cast<index_type>(n + (extents_.extent(r) - 1) * stride(r));
    return n;
  }
  template <class... Indices>
    requires(sizeof...(Indices) == rank_ && (is_convertible_v<Indices, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, Indices> && ...))
  constexpr index_type operator()(Indices... idxs) const noexcept {
    if constexpr (rank_ == 0) {
      return 0;
    } else {
      auto idx = ycxx::detail::md_indices(extents_, std::move(idxs)...);
      index_type off = idx[rank_ - 1], s = 1;
      for (size_t r = rank_ - 1; r-- > 0;) {
        s = static_cast<index_type>(r == rank_ - 2 ? stride_rm2_.get() : s * extents_.extent(r + 1));
        off = static_cast<index_type>(off + idx[r] * s);
      }
      return off;
    }
  }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept {
    if constexpr (rank_ <= 1)
      return true;
    else if constexpr (static_padding_stride != dynamic_extent && last_static_extent != dynamic_extent)
      return static_padding_stride == last_static_extent;
    else
      return false;
  }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_unique() noexcept { return true; }
  constexpr bool is_exhaustive() const noexcept {
    if constexpr (rank_ <= 1)
      return true;
    else
      return extents_.extent(rank_ - 1) == stride(rank_ - 2);
  }
  static constexpr bool is_strided() noexcept { return true; }
  constexpr index_type stride(rank_type r) const noexcept {
    ycxx::detail::precondition(r < rank_, "layout_right_padded::mapping::stride: index out of range");
    if (r == rank_ - 1)
      return 1;
    // In size_t: for an empty index space the product need not be representable.
    size_t s = static_cast<size_t>(stride_rm2_.get());
    for (size_t k = r + 1; k < rank_ - 1; ++k)
      s *= static_cast<size_t>(extents_.extent(k));
    return static_cast<index_type>(s);
  }
  template <class LayoutRightPaddedMapping>
    requires(ycxx::detail::md_right_padded_mapping<LayoutRightPaddedMapping> &&
             LayoutRightPaddedMapping::extents_type::rank() == rank_)
  friend constexpr bool operator==(const mapping& x, const LayoutRightPaddedMapping& y) noexcept {
    if constexpr (rank_ < 2)
      return x.extents() == y.extents();
    else
      return x.extents() == y.extents() && cmp_equal(x.stride(rank_ - 2), y.stride(rank_ - 2));
  }

private:
  [[no_unique_address]] ycxx::detail::md_padding_stride<index_type, static_padding_stride> stride_rm2_{};
  extents_type extents_{};

  template <class... SliceSpecifiers>
  constexpr auto submdspan_mapping_impl(SliceSpecifiers... slices) const;
  template <class... SliceSpecifiers>
    requires(sizeof...(SliceSpecifiers) == extents_type::rank())
  friend constexpr auto submdspan_mapping(const mapping& src, SliceSpecifiers... slices) {
    return src.submdspan_mapping_impl(slices...);
  }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.accessor.default]
// ---------------------------------------------------------------------------------------------
template <class ElementType>
struct default_accessor {
  static_assert(is_object_v<ElementType> && !is_abstract_v<ElementType> && !is_array_v<ElementType>,
                "default_accessor: ElementType must be a complete object type, not abstract, not an array");

  using offset_policy = default_accessor;
  using element_type = ElementType;
  using reference = ElementType&;
  using data_handle_type = ElementType*;

  constexpr default_accessor() noexcept = default;
  template <class OtherElementType>
    requires is_convertible_v<OtherElementType (*)[], element_type (*)[]>
  constexpr default_accessor(default_accessor<OtherElementType>) noexcept {}

  constexpr reference access(data_handle_type p, size_t i) const noexcept { return p[i]; }
  constexpr data_handle_type offset(data_handle_type p, size_t i) const noexcept { return p + i; }
};

// ---------------------------------------------------------------------------------------------
// [mdspan.accessor.aligned]
// ---------------------------------------------------------------------------------------------
template <class ElementType, size_t ByteAlignment>
struct aligned_accessor {
  static_assert(ByteAlignment != 0 && (ByteAlignment & (ByteAlignment - 1)) == 0,
                "aligned_accessor: byte_alignment must be a power of two");
  static_assert(ByteAlignment >= alignof(ElementType), "aligned_accessor: byte_alignment must be at least alignof(ElementType)");
  static_assert(is_object_v<ElementType> && !is_abstract_v<ElementType> && !is_array_v<ElementType>,
                "aligned_accessor: ElementType must be a complete object type, not abstract, not an array");

  using offset_policy = default_accessor<ElementType>;
  using element_type = ElementType;
  using reference = ElementType&;
  using data_handle_type = ElementType*;

  static constexpr size_t byte_alignment = ByteAlignment;

  constexpr aligned_accessor() noexcept = default;
  template <class OtherElementType, size_t OtherByteAlignment>
    requires(is_convertible_v<OtherElementType (*)[], element_type (*)[]> && OtherByteAlignment >= byte_alignment)
  constexpr aligned_accessor(aligned_accessor<OtherElementType, OtherByteAlignment>) noexcept {}
  template <class OtherElementType>
    requires is_convertible_v<OtherElementType (*)[], element_type (*)[]>
  constexpr explicit aligned_accessor(default_accessor<OtherElementType>) noexcept {}
  template <class OtherElementType>
    requires is_convertible_v<element_type (*)[], OtherElementType (*)[]>
  constexpr operator default_accessor<OtherElementType>() const noexcept {
    return {};
  }

  constexpr reference access(data_handle_type p, size_t i) const noexcept { return std::assume_aligned<byte_alignment>(p)[i]; }
  constexpr typename offset_policy::data_handle_type offset(data_handle_type p, size_t i) const noexcept {
    return std::assume_aligned<byte_alignment>(p) + i;
  }
};

} // namespace std
