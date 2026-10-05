// libycxx core: <linalg> part 1 ([linalg.tags] through [linalg.conjtransposed]): the tag
// classes, layout_blas_packed, the exposition-only helpers and argument concepts, and the
// in-place transformations scaled, conjugated, transposed and conjugate_transposed.
#pragma once

#include <ycxx/core/complex.hpp>
#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/mdspan.hpp>

namespace [[gnu::visibility("hidden")]] std { namespace linalg {

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
template <class Triangle, class StorageOrder>
class layout_blas_packed {
public:
  using triangle_type = Triangle;
  using storage_order_type = StorageOrder;

  template <class Extents>
  struct mapping {
    static_assert(is_same_v<Triangle, upper_triangle_t> || is_same_v<Triangle, lower_triangle_t>,
                  "layout_blas_packed: Triangle must be upper_triangle_t or lower_triangle_t");
    static_assert(is_same_v<StorageOrder, column_major_t> || is_same_v<StorageOrder, row_major_t>,
                  "layout_blas_packed: StorageOrder must be column_major_t or row_major_t");
    static_assert(ycxx::detail::md_is_extents<Extents>,
                  "layout_blas_packed: Extents must be a specialization of extents");
    static_assert(Extents::rank() == 2, "layout_blas_packed: Extents::rank() must be 2");

  public:
    using extents_type = Extents;
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
    static constexpr bool packed_fits(size_t n) noexcept {
      index_type p = 0;
      return in_range<index_type>(n) &&
             ycxx::detail::md_mul(static_cast<index_type>(n), static_cast<index_type>(n + 1), p);
    }
    static consteval bool static_fits() {
      if constexpr (extents_type::rank_dynamic() != 0)
        return true;
      else
        return packed_fits(extents_type::static_extent(0));
    }
    static_assert(static_fits(), "layout_blas_packed: Ns * (Ns + 1) must be representable as index_type");

  public:
    // [linalg.layout.packed.cons]
    constexpr mapping() noexcept = default;
    constexpr mapping(const mapping&) noexcept = default;
    constexpr mapping(const extents_type& e) noexcept : extents_(e) {
      ycxx::detail::precondition(e.extent(0) == e.extent(1), "layout_blas_packed: the matrix must be square");
      ycxx::detail::precondition(packed_fits(static_cast<size_t>(e.extent(0))),
                                 "layout_blas_packed: N * (N + 1) is not representable as index_type");
    }
    template <class OtherExtents>
      requires is_constructible_v<extents_type, OtherExtents>
    constexpr explicit(!is_convertible_v<OtherExtents, extents_type>)
        mapping(const mapping<OtherExtents>& other) noexcept
        : extents_(other.extents()) {
      ycxx::detail::precondition(packed_fits(static_cast<size_t>(other.extents().extent(0))),
                                 "layout_blas_packed: N * (N + 1) is not representable as index_type");
    }
    constexpr mapping& operator=(const mapping&) noexcept = default;

    // [linalg.layout.packed.obs]
    constexpr const extents_type& extents() const noexcept { return extents_; }
    constexpr index_type required_span_size() const noexcept {
      return static_cast<index_type>(extents_.extent(0) * (extents_.extent(0) + 1) / 2);
    }
    template <class Index0, class Index1>
      requires(is_convertible_v<Index0, index_type> && is_convertible_v<Index1, index_type> &&
               is_nothrow_constructible_v<index_type, Index0> && is_nothrow_constructible_v<index_type, Index1>)
    constexpr index_type operator()(Index0 ind0, Index1 ind1) const noexcept {
      auto idx = ycxx::detail::md_indices(extents_, std::move(ind0), std::move(ind1));
      index_type i = idx[0], j = idx[1];
      if (i > j) {
        index_type t = i;
        i = j;
        j = t;
      }
      if constexpr ((is_same_v<StorageOrder, column_major_t> && is_same_v<Triangle, upper_triangle_t>) ||
                    (is_same_v<StorageOrder, row_major_t> && is_same_v<Triangle, lower_triangle_t>))
        return static_cast<index_type>(i + j * (j + 1) / 2);
      else
        return static_cast<index_type>(j + extents_.extent(0) * i - i * (i + 1) / 2);
    }
    static constexpr bool is_always_unique() noexcept {
      return (extents_type::static_extent(0) != dynamic_extent && extents_type::static_extent(0) < 2) ||
             (extents_type::static_extent(1) != dynamic_extent && extents_type::static_extent(1) < 2);
    }
    static constexpr bool is_always_exhaustive() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return is_always_unique(); }
    constexpr bool is_unique() const noexcept { return extents_.extent(0) < 2; }
    constexpr bool is_exhaustive() const noexcept { return true; }
    constexpr bool is_strided() const noexcept { return extents_.extent(0) < 2; }
    constexpr index_type stride(rank_type r) const noexcept {
      ycxx::detail::precondition(is_strided() && r < extents_type::rank(),
                                 "layout_blas_packed::mapping::stride: the mapping is not strided");
      return 1;
    }
    template <class OtherExtents>
    friend constexpr bool operator==(const mapping& x, const mapping<OtherExtents>& y) noexcept {
      return x.extents() == y.extents();
    }

  private:
    extents_type extents_{};
  };
};

}} // namespace std::linalg

// ---------------------------------------------------------------------------------------------
// [linalg.helpers]: abs-if-needed, conj-if-needed, real-if-needed, imag-if-needed. The
// unqualified calls are made here, next to deleted templates that hide nothing but stop the
// lookup from reaching std:: functions for arithmetic arguments ([linalg.general]/6).
// ---------------------------------------------------------------------------------------------
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::la_adl {

template <class U>
U abs(U) = delete;
template <class U>
U conj(const U&) = delete;
template <class U>
U real(const U&) = delete;
template <class U>
U imag(const U&) = delete;

template <class T>
concept has_abs = requires(T t) { abs(t); };
template <class T>
concept has_conj = requires(const T& t) { conj(t); };
template <class T>
concept has_real = requires(const T& t) { real(t); };
template <class T>
concept has_imag = requires(const T& t) { imag(t); };

template <class T>
constexpr auto abs_if_needed(const T& e) {
  if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
    return e;
  else if constexpr (std::is_arithmetic_v<T>)
    return std::abs(e);
  else
    return abs(e);
}
template <class T>
constexpr auto conj_if_needed(const T& e) {
  if constexpr (!std::is_arithmetic_v<T> && has_conj<T>)
    return conj(e);
  else
    return e;
}
template <class T>
constexpr auto real_if_needed(const T& e) {
  if constexpr (!std::is_arithmetic_v<T> && has_real<T>)
    return real(e);
  else
    return e;
}
template <class T>
constexpr auto imag_if_needed(const T& e) {
  if constexpr (!std::is_arithmetic_v<T> && has_imag<T>)
    return imag(e);
  else
    return ((void)e, T{});
}

}} // namespace ycxx::detail::la_adl

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// [linalg.helpers.concepts]
template <class T>
concept la_in_vector = md_is_mdspan<T> && T::rank() == 1;
template <class T>
concept la_out_vector = md_is_mdspan<T> && T::rank() == 1 &&
                        std::is_assignable_v<typename T::reference, typename T::element_type> && T::is_always_unique();
template <class T>
concept la_inout_vector = la_out_vector<T>;
template <class T>
concept la_in_matrix = md_is_mdspan<T> && T::rank() == 2;
template <class T>
concept la_out_matrix = md_is_mdspan<T> && T::rank() == 2 &&
                        std::is_assignable_v<typename T::reference, typename T::element_type> && T::is_always_unique();
template <class T>
concept la_inout_matrix = la_out_matrix<T>;
template <class T>
inline constexpr bool la_is_layout_blas_packed = false;
template <class Triangle, class StorageOrder>
inline constexpr bool la_is_layout_blas_packed<std::linalg::layout_blas_packed<Triangle, StorageOrder>> = true;
template <class T>
concept la_possibly_packed_out_matrix =
    md_is_mdspan<T> && T::rank() == 2 && std::is_assignable_v<typename T::reference, typename T::element_type> &&
    (T::is_always_unique() || la_is_layout_blas_packed<typename T::layout_type>);
template <class T>
concept la_in_object = md_is_mdspan<T> && (T::rank() == 1 || T::rank() == 2);
template <class T>
concept la_out_object = md_is_mdspan<T> && (T::rank() == 1 || T::rank() == 2) &&
                        std::is_assignable_v<typename T::reference, typename T::element_type> && T::is_always_unique();
template <class T>
concept la_inout_object = la_out_object<T>;
template <class T>
concept la_scalar = std::semiregular<T> && !md_is_mdspan<T> && !std::is_execution_policy_v<T>;

// The template parameter name constraints of [linalg.algs.reqs].
template <class T>
concept la_triangle =
    std::is_same_v<T, std::linalg::upper_triangle_t> || std::is_same_v<T, std::linalg::lower_triangle_t>;
template <class T>
concept la_diagonal =
    std::is_same_v<T, std::linalg::implicit_unit_diagonal_t> || std::is_same_v<T, std::linalg::explicit_diagonal_t>;
template <class T>
concept la_real = std::is_floating_point_v<T> && std::is_same_v<T, std::remove_cv_t<T>>;

// [linalg.helpers.mandates]
template <class MDS1, class MDS2>
consteval bool la_compatible(std::size_t r1, std::size_t r2) {
  return MDS1::static_extent(r1) == std::dynamic_extent || MDS2::static_extent(r2) == std::dynamic_extent ||
         MDS1::static_extent(r1) == MDS2::static_extent(r2);
}
template <class In1, class In2, class Out>
consteval bool la_possibly_addable() {
  if constexpr (In1::rank() == 1)
    return la_compatible<Out, In1>(0, 0) && la_compatible<Out, In2>(0, 0) && la_compatible<In1, In2>(0, 0);
  else
    return la_compatible<Out, In1>(0, 0) && la_compatible<Out, In1>(1, 1) && la_compatible<Out, In2>(0, 0) &&
           la_compatible<Out, In2>(1, 1) && la_compatible<In1, In2>(0, 0) && la_compatible<In1, In2>(1, 1);
}
template <class A, class B, class Out>
consteval bool la_possibly_multipliable() {
  if constexpr (A::rank() == 2 && B::rank() == 1)
    return la_compatible<Out, A>(0, 0) && la_compatible<A, B>(1, 0);
  else if constexpr (A::rank() == 1)
    return la_compatible<Out, B>(0, 1) && la_compatible<B, A>(0, 0);
  else
    return la_compatible<Out, A>(0, 0) && la_compatible<Out, B>(1, 1) && la_compatible<A, B>(1, 0);
}

// [linalg.helpers.precond]
template <class In1, class In2, class Out>
constexpr bool la_addable(const In1& in1, const In2& in2, const Out& out) {
  for (std::size_t r = 0; r < Out::rank(); ++r)
    if (!std::cmp_equal(out.extent(r), in1.extent(r)) || !std::cmp_equal(out.extent(r), in2.extent(r)))
      return false;
  return true;
}
template <class A, class B, class Out>
constexpr bool la_multipliable(const A& a, const B& b, const Out& out) {
  if constexpr (A::rank() == 2 && B::rank() == 1)
    return std::cmp_equal(out.extent(0), a.extent(0)) && std::cmp_equal(a.extent(1), b.extent(0));
  else if constexpr (A::rank() == 1)
    return std::cmp_equal(out.extent(0), b.extent(1)) && std::cmp_equal(b.extent(0), a.extent(0));
  else
    return std::cmp_equal(out.extent(0), a.extent(0)) && std::cmp_equal(out.extent(1), b.extent(1)) &&
           std::cmp_equal(a.extent(1), b.extent(0));
}

// The Triangle of a layout_blas_packed matrix, or void.
template <class Layout>
struct la_packed_triangle {
  using type = void;
};
template <class T, class S>
struct la_packed_triangle<std::linalg::layout_blas_packed<T, S>> {
  using type = T;
};
// "If M has layout_blas_packed layout, then the layout's Triangle template argument has the
// same type as the function's Triangle template argument."
template <class M, class Triangle>
inline constexpr bool la_triangle_matches =
    !la_is_layout_blas_packed<typename M::layout_type> ||
    std::is_same_v<typename la_packed_triangle<typename M::layout_type>::type, Triangle>;

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace linalg {

// ---------------------------------------------------------------------------------------------
// [linalg.scaled]
// ---------------------------------------------------------------------------------------------
template <class ScalingFactor, class NestedAccessor>
class scaled_accessor {
public:
  using element_type = const decltype(declval<ScalingFactor>() * declval<typename NestedAccessor::element_type>());
  using reference = remove_const_t<element_type>;
  using data_handle_type = typename NestedAccessor::data_handle_type;
  using offset_policy = scaled_accessor<ScalingFactor, typename NestedAccessor::offset_policy>;

  static_assert(is_copy_constructible_v<reference>, "scaled_accessor: reference must be copy constructible");
  static_assert(!is_reference_v<element_type>, "scaled_accessor: element_type must not be a reference");
  static_assert(semiregular<ScalingFactor>, "scaled_accessor: ScalingFactor must model semiregular");

  constexpr scaled_accessor() = default;
  template <class OtherNestedAccessor>
    requires is_constructible_v<NestedAccessor, const OtherNestedAccessor&>
  constexpr explicit(!is_convertible_v<OtherNestedAccessor, NestedAccessor>)
      scaled_accessor(const scaled_accessor<ScalingFactor, OtherNestedAccessor>& other)
      : scaling_factor_(other.scaling_factor()), nested_accessor_(other.nested_accessor()) {}
  constexpr scaled_accessor(const ScalingFactor& s, const NestedAccessor& a)
      : scaling_factor_(s), nested_accessor_(a) {}

  constexpr reference access(data_handle_type p, size_t i) const {
    return scaling_factor() * typename NestedAccessor::element_type(nested_accessor_.access(p, i));
  }
  constexpr typename offset_policy::data_handle_type offset(data_handle_type p, size_t i) const {
    return nested_accessor_.offset(p, i);
  }
  constexpr const ScalingFactor& scaling_factor() const noexcept { return scaling_factor_; }
  constexpr const NestedAccessor& nested_accessor() const noexcept { return nested_accessor_; }

private:
  ScalingFactor scaling_factor_{};
  NestedAccessor nested_accessor_{};
};

template <class ScalingFactor, class ElementType, class Extents, class Layout, class Accessor>
constexpr auto scaled(ScalingFactor alpha, mdspan<ElementType, Extents, Layout, Accessor> x) {
  using SA = scaled_accessor<ScalingFactor, Accessor>;
  return mdspan<typename SA::element_type, Extents, Layout, SA>(x.data_handle(), x.mapping(), SA(alpha, x.accessor()));
}

// ---------------------------------------------------------------------------------------------
// [linalg.conj]
// ---------------------------------------------------------------------------------------------
template <class NestedAccessor>
class conjugated_accessor {
public:
  using element_type =
      const decltype(ycxx::detail::la_adl::conj_if_needed(declval<typename NestedAccessor::element_type>()));
  using reference = remove_const_t<element_type>;
  using data_handle_type = typename NestedAccessor::data_handle_type;
  using offset_policy = conjugated_accessor<typename NestedAccessor::offset_policy>;

  static_assert(is_copy_constructible_v<reference>, "conjugated_accessor: reference must be copy constructible");
  static_assert(!is_reference_v<element_type>, "conjugated_accessor: element_type must not be a reference");

  constexpr conjugated_accessor() = default;
  constexpr conjugated_accessor(const NestedAccessor& acc) : nested_accessor_(acc) {}
  template <class OtherNestedAccessor>
    requires is_constructible_v<NestedAccessor, const OtherNestedAccessor&>
  constexpr explicit(!is_convertible_v<OtherNestedAccessor, NestedAccessor>)
      conjugated_accessor(const conjugated_accessor<OtherNestedAccessor>& other)
      : nested_accessor_(other.nested_accessor()) {}

  constexpr reference access(data_handle_type p, size_t i) const {
    return ycxx::detail::la_adl::conj_if_needed(typename NestedAccessor::element_type(nested_accessor_.access(p, i)));
  }
  constexpr typename offset_policy::data_handle_type offset(data_handle_type p, size_t i) const {
    return nested_accessor_.offset(p, i);
  }
  constexpr const NestedAccessor& nested_accessor() const noexcept { return nested_accessor_; }

private:
  NestedAccessor nested_accessor_{};
};

}} // namespace std::linalg

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class T>
inline constexpr bool la_is_conjugated_accessor = false;
template <class A>
inline constexpr bool la_is_conjugated_accessor<std::linalg::conjugated_accessor<A>> = true;
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace linalg {

template <class ElementType, class Extents, class Layout, class Accessor>
constexpr auto conjugated(mdspan<ElementType, Extents, Layout, Accessor> a) {
  using E = remove_cvref_t<ElementType>;
  if constexpr (ycxx::detail::la_is_conjugated_accessor<Accessor>) {
    using A = remove_cvref_t<decltype(a.accessor().nested_accessor())>;
    return mdspan<typename A::element_type, Extents, Layout, A>(a.data_handle(), a.mapping(),
                                                                a.accessor().nested_accessor());
  } else if constexpr (is_arithmetic_v<E> || !ycxx::detail::la_adl::has_conj<E>) {
    return a;
  } else {
    using A = conjugated_accessor<Accessor>;
    return mdspan<typename A::element_type, Extents, Layout, A>(a.data_handle(), a.mapping(), A(a.accessor()));
  }
}

// ---------------------------------------------------------------------------------------------
// [linalg.transp]
// ---------------------------------------------------------------------------------------------
}} // namespace std::linalg

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// transpose-extents ([linalg.transp.helpers])
template <class IndexType, std::size_t E0, std::size_t E1>
constexpr std::extents<IndexType, E1, E0> la_transpose_extents(const std::extents<IndexType, E0, E1>& in) {
  return std::extents<IndexType, E1, E0>(in.extent(1), in.extent(0));
}
template <class E>
using la_transpose_extents_t = decltype(::ycxx::detail::la_transpose_extents(std::declval<E>()));
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace linalg {

template <class Layout>
class layout_transpose {
public:
  using nested_layout_type = Layout;

  template <class Extents>
  struct mapping {
    static_assert(ycxx::detail::md_is_extents<Extents>,
                  "layout_transpose: Extents must be a specialization of extents");
    static_assert(Extents::rank() == 2, "layout_transpose: Extents::rank() must be 2");

  private:
    using nested_mapping_type = typename Layout::template mapping<ycxx::detail::la_transpose_extents_t<Extents>>;

  public:
    using extents_type = Extents;
    using index_type = typename extents_type::index_type;
    using size_type = typename extents_type::size_type;
    using rank_type = typename extents_type::rank_type;
    using layout_type = layout_transpose;

    constexpr explicit mapping(const nested_mapping_type& map)
        : nested_mapping_(map), extents_(ycxx::detail::la_transpose_extents(map.extents())) {}

    constexpr const extents_type& extents() const noexcept { return extents_; }
    constexpr index_type required_span_size() const { return nested_mapping_.required_span_size(); }
    template <class Index0, class Index1>
    constexpr index_type operator()(Index0 ind0, Index1 ind1) const {
      return nested_mapping_(ind1, ind0);
    }
    constexpr const nested_mapping_type& nested_mapping() const noexcept { return nested_mapping_; }
    static constexpr bool is_always_unique() noexcept { return nested_mapping_type::is_always_unique(); }
    static constexpr bool is_always_exhaustive() noexcept { return nested_mapping_type::is_always_exhaustive(); }
    static constexpr bool is_always_strided() noexcept { return nested_mapping_type::is_always_strided(); }
    constexpr bool is_unique() const { return nested_mapping_.is_unique(); }
    constexpr bool is_exhaustive() const { return nested_mapping_.is_exhaustive(); }
    constexpr bool is_strided() const { return nested_mapping_.is_strided(); }
    constexpr index_type stride(size_t r) const {
      ycxx::detail::precondition(r < 2, "layout_transpose::mapping::stride: index out of range");
      return nested_mapping_.stride(r == 0 ? 1 : 0);
    }
    template <class OtherExtents>
      requires requires(const mapping& x, const mapping<OtherExtents>& y) {
        { x.nested_mapping() == y.nested_mapping() } -> convertible_to<bool>;
      }
    friend constexpr bool operator==(const mapping& x, const mapping<OtherExtents>& y) {
      return x.nested_mapping() == y.nested_mapping();
    }

  private:
    nested_mapping_type nested_mapping_;
    extents_type extents_;
  };
};

}} // namespace std::linalg

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class T>
inline constexpr bool la_is_layout_transpose = false;
template <class L>
inline constexpr bool la_is_layout_transpose<std::linalg::layout_transpose<L>> = true;
template <class T>
inline constexpr bool la_is_left_padded = false;
template <std::size_t P>
inline constexpr bool la_is_left_padded<std::layout_left_padded<P>> = true;
template <class T>
inline constexpr bool la_is_right_padded = false;
template <std::size_t P>
inline constexpr bool la_is_right_padded<std::layout_right_padded<P>> = true;

// ReturnLayout of [linalg.transp.transposed]/3.
template <class Layout>
struct la_transposed_layout {
  using type = std::linalg::layout_transpose<Layout>;
};
template <>
struct la_transposed_layout<std::layout_left> {
  using type = std::layout_right;
};
template <>
struct la_transposed_layout<std::layout_right> {
  using type = std::layout_left;
};
template <std::size_t P>
struct la_transposed_layout<std::layout_left_padded<P>> {
  using type = std::layout_right_padded<P>;
};
template <std::size_t P>
struct la_transposed_layout<std::layout_right_padded<P>> {
  using type = std::layout_left_padded<P>;
};
template <>
struct la_transposed_layout<std::layout_stride> {
  using type = std::layout_stride;
};
template <class Triangle, class StorageOrder>
struct la_transposed_layout<std::linalg::layout_blas_packed<Triangle, StorageOrder>> {
  using type =
      std::linalg::layout_blas_packed<std::conditional_t<std::is_same_v<Triangle, std::linalg::upper_triangle_t>,
                                                         std::linalg::lower_triangle_t, std::linalg::upper_triangle_t>,
                                      std::conditional_t<std::is_same_v<StorageOrder, std::linalg::column_major_t>,
                                                         std::linalg::row_major_t, std::linalg::column_major_t>>;
};
template <class NestedLayout>
struct la_transposed_layout<std::linalg::layout_transpose<NestedLayout>> {
  using type = NestedLayout;
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace linalg {

template <class ElementType, class Extents, class Layout, class Accessor>
constexpr auto transposed(mdspan<ElementType, Extents, Layout, Accessor> a) {
  static_assert(Extents::rank() == 2, "std::linalg::transposed: the mdspan must have rank 2");
  using ReturnExtents = ycxx::detail::la_transpose_extents_t<Extents>;
  using ReturnLayout = typename ycxx::detail::la_transposed_layout<Layout>::type;
  using ReturnMapping = typename ReturnLayout::template mapping<ReturnExtents>;
  using R = mdspan<ElementType, ReturnExtents, ReturnLayout, Accessor>;
  auto ext = ycxx::detail::la_transpose_extents(a.mapping().extents());
  if constexpr (is_same_v<Layout, layout_left> || is_same_v<Layout, layout_right> ||
                ycxx::detail::la_is_layout_blas_packed<Layout>)
    return R(a.data_handle(), ReturnMapping(ext), a.accessor());
  else if constexpr (ycxx::detail::la_is_left_padded<Layout>)
    return R(a.data_handle(), ReturnMapping(ext, a.mapping().stride(1)), a.accessor());
  else if constexpr (ycxx::detail::la_is_right_padded<Layout>)
    return R(a.data_handle(), ReturnMapping(ext, a.mapping().stride(0)), a.accessor());
  else if constexpr (is_same_v<Layout, layout_stride>)
    return R(a.data_handle(), ReturnMapping(ext, array{a.mapping().stride(1), a.mapping().stride(0)}), a.accessor());
  else if constexpr (ycxx::detail::la_is_layout_transpose<Layout>)
    return R(a.data_handle(), a.mapping().nested_mapping(), a.accessor());
  else
    return R(a.data_handle(), ReturnMapping(a.mapping()), a.accessor());
}

// [linalg.conjtransposed]
template <class ElementType, class Extents, class Layout, class Accessor>
constexpr auto conjugate_transposed(mdspan<ElementType, Extents, Layout, Accessor> a) {
  return std::linalg::conjugated(std::linalg::transposed(a));
}

}} // namespace std::linalg
