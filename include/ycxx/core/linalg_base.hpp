// libycxx core: <linalg> part 1 ([linalg.tags] through [linalg.conjtransposed]): the tag
// classes, layout_blas_packed, the exposition-only helpers and argument concepts, and the
// in-place transformations scaled, conjugated, transposed and conjugate_transposed.
#pragma once

#include <ycxx/core/complex.hpp>
#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/mdspan.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace linalg {

// [linalg.tags.order]
struct column_major_t {
  explicit column_major_t() = default;
};
inline constexpr column_major_t column_major{};
struct row_major_t {
  explicit row_major_t() = default;
};
inline constexpr row_major_t row_major{};

// [linalg.tags.triangle]
struct upper_triangle_t {
  explicit upper_triangle_t() = default;
};
inline constexpr upper_triangle_t upper_triangle{};
struct lower_triangle_t {
  explicit lower_triangle_t() = default;
};
inline constexpr lower_triangle_t lower_triangle{};

// [linalg.tags.diagonal]
struct implicit_unit_diagonal_t {
  explicit implicit_unit_diagonal_t() = default;
};
inline constexpr implicit_unit_diagonal_t implicit_unit_diagonal{};
struct explicit_diagonal_t {
  explicit explicit_diagonal_t() = default;
};
inline constexpr explicit_diagonal_t explicit_diagonal{};

// ---------------------------------------------------------------------------------------------
// [linalg.layout.packed]
// ---------------------------------------------------------------------------------------------
template <class _Triangle, class _StorageOrder>
class layout_blas_packed {
public:
  using triangle_type = _Triangle;
  using storage_order_type = _StorageOrder;

  template <class _Extents>
  struct mapping {
    static_assert(is_same_v<_Triangle, upper_triangle_t> || is_same_v<_Triangle, lower_triangle_t>,
                  "layout_blas_packed: Triangle must be upper_triangle_t or lower_triangle_t");
    static_assert(is_same_v<_StorageOrder, column_major_t> || is_same_v<_StorageOrder, row_major_t>,
                  "layout_blas_packed: StorageOrder must be column_major_t or row_major_t");
    static_assert(__ycxx::__detail::__md_is_extents<_Extents>,
                  "layout_blas_packed: Extents must be a specialization of extents");
    static_assert(_Extents::rank() == 2, "layout_blas_packed: Extents::rank() must be 2");

  public:
    using extents_type = _Extents;
    using index_type = typename extents_type::index_type;
    using size_type = typename extents_type::size_type;
    using rank_type = typename extents_type::rank_type;
    using layout_type = layout_blas_packed;

  private:
    static_assert(extents_type::static_extent(0) == dynamic_extent ||
                      extents_type::static_extent(1) == dynamic_extent ||
                      extents_type::static_extent(0) == extents_type::static_extent(1),
                  "layout_blas_packed: the static extents of a packed matrix must be equal");
    // N * (N + 1) representable as index_type.
    static constexpr bool __packed_fits(size_t n) noexcept {
      index_type p = 0;
      return in_range<index_type>(n) &&
             __ycxx::__detail::__md_mul(static_cast<index_type>(n), static_cast<index_type>(n + 1), p);
    }
    static consteval bool __static_fits() {
      if constexpr (extents_type::rank_dynamic() != 0)
        return true;
      else
        return __packed_fits(extents_type::static_extent(0));
    }
    static_assert(__static_fits(), "layout_blas_packed: Ns * (Ns + 1) must be representable as index_type");

  public:
    // [linalg.layout.packed.cons]
    constexpr mapping() noexcept = default;
    constexpr mapping(const mapping&) noexcept = default;
    constexpr mapping(const extents_type& e) noexcept : __extents_(e) {
      __ycxx::__detail::__precondition(e.extent(0) == e.extent(1), "layout_blas_packed: the matrix must be square");
      __ycxx::__detail::__precondition(__packed_fits(static_cast<size_t>(e.extent(0))),
                                 "layout_blas_packed: N * (N + 1) is not representable as index_type");
    }
    template <class _OtherExtents>
      requires is_constructible_v<extents_type, _OtherExtents>
    constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>)
        mapping(const mapping<_OtherExtents>& other) noexcept
        : __extents_(other.extents()) {
      __ycxx::__detail::__precondition(__packed_fits(static_cast<size_t>(other.extents().extent(0))),
                                 "layout_blas_packed: N * (N + 1) is not representable as index_type");
    }
    constexpr mapping& operator=(const mapping&) noexcept = default;

    // [linalg.layout.packed.obs]
    constexpr const extents_type& extents() const noexcept { return __extents_; }
    constexpr index_type required_span_size() const noexcept {
      return static_cast<index_type>(__extents_.extent(0) * (__extents_.extent(0) + 1) / 2);
    }
    template <class _Index0, class _Index1>
      requires(is_convertible_v<_Index0, index_type> && is_convertible_v<_Index1, index_type> &&
               is_nothrow_constructible_v<index_type, _Index0> && is_nothrow_constructible_v<index_type, _Index1>)
    constexpr index_type operator()(_Index0 __ind0, _Index1 __ind1) const noexcept {
      auto __idx = __ycxx::__detail::__md_indices(__extents_, std::move(__ind0), std::move(__ind1));
      index_type i = __idx[0], __j = __idx[1];
      if (i > __j) {
        index_type t = i;
        i = __j;
        __j = t;
      }
      if constexpr ((is_same_v<_StorageOrder, column_major_t> && is_same_v<_Triangle, upper_triangle_t>) ||
                    (is_same_v<_StorageOrder, row_major_t> && is_same_v<_Triangle, lower_triangle_t>))
        return static_cast<index_type>(i + __j * (__j + 1) / 2);
      else
        return static_cast<index_type>(__j + __extents_.extent(0) * i - i * (i + 1) / 2);
    }
    static constexpr bool is_always_unique() noexcept {
      return (extents_type::static_extent(0) != dynamic_extent && extents_type::static_extent(0) < 2) ||
             (extents_type::static_extent(1) != dynamic_extent && extents_type::static_extent(1) < 2);
    }
    static constexpr bool is_always_exhaustive() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return is_always_unique(); }
    constexpr bool is_unique() const noexcept { return __extents_.extent(0) < 2; }
    constexpr bool is_exhaustive() const noexcept { return true; }
    constexpr bool is_strided() const noexcept { return __extents_.extent(0) < 2; }
    constexpr index_type stride(rank_type r) const noexcept {
      __ycxx::__detail::__precondition(is_strided() && r < extents_type::rank(),
                                 "layout_blas_packed::mapping::stride: the mapping is not strided");
      return 1;
    }
    template <class _OtherExtents>
    friend constexpr bool operator==(const mapping& __x, const mapping<_OtherExtents>& y) noexcept {
      return __x.extents() == y.extents();
    }

  private:
    extents_type __extents_{};
  };
};

}}} // namespace std::linalg

// ---------------------------------------------------------------------------------------------
// [linalg.helpers]: abs-if-needed, conj-if-needed, real-if-needed, imag-if-needed. The
// unqualified calls are made here, next to deleted templates that hide nothing but stop the
// lookup from reaching std:: functions for arithmetic arguments ([linalg.general]/6).
// ---------------------------------------------------------------------------------------------
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__la_adl {

template <class _Up>
_Up abs(_Up) = delete;
template <class _Up>
_Up conj(const _Up&) = delete;
template <class _Up>
_Up real(const _Up&) = delete;
template <class _Up>
_Up imag(const _Up&) = delete;

template <class _Tp>
concept __has_abs = requires(_Tp t) { abs(t); };
template <class _Tp>
concept __has_conj = requires(const _Tp& t) { conj(t); };
template <class _Tp>
concept __has_real = requires(const _Tp& t) { real(t); };
template <class _Tp>
concept __has_imag = requires(const _Tp& t) { imag(t); };

template <class _Tp>
constexpr auto __abs_if_needed(const _Tp& e) {
  if constexpr (std::is_integral_v<_Tp> && std::is_unsigned_v<_Tp>)
    return e;
  else if constexpr (std::is_arithmetic_v<_Tp>)
    return std::abs(e);
  else
    return abs(e);
}
template <class _Tp>
constexpr auto __conj_if_needed(const _Tp& e) {
  if constexpr (!std::is_arithmetic_v<_Tp> && __has_conj<_Tp>)
    return conj(e);
  else
    return e;
}
template <class _Tp>
constexpr auto __real_if_needed(const _Tp& e) {
  if constexpr (!std::is_arithmetic_v<_Tp> && __has_real<_Tp>)
    return real(e);
  else
    return e;
}
template <class _Tp>
constexpr auto __imag_if_needed(const _Tp& e) {
  if constexpr (!std::is_arithmetic_v<_Tp> && __has_imag<_Tp>)
    return imag(e);
  else
    return ((void)e, _Tp{});
}

}} // namespace __ycxx::__detail::__la_adl

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// [linalg.helpers.concepts]
template <class _Tp>
concept __la_in_vector = __md_is_mdspan<_Tp> && _Tp::rank() == 1;
template <class _Tp>
concept __la_out_vector = __md_is_mdspan<_Tp> && _Tp::rank() == 1 &&
                        std::is_assignable_v<typename _Tp::reference, typename _Tp::element_type> && _Tp::is_always_unique();
template <class _Tp>
concept __la_inout_vector = __la_out_vector<_Tp>;
template <class _Tp>
concept __la_in_matrix = __md_is_mdspan<_Tp> && _Tp::rank() == 2;
template <class _Tp>
concept __la_out_matrix = __md_is_mdspan<_Tp> && _Tp::rank() == 2 &&
                        std::is_assignable_v<typename _Tp::reference, typename _Tp::element_type> && _Tp::is_always_unique();
template <class _Tp>
concept __la_inout_matrix = __la_out_matrix<_Tp>;
template <class _Tp>
inline constexpr bool __la_is_layout_blas_packed = false;
template <class _Triangle, class _StorageOrder>
inline constexpr bool __la_is_layout_blas_packed<std::linalg::layout_blas_packed<_Triangle, _StorageOrder>> = true;
template <class _Tp>
concept __la_possibly_packed_out_matrix =
    __md_is_mdspan<_Tp> && _Tp::rank() == 2 && std::is_assignable_v<typename _Tp::reference, typename _Tp::element_type> &&
    (_Tp::is_always_unique() || __la_is_layout_blas_packed<typename _Tp::layout_type>);
template <class _Tp>
concept __la_in_object = __md_is_mdspan<_Tp> && (_Tp::rank() == 1 || _Tp::rank() == 2);
template <class _Tp>
concept __la_out_object = __md_is_mdspan<_Tp> && (_Tp::rank() == 1 || _Tp::rank() == 2) &&
                        std::is_assignable_v<typename _Tp::reference, typename _Tp::element_type> && _Tp::is_always_unique();
template <class _Tp>
concept __la_inout_object = __la_out_object<_Tp>;
template <class _Tp>
concept __la_scalar = std::semiregular<_Tp> && !__md_is_mdspan<_Tp> && !std::is_execution_policy_v<_Tp>;

// The template parameter name constraints of [linalg.algs.reqs].
template <class _Tp>
concept __la_triangle =
    std::is_same_v<_Tp, std::linalg::upper_triangle_t> || std::is_same_v<_Tp, std::linalg::lower_triangle_t>;
template <class _Tp>
concept __la_diagonal =
    std::is_same_v<_Tp, std::linalg::implicit_unit_diagonal_t> || std::is_same_v<_Tp, std::linalg::explicit_diagonal_t>;
template <class _Tp>
concept __la_real = std::is_floating_point_v<_Tp> && std::is_same_v<_Tp, std::remove_cv_t<_Tp>>;

// [linalg.helpers.mandates]
template <class _MDS1, class _MDS2>
consteval bool __la_compatible(std::size_t __r1, std::size_t __r2) {
  return _MDS1::static_extent(__r1) == std::dynamic_extent || _MDS2::static_extent(__r2) == std::dynamic_extent ||
         _MDS1::static_extent(__r1) == _MDS2::static_extent(__r2);
}
template <class _In1, class _In2, class _Out>
consteval bool __la_possibly_addable() {
  if constexpr (_In1::rank() == 1)
    return __la_compatible<_Out, _In1>(0, 0) && __la_compatible<_Out, _In2>(0, 0) && __la_compatible<_In1, _In2>(0, 0);
  else
    return __la_compatible<_Out, _In1>(0, 0) && __la_compatible<_Out, _In1>(1, 1) && __la_compatible<_Out, _In2>(0, 0) &&
           __la_compatible<_Out, _In2>(1, 1) && __la_compatible<_In1, _In2>(0, 0) && __la_compatible<_In1, _In2>(1, 1);
}
template <class _Ap, class _Bp, class _Out>
consteval bool __la_possibly_multipliable() {
  if constexpr (_Ap::rank() == 2 && _Bp::rank() == 1)
    return __la_compatible<_Out, _Ap>(0, 0) && __la_compatible<_Ap, _Bp>(1, 0);
  else if constexpr (_Ap::rank() == 1)
    return __la_compatible<_Out, _Bp>(0, 1) && __la_compatible<_Bp, _Ap>(0, 0);
  else
    return __la_compatible<_Out, _Ap>(0, 0) && __la_compatible<_Out, _Bp>(1, 1) && __la_compatible<_Ap, _Bp>(1, 0);
}

// [linalg.helpers.precond]
template <class _In1, class _In2, class _Out>
constexpr bool __la_addable(const _In1& in1, const _In2& in2, const _Out& out) {
  for (std::size_t r = 0; r < _Out::rank(); ++r)
    if (!std::cmp_equal(out.extent(r), in1.extent(r)) || !std::cmp_equal(out.extent(r), in2.extent(r)))
      return false;
  return true;
}
template <class _Ap, class _Bp, class _Out>
constexpr bool __la_multipliable(const _Ap& a, const _Bp& b, const _Out& out) {
  if constexpr (_Ap::rank() == 2 && _Bp::rank() == 1)
    return std::cmp_equal(out.extent(0), a.extent(0)) && std::cmp_equal(a.extent(1), b.extent(0));
  else if constexpr (_Ap::rank() == 1)
    return std::cmp_equal(out.extent(0), b.extent(1)) && std::cmp_equal(b.extent(0), a.extent(0));
  else
    return std::cmp_equal(out.extent(0), a.extent(0)) && std::cmp_equal(out.extent(1), b.extent(1)) &&
           std::cmp_equal(a.extent(1), b.extent(0));
}

// The Triangle of a layout_blas_packed matrix, or void.
template <class _Layout>
struct __la_packed_triangle {
  using type = void;
};
template <class _Tp, class _Sp>
struct __la_packed_triangle<std::linalg::layout_blas_packed<_Tp, _Sp>> {
  using type = _Tp;
};
// "If M has layout_blas_packed layout, then the layout's Triangle template argument has the
// same type as the function's Triangle template argument."
template <class _Mp, class _Triangle>
inline constexpr bool __la_triangle_matches =
    !__la_is_layout_blas_packed<typename _Mp::layout_type> ||
    std::is_same_v<typename __la_packed_triangle<typename _Mp::layout_type>::type, _Triangle>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace linalg {

// ---------------------------------------------------------------------------------------------
// [linalg.scaled]
// ---------------------------------------------------------------------------------------------
template <class _ScalingFactor, class _NestedAccessor>
class scaled_accessor {
public:
  using element_type = const decltype(declval<_ScalingFactor>() * declval<typename _NestedAccessor::element_type>());
  using reference = remove_const_t<element_type>;
  using data_handle_type = typename _NestedAccessor::data_handle_type;
  using offset_policy = scaled_accessor<_ScalingFactor, typename _NestedAccessor::offset_policy>;

  static_assert(is_copy_constructible_v<reference>, "scaled_accessor: reference must be copy constructible");
  static_assert(!is_reference_v<element_type>, "scaled_accessor: element_type must not be a reference");
  static_assert(semiregular<_ScalingFactor>, "scaled_accessor: ScalingFactor must model semiregular");

  constexpr scaled_accessor() = default;
  template <class _OtherNestedAccessor>
    requires is_constructible_v<_NestedAccessor, const _OtherNestedAccessor&>
  constexpr explicit(!is_convertible_v<_OtherNestedAccessor, _NestedAccessor>)
      scaled_accessor(const scaled_accessor<_ScalingFactor, _OtherNestedAccessor>& other)
      : __scaling_factor_(other.scaling_factor()), __nested_accessor_(other.nested_accessor()) {}
  constexpr scaled_accessor(const _ScalingFactor& s, const _NestedAccessor& a)
      : __scaling_factor_(s), __nested_accessor_(a) {}

  constexpr reference access(data_handle_type p, size_t i) const {
    return scaling_factor() * typename _NestedAccessor::element_type(__nested_accessor_.access(p, i));
  }
  constexpr typename offset_policy::data_handle_type offset(data_handle_type p, size_t i) const {
    return __nested_accessor_.offset(p, i);
  }
  constexpr const _ScalingFactor& scaling_factor() const noexcept { return __scaling_factor_; }
  constexpr const _NestedAccessor& nested_accessor() const noexcept { return __nested_accessor_; }

private:
  _ScalingFactor __scaling_factor_{};
  _NestedAccessor __nested_accessor_{};
};

template <class _ScalingFactor, class _ElementType, class _Extents, class _Layout, class _Accessor>
constexpr auto scaled(_ScalingFactor alpha, mdspan<_ElementType, _Extents, _Layout, _Accessor> __x) {
  using _SA = scaled_accessor<_ScalingFactor, _Accessor>;
  return mdspan<typename _SA::element_type, _Extents, _Layout, _SA>(__x.data_handle(), __x.mapping(), _SA(alpha, __x.accessor()));
}

// ---------------------------------------------------------------------------------------------
// [linalg.conj]
// ---------------------------------------------------------------------------------------------
template <class _NestedAccessor>
class conjugated_accessor {
public:
  using element_type =
      const decltype(__ycxx::__detail::__la_adl::__conj_if_needed(declval<typename _NestedAccessor::element_type>()));
  using reference = remove_const_t<element_type>;
  using data_handle_type = typename _NestedAccessor::data_handle_type;
  using offset_policy = conjugated_accessor<typename _NestedAccessor::offset_policy>;

  static_assert(is_copy_constructible_v<reference>, "conjugated_accessor: reference must be copy constructible");
  static_assert(!is_reference_v<element_type>, "conjugated_accessor: element_type must not be a reference");

  constexpr conjugated_accessor() = default;
  constexpr conjugated_accessor(const _NestedAccessor& __acc) : __nested_accessor_(__acc) {}
  template <class _OtherNestedAccessor>
    requires is_constructible_v<_NestedAccessor, const _OtherNestedAccessor&>
  constexpr explicit(!is_convertible_v<_OtherNestedAccessor, _NestedAccessor>)
      conjugated_accessor(const conjugated_accessor<_OtherNestedAccessor>& other)
      : __nested_accessor_(other.nested_accessor()) {}

  constexpr reference access(data_handle_type p, size_t i) const {
    return __ycxx::__detail::__la_adl::__conj_if_needed(typename _NestedAccessor::element_type(__nested_accessor_.access(p, i)));
  }
  constexpr typename offset_policy::data_handle_type offset(data_handle_type p, size_t i) const {
    return __nested_accessor_.offset(p, i);
  }
  constexpr const _NestedAccessor& nested_accessor() const noexcept { return __nested_accessor_; }

private:
  _NestedAccessor __nested_accessor_{};
};

}}} // namespace std::linalg

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
inline constexpr bool __la_is_conjugated_accessor = false;
template <class _Ap>
inline constexpr bool __la_is_conjugated_accessor<std::linalg::conjugated_accessor<_Ap>> = true;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace linalg {

template <class _ElementType, class _Extents, class _Layout, class _Accessor>
constexpr auto conjugated(mdspan<_ElementType, _Extents, _Layout, _Accessor> a) {
  using _Ep = remove_cvref_t<_ElementType>;
  if constexpr (__ycxx::__detail::__la_is_conjugated_accessor<_Accessor>) {
    using _Ap = remove_cvref_t<decltype(a.accessor().nested_accessor())>;
    return mdspan<typename _Ap::element_type, _Extents, _Layout, _Ap>(a.data_handle(), a.mapping(),
                                                                a.accessor().nested_accessor());
  } else if constexpr (is_arithmetic_v<_Ep> || !__ycxx::__detail::__la_adl::__has_conj<_Ep>) {
    return a;
  } else {
    using _Ap = conjugated_accessor<_Accessor>;
    return mdspan<typename _Ap::element_type, _Extents, _Layout, _Ap>(a.data_handle(), a.mapping(), _Ap(a.accessor()));
  }
}

// ---------------------------------------------------------------------------------------------
// [linalg.transp]
// ---------------------------------------------------------------------------------------------
}}} // namespace std::linalg

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// transpose-extents ([linalg.transp.helpers])
template <class _IndexType, std::size_t _E0, std::size_t _E1>
constexpr std::extents<_IndexType, _E1, _E0> __la_transpose_extents(const std::extents<_IndexType, _E0, _E1>& in) {
  return std::extents<_IndexType, _E1, _E0>(in.extent(1), in.extent(0));
}
template <class _Ep>
using __la_transpose_extents_t = decltype(::__ycxx::__detail::__la_transpose_extents(std::declval<_Ep>()));
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace linalg {

template <class _Layout>
class layout_transpose {
public:
  using nested_layout_type = _Layout;

  template <class _Extents>
  struct mapping {
    static_assert(__ycxx::__detail::__md_is_extents<_Extents>,
                  "layout_transpose: Extents must be a specialization of extents");
    static_assert(_Extents::rank() == 2, "layout_transpose: Extents::rank() must be 2");

  private:
    using __nested_mapping_type = typename _Layout::template mapping<__ycxx::__detail::__la_transpose_extents_t<_Extents>>;

  public:
    using extents_type = _Extents;
    using index_type = typename extents_type::index_type;
    using size_type = typename extents_type::size_type;
    using rank_type = typename extents_type::rank_type;
    using layout_type = layout_transpose;

    constexpr explicit mapping(const __nested_mapping_type& map)
        : __nested_mapping_(map), __extents_(__ycxx::__detail::__la_transpose_extents(map.extents())) {}

    constexpr const extents_type& extents() const noexcept { return __extents_; }
    constexpr index_type required_span_size() const { return __nested_mapping_.required_span_size(); }
    template <class _Index0, class _Index1>
    constexpr index_type operator()(_Index0 __ind0, _Index1 __ind1) const {
      return __nested_mapping_(__ind1, __ind0);
    }
    constexpr const __nested_mapping_type& nested_mapping() const noexcept { return __nested_mapping_; }
    static constexpr bool is_always_unique() noexcept { return __nested_mapping_type::is_always_unique(); }
    static constexpr bool is_always_exhaustive() noexcept { return __nested_mapping_type::is_always_exhaustive(); }
    static constexpr bool is_always_strided() noexcept { return __nested_mapping_type::is_always_strided(); }
    constexpr bool is_unique() const { return __nested_mapping_.is_unique(); }
    constexpr bool is_exhaustive() const { return __nested_mapping_.is_exhaustive(); }
    constexpr bool is_strided() const { return __nested_mapping_.is_strided(); }
    constexpr index_type stride(size_t r) const {
      __ycxx::__detail::__precondition(r < 2, "layout_transpose::mapping::stride: index out of range");
      return __nested_mapping_.stride(r == 0 ? 1 : 0);
    }
    template <class _OtherExtents>
      requires requires(const mapping& __x, const mapping<_OtherExtents>& y) {
        { __x.nested_mapping() == y.nested_mapping() } -> convertible_to<bool>;
      }
    friend constexpr bool operator==(const mapping& __x, const mapping<_OtherExtents>& y) {
      return __x.nested_mapping() == y.nested_mapping();
    }

  private:
    __nested_mapping_type __nested_mapping_;
    extents_type __extents_;
  };
};

}}} // namespace std::linalg

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __la_is_layout_transpose = false;
template <class _Lp>
inline constexpr bool __la_is_layout_transpose<std::linalg::layout_transpose<_Lp>> = true;
template <class _Tp>
inline constexpr bool __la_is_left_padded = false;
template <std::size_t _Pp>
inline constexpr bool __la_is_left_padded<std::layout_left_padded<_Pp>> = true;
template <class _Tp>
inline constexpr bool __la_is_right_padded = false;
template <std::size_t _Pp>
inline constexpr bool __la_is_right_padded<std::layout_right_padded<_Pp>> = true;

// ReturnLayout of [linalg.transp.transposed]/3.
template <class _Layout>
struct __la_transposed_layout {
  using type = std::linalg::layout_transpose<_Layout>;
};
template <>
struct __la_transposed_layout<std::layout_left> {
  using type = std::layout_right;
};
template <>
struct __la_transposed_layout<std::layout_right> {
  using type = std::layout_left;
};
template <std::size_t _Pp>
struct __la_transposed_layout<std::layout_left_padded<_Pp>> {
  using type = std::layout_right_padded<_Pp>;
};
template <std::size_t _Pp>
struct __la_transposed_layout<std::layout_right_padded<_Pp>> {
  using type = std::layout_left_padded<_Pp>;
};
template <>
struct __la_transposed_layout<std::layout_stride> {
  using type = std::layout_stride;
};
template <class _Triangle, class _StorageOrder>
struct __la_transposed_layout<std::linalg::layout_blas_packed<_Triangle, _StorageOrder>> {
  using type =
      std::linalg::layout_blas_packed<std::conditional_t<std::is_same_v<_Triangle, std::linalg::upper_triangle_t>,
                                                         std::linalg::lower_triangle_t, std::linalg::upper_triangle_t>,
                                      std::conditional_t<std::is_same_v<_StorageOrder, std::linalg::column_major_t>,
                                                         std::linalg::row_major_t, std::linalg::column_major_t>>;
};
template <class _NestedLayout>
struct __la_transposed_layout<std::linalg::layout_transpose<_NestedLayout>> {
  using type = _NestedLayout;
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace linalg {

template <class _ElementType, class _Extents, class _Layout, class _Accessor>
constexpr auto transposed(mdspan<_ElementType, _Extents, _Layout, _Accessor> a) {
  static_assert(_Extents::rank() == 2, "std::linalg::transposed: the mdspan must have rank 2");
  using _ReturnExtents = __ycxx::__detail::__la_transpose_extents_t<_Extents>;
  using _ReturnLayout = typename __ycxx::__detail::__la_transposed_layout<_Layout>::type;
  using _ReturnMapping = typename _ReturnLayout::template mapping<_ReturnExtents>;
  using _Rp = mdspan<_ElementType, _ReturnExtents, _ReturnLayout, _Accessor>;
  auto __ext = __ycxx::__detail::__la_transpose_extents(a.mapping().extents());
  if constexpr (is_same_v<_Layout, layout_left> || is_same_v<_Layout, layout_right> ||
                __ycxx::__detail::__la_is_layout_blas_packed<_Layout>)
    return _Rp(a.data_handle(), _ReturnMapping(__ext), a.accessor());
  else if constexpr (__ycxx::__detail::__la_is_left_padded<_Layout>)
    return _Rp(a.data_handle(), _ReturnMapping(__ext, a.mapping().stride(1)), a.accessor());
  else if constexpr (__ycxx::__detail::__la_is_right_padded<_Layout>)
    return _Rp(a.data_handle(), _ReturnMapping(__ext, a.mapping().stride(0)), a.accessor());
  else if constexpr (is_same_v<_Layout, layout_stride>)
    return _Rp(a.data_handle(), _ReturnMapping(__ext, array{a.mapping().stride(1), a.mapping().stride(0)}), a.accessor());
  else if constexpr (__ycxx::__detail::__la_is_layout_transpose<_Layout>)
    return _Rp(a.data_handle(), a.mapping().nested_mapping(), a.accessor());
  else
    return _Rp(a.data_handle(), _ReturnMapping(a.mapping()), a.accessor());
}

// [linalg.conjtransposed]
template <class _ElementType, class _Extents, class _Layout, class _Accessor>
constexpr auto conjugate_transposed(mdspan<_ElementType, _Extents, _Layout, _Accessor> a) {
  return std::linalg::conjugated(std::linalg::transposed(a));
}

}}} // namespace std::linalg
