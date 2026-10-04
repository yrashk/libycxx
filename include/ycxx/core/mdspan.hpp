// libycxx core: <mdspan> part 3, class template mdspan ([mdspan.mdspan]) and the
// multidimensional copy and fill algorithms ([mdspan.copy]). submdspan is in mdspan_sub.hpp.
#pragma once

#include <ycxx/core/concepts.hpp>
#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/mdspan_layout.hpp>

namespace ycxx::detail {

// Visits every multidimensional index of e in row-major order: f(i0, ..., i_{rank-1}) with
// index_type arguments.
template <class E, class F>
constexpr void md_for_each_index(const E& e, F&& f) {
  using I = typename E::index_type;
  constexpr std::size_t rank = E::rank();
  if constexpr (rank == 0) {
    f();
  } else {
    for (std::size_t r = 0; r < rank; ++r)
      if (e.extent(r) == 0)
        return;
    std::array<I, rank> idx{};
    for (;;) {
      [&]<std::size_t... K>(std::index_sequence<K...>) { f(idx[K]...); }(std::make_index_sequence<rank>());
      std::size_t r = rank;
      while (r-- > 0) {
        if (++idx[r] < e.extent(r))
          break;
        idx[r] = 0;
        if (r == 0)
          return;
      }
    }
  }
}

template <class T>
inline constexpr bool md_is_mdspan = false;

// The mappings of the standard layouts, which check their indices themselves.
template <class M>
concept md_standard_mapping =
    md_mapping_of<std::layout_left, M> || md_mapping_of<std::layout_right, M> || md_mapping_of<std::layout_stride, M> ||
    md_left_padded_mapping<M> || md_right_padded_mapping<M>;

} // namespace ycxx::detail

namespace std {

template <class ElementType, class Extents, class LayoutPolicy = layout_right,
          class AccessorPolicy = default_accessor<ElementType>>
class mdspan {
  static_assert(is_object_v<ElementType> && !is_abstract_v<ElementType> && !is_array_v<ElementType>,
                "std::mdspan: ElementType must be a complete object type, not abstract, not an array");
  static_assert(ycxx::detail::md_is_extents<Extents>, "std::mdspan: Extents must be a specialization of extents");
  static_assert(is_same_v<ElementType, typename AccessorPolicy::element_type>,
                "std::mdspan: ElementType must be AccessorPolicy::element_type");

public:
  using extents_type = Extents;
  using layout_type = LayoutPolicy;
  using accessor_type = AccessorPolicy;
  using mapping_type = typename layout_type::template mapping<extents_type>;
  using element_type = ElementType;
  using value_type = remove_cv_t<element_type>;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using data_handle_type = typename accessor_type::data_handle_type;
  using reference = typename accessor_type::reference;

  static constexpr rank_type rank() noexcept { return extents_type::rank(); }
  static constexpr rank_type rank_dynamic() noexcept { return extents_type::rank_dynamic(); }
  static constexpr size_t static_extent(rank_type r) noexcept { return extents_type::static_extent(r); }
  constexpr index_type extent(rank_type r) const noexcept { return extents().extent(r); }

  // [mdspan.mdspan.cons]
  // noexcept when the members' value-initialization is (a strengthening).
  constexpr mdspan() noexcept(is_nothrow_default_constructible_v<data_handle_type> &&
                              is_nothrow_default_constructible_v<mapping_type> &&
                              is_nothrow_default_constructible_v<accessor_type>)
    requires(rank_dynamic() > 0 && is_default_constructible_v<data_handle_type> &&
             is_default_constructible_v<mapping_type> && is_default_constructible_v<accessor_type>)
      : acc_(), map_(), ptr_() {}
  constexpr mdspan(const mdspan& rhs) = default;
  constexpr mdspan(mdspan&& rhs) = default;

  template <class... OtherIndexTypes>
    requires((is_convertible_v<OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, OtherIndexTypes> && ...) &&
             (sizeof...(OtherIndexTypes) == rank() || sizeof...(OtherIndexTypes) == rank_dynamic()) &&
             is_constructible_v<mapping_type, extents_type> && is_default_constructible_v<accessor_type>)
  constexpr explicit mdspan(data_handle_type p, OtherIndexTypes... exts)
      : acc_(), map_(extents_type(static_cast<index_type>(std::move(exts))...)), ptr_(std::move(p)) {}

  template <class OtherIndexType, size_t N>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&> && (N == rank() || N == rank_dynamic()) &&
             is_constructible_v<mapping_type, extents_type> && is_default_constructible_v<accessor_type>)
  constexpr explicit(N != rank_dynamic()) mdspan(data_handle_type p, span<OtherIndexType, N> exts)
      : acc_(), map_(extents_type(exts)), ptr_(std::move(p)) {}
  template <class OtherIndexType, size_t N>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&> && (N == rank() || N == rank_dynamic()) &&
             is_constructible_v<mapping_type, extents_type> && is_default_constructible_v<accessor_type>)
  constexpr explicit(N != rank_dynamic()) mdspan(data_handle_type p, const array<OtherIndexType, N>& exts)
      : acc_(), map_(extents_type(exts)), ptr_(std::move(p)) {}

  constexpr mdspan(data_handle_type p, const extents_type& ext)
    requires(is_constructible_v<mapping_type, const extents_type&> && is_default_constructible_v<accessor_type>)
      : acc_(), map_(ext), ptr_(std::move(p)) {}
  constexpr mdspan(data_handle_type p, const mapping_type& m)
    requires is_default_constructible_v<accessor_type>
      : acc_(), map_(m), ptr_(std::move(p)) {}
  constexpr mdspan(data_handle_type p, const mapping_type& m, const accessor_type& a)
      : acc_(a), map_(m), ptr_(std::move(p)) {}

  template <class OtherElementType, class OtherExtents, class OtherLayoutPolicy, class OtherAccessor>
    requires(is_constructible_v<mapping_type, const typename OtherLayoutPolicy::template mapping<OtherExtents>&> &&
             is_constructible_v<accessor_type, const OtherAccessor&>)
  constexpr explicit(
      !is_convertible_v<const typename OtherLayoutPolicy::template mapping<OtherExtents>&, mapping_type> ||
      !is_convertible_v<const OtherAccessor&, accessor_type>)
      mdspan(const mdspan<OtherElementType, OtherExtents, OtherLayoutPolicy, OtherAccessor>& other)
      : acc_(other.accessor()), map_(other.mapping()), ptr_(other.data_handle()) {
    static_assert(is_constructible_v<data_handle_type, const typename OtherAccessor::data_handle_type&>,
                  "std::mdspan: the data handle types are not convertible");
    static_assert(is_constructible_v<extents_type, OtherExtents>, "std::mdspan: the extents are not convertible");
    for (size_t r = 0; r < rank(); ++r)
      ycxx::detail::precondition(static_extent(r) == dynamic_extent || cmp_equal(static_extent(r), other.extent(r)),
                                 "std::mdspan: a static extent differs from the source extent");
  }

  constexpr mdspan& operator=(const mdspan& rhs) = default;
  constexpr mdspan& operator=(mdspan&& rhs) = default;

  // [mdspan.mdspan.members]
  template <class... OtherIndexTypes>
    requires((is_convertible_v<OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, OtherIndexTypes> && ...) && sizeof...(OtherIndexTypes) == rank())
  constexpr reference operator[](OtherIndexTypes... indices) const {
    if constexpr (ycxx::detail::md_standard_mapping<mapping_type>) {
      // The standard mappings check the same precondition themselves.
      return acc_.access(ptr_,
                         static_cast<size_t>(map_(ycxx::detail::md_index_cast<index_type>(std::move(indices))...)));
    } else {
      bool ok = true;
      [[maybe_unused]] size_t r = 0;
      array<index_type, rank()> idx{ycxx::detail::md_cast_index<index_type>(std::move(indices), extent(r++), ok)...};
      ycxx::detail::precondition(ok, "std::mdspan::operator[]: index out of range");
      return access(idx, make_index_sequence<rank()>());
    }
  }
  template <class OtherIndexType>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&>)
  constexpr reference operator[](span<OtherIndexType, rank()> indices) const {
    return subscript(indices, make_index_sequence<rank()>());
  }
  template <class OtherIndexType>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&>)
  constexpr reference operator[](const array<OtherIndexType, rank()>& indices) const {
    return subscript(indices, make_index_sequence<rank()>());
  }

  template <class... OtherIndexTypes>
    requires((is_convertible_v<OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, OtherIndexTypes> && ...) && sizeof...(OtherIndexTypes) == rank())
  constexpr reference at(OtherIndexTypes... indices) const {
    bool ok = true;
    [[maybe_unused]] size_t r = 0;
    array<index_type, rank()> idx{ycxx::detail::md_cast_index<index_type>(std::move(indices), extent(r++), ok)...};
    if (!ok)
      ycxx::detail::throw_out_of_range("std::mdspan::at: index out of range");
    return access(idx, make_index_sequence<rank()>());
  }
  template <class OtherIndexType>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&>)
  constexpr reference at(span<OtherIndexType, rank()> indices) const {
    return at_array(indices, make_index_sequence<rank()>());
  }
  template <class OtherIndexType>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&>)
  constexpr reference at(const array<OtherIndexType, rank()>& indices) const {
    return at_array(indices, make_index_sequence<rank()>());
  }

  constexpr size_type size() const noexcept {
    ycxx::detail::precondition(ycxx::detail::md_size_fits<size_type>(extents()),
                               "std::mdspan::size: the size is not representable as size_type");
    return static_cast<size_type>(ycxx::detail::md_fwd_prod(extents(), rank()));
  }
  constexpr bool empty() const noexcept {
    for (size_t r = 0; r < rank(); ++r)
      if (extent(r) == 0)
        return true;
    return false;
  }

  friend constexpr void swap(mdspan& x, mdspan& y) noexcept {
    ranges::swap(x.ptr_, y.ptr_);
    ranges::swap(x.map_, y.map_);
    ranges::swap(x.acc_, y.acc_);
  }

  constexpr const extents_type& extents() const noexcept { return map_.extents(); }
  constexpr const data_handle_type& data_handle() const noexcept { return ptr_; }
  constexpr const mapping_type& mapping() const noexcept { return map_; }
  constexpr const accessor_type& accessor() const noexcept { return acc_; }

  // noexcept when the mapping's function is (a strengthening, in the spirit of LWG 4021).
  static constexpr bool is_always_unique() noexcept(noexcept(mapping_type::is_always_unique())) {
    return mapping_type::is_always_unique();
  }
  static constexpr bool is_always_exhaustive() noexcept(noexcept(mapping_type::is_always_exhaustive())) {
    return mapping_type::is_always_exhaustive();
  }
  static constexpr bool is_always_strided() noexcept(noexcept(mapping_type::is_always_strided())) {
    return mapping_type::is_always_strided();
  }
  constexpr bool is_unique() const noexcept(noexcept(declval<const mapping_type&>().is_unique())) {
    return map_.is_unique();
  }
  constexpr bool is_exhaustive() const noexcept(noexcept(declval<const mapping_type&>().is_exhaustive())) {
    return map_.is_exhaustive();
  }
  constexpr bool is_strided() const noexcept(noexcept(declval<const mapping_type&>().is_strided())) {
    return map_.is_strided();
  }
  constexpr index_type stride(rank_type r) const { return map_.stride(r); }

private:
  template <size_t... P>
  constexpr reference access(const array<index_type, rank()>& idx, index_sequence<P...>) const {
    return acc_.access(ptr_, static_cast<size_t>(map_(idx[P]...)));
  }
  template <class Indices, size_t... P>
  constexpr reference subscript(const Indices& indices, index_sequence<P...>) const {
    return (*this)[ycxx::detail::md_index_cast<index_type>(as_const(indices[P]))...];
  }
  template <class Indices, size_t... P>
  constexpr reference at_array(const Indices& indices, index_sequence<P...>) const {
    return at(ycxx::detail::md_index_cast<index_type>(as_const(indices[P]))...);
  }

  [[no_unique_address]] accessor_type acc_;
  [[no_unique_address]] mapping_type map_;
  data_handle_type ptr_;
};

template <class CArray>
  requires(is_array_v<CArray> && rank_v<CArray> == 1)
mdspan(CArray&) -> mdspan<remove_all_extents_t<CArray>, extents<size_t, extent_v<CArray, 0>>>;

template <class Pointer>
  requires(is_pointer_v<remove_reference_t<Pointer>>)
mdspan(Pointer&&) -> mdspan<remove_pointer_t<remove_reference_t<Pointer>>, extents<size_t>>;

template <class ElementType, class... Integrals>
  requires((is_convertible_v<Integrals, size_t> && ...) && sizeof...(Integrals) > 0)
explicit mdspan(ElementType*,
                Integrals...) -> mdspan<ElementType, extents<size_t, ycxx::detail::maybe_static_ext<Integrals>...>>;

template <class ElementType, class OtherIndexType, size_t N>
mdspan(ElementType*, span<OtherIndexType, N>) -> mdspan<ElementType, dextents<size_t, N>>;

template <class ElementType, class OtherIndexType, size_t N>
mdspan(ElementType*, const array<OtherIndexType, N>&) -> mdspan<ElementType, dextents<size_t, N>>;

template <class ElementType, class IndexType, size_t... ExtentsPack>
mdspan(ElementType*,
       const extents<IndexType, ExtentsPack...>&) -> mdspan<ElementType, extents<IndexType, ExtentsPack...>>;

template <class ElementType, class MappingType>
mdspan(ElementType*, const MappingType&)
    -> mdspan<ElementType, typename MappingType::extents_type, typename MappingType::layout_type>;

template <class MappingType, class AccessorType>
mdspan(typename AccessorType::data_handle_type, const MappingType&,
       const AccessorType&) -> mdspan<typename AccessorType::element_type, typename MappingType::extents_type,
                                      typename MappingType::layout_type, AccessorType>;

} // namespace std

namespace ycxx::detail {

template <class E, class X, class L, class A>
inline constexpr bool md_is_mdspan<std::mdspan<E, X, L, A>> = true;

// The element of m at the index i... (no bounds check: the indices come from m's extents).
template <class M, class... I>
constexpr typename M::reference md_elem(const M& m, I... i) {
  return m.accessor().access(m.data_handle(), static_cast<std::size_t>(m.mapping()(i...)));
}

template <class Src, class Dst>
concept md_copyable =
    md_is_mdspan<Src> && md_is_mdspan<Dst> && std::is_assignable_v<typename Dst::reference, typename Src::reference> &&
    std::is_constructible_v<typename Src::extents_type, typename Dst::extents_type>;
template <class Dst, class T>
concept md_fillable = md_is_mdspan<Dst> && std::is_assignable_v<typename Dst::reference, const T&>;

} // namespace ycxx::detail

namespace std {

// [mdspan.copy]
template <class Src, class Dst>
  requires ycxx::detail::md_copyable<Src, Dst>
constexpr void copy(const Src& src, const Dst& dst) {
  ycxx::detail::precondition(src.extents() == dst.extents(), "std::copy(mdspan): the extents differ");
  ycxx::detail::md_for_each_index(
      src.extents(), [&](auto... i) { ycxx::detail::md_elem(dst, i...) = ycxx::detail::md_elem(src, i...); });
}
template <class ExecutionPolicy, class Src, class Dst>
  requires(ycxx::detail::execution_policy<ExecutionPolicy> && ycxx::detail::md_copyable<Src, Dst>)
void copy(ExecutionPolicy&&, const Src& src, const Dst& dst) noexcept {
  std::copy(src, dst);
}

template <class Dst, class T = typename Dst::value_type>
  requires ycxx::detail::md_fillable<Dst, T>
constexpr void fill(const Dst& dst, const T& value) {
  ycxx::detail::md_for_each_index(dst.extents(), [&](auto... i) { ycxx::detail::md_elem(dst, i...) = value; });
}
template <class ExecutionPolicy, class Dst, class T = typename Dst::value_type>
  requires(ycxx::detail::execution_policy<ExecutionPolicy> && ycxx::detail::md_fillable<Dst, T>)
void fill(ExecutionPolicy&&, const Dst& dst, const T& value) noexcept {
  std::fill(dst, value);
}

} // namespace std

#include <ycxx/core/mdspan_sub.hpp>
