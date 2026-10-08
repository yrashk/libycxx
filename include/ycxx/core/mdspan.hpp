// libycxx core: <mdspan> part 3, class template mdspan ([mdspan.mdspan]) and the
// multidimensional copy and fill algorithms ([mdspan.copy]). submdspan is in mdspan_sub.hpp.
#pragma once

#include <ycxx/core/concepts.hpp>
#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/mdspan_layout.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// Visits every multidimensional index of e in row-major order: f(i0, ..., i_{rank-1}) with
// index_type arguments.
template <class _Ep, class _Fp>
constexpr void __md_for_each_index(const _Ep& e, _Fp&& __f) {
  using _Ip = typename _Ep::index_type;
  constexpr std::size_t rank = _Ep::rank();
  if constexpr (rank == 0) {
    __f();
  } else {
    for (std::size_t r = 0; r < rank; ++r)
      if (e.extent(r) == 0)
        return;
    std::array<_Ip, rank> __idx{};
    for (;;) {
      [&]<std::size_t... _Kp>(std::index_sequence<_Kp...>) { __f(__idx[_Kp]...); }(std::make_index_sequence<rank>());
      std::size_t r = rank;
      while (r-- > 0) {
        if (++__idx[r] < e.extent(r))
          break;
        __idx[r] = 0;
        if (r == 0)
          return;
      }
    }
  }
}

template <class _Tp>
inline constexpr bool __md_is_mdspan = false;

// The mappings of the standard layouts, which check their indices themselves.
template <class _Mp>
concept __md_standard_mapping =
    __md_mapping_of<std::layout_left, _Mp> || __md_mapping_of<std::layout_right, _Mp> || __md_mapping_of<std::layout_stride, _Mp> ||
    __md_left_padded_mapping<_Mp> || __md_right_padded_mapping<_Mp>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _ElementType, class _Extents, class _LayoutPolicy = layout_right,
          class _AccessorPolicy = default_accessor<_ElementType>>
class mdspan {
  static_assert(is_object_v<_ElementType> && !is_abstract_v<_ElementType> && !is_array_v<_ElementType>,
                "std::mdspan: ElementType must be a complete object type, not abstract, not an array");
  static_assert(__ycxx::__detail::__md_is_extents<_Extents>, "std::mdspan: Extents must be a specialization of extents");
  static_assert(is_same_v<_ElementType, typename _AccessorPolicy::element_type>,
                "std::mdspan: ElementType must be AccessorPolicy::element_type");

public:
  using extents_type = _Extents;
  using layout_type = _LayoutPolicy;
  using accessor_type = _AccessorPolicy;
  using mapping_type = typename layout_type::template mapping<extents_type>;
  using element_type = _ElementType;
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
      : __acc_(), __map_(), __ptr_() {}
  constexpr mdspan(const mdspan& __rhs) = default;
  constexpr mdspan(mdspan&& __rhs) = default;

  template <class... _OtherIndexTypes>
    requires((is_convertible_v<_OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _OtherIndexTypes> && ...) &&
             (sizeof...(_OtherIndexTypes) == rank() || sizeof...(_OtherIndexTypes) == rank_dynamic()) &&
             is_constructible_v<mapping_type, extents_type> && is_default_constructible_v<accessor_type>)
  constexpr explicit mdspan(data_handle_type p, _OtherIndexTypes... __exts)
      : __acc_(), __map_(extents_type(static_cast<index_type>(std::move(__exts))...)), __ptr_(std::move(p)) {}

  template <class _OtherIndexType, size_t _Np>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&> && (_Np == rank() || _Np == rank_dynamic()) &&
             is_constructible_v<mapping_type, extents_type> && is_default_constructible_v<accessor_type>)
  constexpr explicit(_Np != rank_dynamic()) mdspan(data_handle_type p, span<_OtherIndexType, _Np> __exts)
      : __acc_(), __map_(extents_type(__exts)), __ptr_(std::move(p)) {}
  template <class _OtherIndexType, size_t _Np>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&> && (_Np == rank() || _Np == rank_dynamic()) &&
             is_constructible_v<mapping_type, extents_type> && is_default_constructible_v<accessor_type>)
  constexpr explicit(_Np != rank_dynamic()) mdspan(data_handle_type p, const array<_OtherIndexType, _Np>& __exts)
      : __acc_(), __map_(extents_type(__exts)), __ptr_(std::move(p)) {}

  constexpr mdspan(data_handle_type p, const extents_type& __ext)
    requires(is_constructible_v<mapping_type, const extents_type&> && is_default_constructible_v<accessor_type>)
      : __acc_(), __map_(__ext), __ptr_(std::move(p)) {}
  constexpr mdspan(data_handle_type p, const mapping_type& m)
    requires is_default_constructible_v<accessor_type>
      : __acc_(), __map_(m), __ptr_(std::move(p)) {}
  constexpr mdspan(data_handle_type p, const mapping_type& m, const accessor_type& a)
      : __acc_(a), __map_(m), __ptr_(std::move(p)) {}

  template <class _OtherElementType, class _OtherExtents, class _OtherLayoutPolicy, class _OtherAccessor>
    requires(is_constructible_v<mapping_type, const typename _OtherLayoutPolicy::template mapping<_OtherExtents>&> &&
             is_constructible_v<accessor_type, const _OtherAccessor&>)
  constexpr explicit(
      !is_convertible_v<const typename _OtherLayoutPolicy::template mapping<_OtherExtents>&, mapping_type> ||
      !is_convertible_v<const _OtherAccessor&, accessor_type>)
      mdspan(const mdspan<_OtherElementType, _OtherExtents, _OtherLayoutPolicy, _OtherAccessor>& other)
      : __acc_(other.accessor()), __map_(other.mapping()), __ptr_(other.data_handle()) {
    static_assert(is_constructible_v<data_handle_type, const typename _OtherAccessor::data_handle_type&>,
                  "std::mdspan: the data handle types are not convertible");
    static_assert(is_constructible_v<extents_type, _OtherExtents>, "std::mdspan: the extents are not convertible");
    for (size_t r = 0; r < rank(); ++r)
      __ycxx::__detail::__precondition(static_extent(r) == dynamic_extent || cmp_equal(static_extent(r), other.extent(r)),
                                 "std::mdspan: a static extent differs from the source extent");
  }

  constexpr mdspan& operator=(const mdspan& __rhs) = default;
  constexpr mdspan& operator=(mdspan&& __rhs) = default;

  // [mdspan.mdspan.members]
  template <class... _OtherIndexTypes>
    requires((is_convertible_v<_OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _OtherIndexTypes> && ...) && sizeof...(_OtherIndexTypes) == rank())
  constexpr reference operator[](_OtherIndexTypes... indices) const {
    if constexpr (__ycxx::__detail::__md_standard_mapping<mapping_type>) {
      // The standard mappings check the same precondition themselves.
      return __acc_.access(__ptr_,
                         static_cast<size_t>(__map_(__ycxx::__detail::__md_index_cast<index_type>(std::move(indices))...)));
    } else {
      bool ok = true;
      [[maybe_unused]] size_t r = 0;
      array<index_type, rank()> __idx{__ycxx::__detail::__md_cast_index<index_type>(std::move(indices), extent(r++), ok)...};
      __ycxx::__detail::__precondition(ok, "std::mdspan::operator[]: index out of range");
      return access(__idx, make_index_sequence<rank()>());
    }
  }
  template <class _OtherIndexType>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&>)
  constexpr reference operator[](span<_OtherIndexType, rank()> indices) const {
    return __subscript(indices, make_index_sequence<rank()>());
  }
  template <class _OtherIndexType>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&>)
  constexpr reference operator[](const array<_OtherIndexType, rank()>& indices) const {
    return __subscript(indices, make_index_sequence<rank()>());
  }

  template <class... _OtherIndexTypes>
    requires((is_convertible_v<_OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _OtherIndexTypes> && ...) && sizeof...(_OtherIndexTypes) == rank())
  constexpr reference at(_OtherIndexTypes... indices) const {
    bool ok = true;
    [[maybe_unused]] size_t r = 0;
    array<index_type, rank()> __idx{__ycxx::__detail::__md_cast_index<index_type>(std::move(indices), extent(r++), ok)...};
    if (!ok)
      __ycxx::__detail::__throw_out_of_range("std::mdspan::at: index out of range");
    return access(__idx, make_index_sequence<rank()>());
  }
  template <class _OtherIndexType>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&>)
  constexpr reference at(span<_OtherIndexType, rank()> indices) const {
    return __at_array(indices, make_index_sequence<rank()>());
  }
  template <class _OtherIndexType>
    requires(is_convertible_v<const _OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const _OtherIndexType&>)
  constexpr reference at(const array<_OtherIndexType, rank()>& indices) const {
    return __at_array(indices, make_index_sequence<rank()>());
  }

  constexpr size_type size() const noexcept {
    __ycxx::__detail::__precondition(__ycxx::__detail::__md_size_fits<size_type>(extents()),
                               "std::mdspan::size: the size is not representable as size_type");
    return static_cast<size_type>(__ycxx::__detail::__md_fwd_prod(extents(), rank()));
  }
  constexpr bool empty() const noexcept {
    for (size_t r = 0; r < rank(); ++r)
      if (extent(r) == 0)
        return true;
    return false;
  }

  friend constexpr void swap(mdspan& __x, mdspan& y) noexcept {
    ranges::swap(__x.__ptr_, y.__ptr_);
    ranges::swap(__x.__map_, y.__map_);
    ranges::swap(__x.__acc_, y.__acc_);
  }

  constexpr const extents_type& extents() const noexcept { return __map_.extents(); }
  constexpr const data_handle_type& data_handle() const noexcept { return __ptr_; }
  constexpr const mapping_type& mapping() const noexcept { return __map_; }
  constexpr const accessor_type& accessor() const noexcept { return __acc_; }

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
    return __map_.is_unique();
  }
  constexpr bool is_exhaustive() const noexcept(noexcept(declval<const mapping_type&>().is_exhaustive())) {
    return __map_.is_exhaustive();
  }
  constexpr bool is_strided() const noexcept(noexcept(declval<const mapping_type&>().is_strided())) {
    return __map_.is_strided();
  }
  constexpr index_type stride(rank_type r) const { return __map_.stride(r); }

private:
  template <size_t... _Pp>
  constexpr reference access(const array<index_type, rank()>& __idx, index_sequence<_Pp...>) const {
    return __acc_.access(__ptr_, static_cast<size_t>(__map_(__idx[_Pp]...)));
  }
  template <class _Indices, size_t... _Pp>
  constexpr reference __subscript(const _Indices& indices, index_sequence<_Pp...>) const {
    return (*this)[__ycxx::__detail::__md_index_cast<index_type>(as_const(indices[_Pp]))...];
  }
  template <class _Indices, size_t... _Pp>
  constexpr reference __at_array(const _Indices& indices, index_sequence<_Pp...>) const {
    return at(__ycxx::__detail::__md_index_cast<index_type>(as_const(indices[_Pp]))...);
  }

  [[no_unique_address]] accessor_type __acc_;
  [[no_unique_address]] mapping_type __map_;
  data_handle_type __ptr_;
};

template <class _CArray>
  requires(is_array_v<_CArray> && rank_v<_CArray> == 1)
mdspan(_CArray&) -> mdspan<remove_all_extents_t<_CArray>, extents<size_t, extent_v<_CArray, 0>>>;

template <class _Pointer>
  requires(is_pointer_v<remove_reference_t<_Pointer>>)
mdspan(_Pointer&&) -> mdspan<remove_pointer_t<remove_reference_t<_Pointer>>, extents<size_t>>;

template <class _ElementType, class... _Integrals>
  requires((is_convertible_v<_Integrals, size_t> && ...) && sizeof...(_Integrals) > 0)
explicit mdspan(_ElementType*,
                _Integrals...) -> mdspan<_ElementType, extents<size_t, __ycxx::__detail::__maybe_static_ext<_Integrals>...>>;

template <class _ElementType, class _OtherIndexType, size_t _Np>
mdspan(_ElementType*, span<_OtherIndexType, _Np>) -> mdspan<_ElementType, dextents<size_t, _Np>>;

template <class _ElementType, class _OtherIndexType, size_t _Np>
mdspan(_ElementType*, const array<_OtherIndexType, _Np>&) -> mdspan<_ElementType, dextents<size_t, _Np>>;

template <class _ElementType, class _IndexType, size_t... _ExtentsPack>
mdspan(_ElementType*,
       const extents<_IndexType, _ExtentsPack...>&) -> mdspan<_ElementType, extents<_IndexType, _ExtentsPack...>>;

template <class _ElementType, class _MappingType>
mdspan(_ElementType*, const _MappingType&)
    -> mdspan<_ElementType, typename _MappingType::extents_type, typename _MappingType::layout_type>;

template <class _MappingType, class _AccessorType>
mdspan(typename _AccessorType::data_handle_type, const _MappingType&,
       const _AccessorType&) -> mdspan<typename _AccessorType::element_type, typename _MappingType::extents_type,
                                      typename _MappingType::layout_type, _AccessorType>;

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Ep, class _Xp, class _Lp, class _Ap>
inline constexpr bool __md_is_mdspan<std::mdspan<_Ep, _Xp, _Lp, _Ap>> = true;

// The element of m at the index i... (no bounds check: the indices come from m's extents).
template <class _Mp, class... _Ip>
constexpr typename _Mp::reference __md_elem(const _Mp& m, _Ip... i) {
  return m.accessor().access(m.data_handle(), static_cast<std::size_t>(m.mapping()(i...)));
}

template <class _Src, class _Dst>
concept __md_copyable =
    __md_is_mdspan<_Src> && __md_is_mdspan<_Dst> && std::is_assignable_v<typename _Dst::reference, typename _Src::reference> &&
    std::is_constructible_v<typename _Src::extents_type, typename _Dst::extents_type>;
template <class _Dst, class _Tp>
concept __md_fillable = __md_is_mdspan<_Dst> && std::is_assignable_v<typename _Dst::reference, const _Tp&>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [mdspan.copy]
template <class _Src, class _Dst>
  requires __ycxx::__detail::__md_copyable<_Src, _Dst>
constexpr void copy(const _Src& __src, const _Dst& __dst) {
  __ycxx::__detail::__precondition(__src.extents() == __dst.extents(), "std::copy(mdspan): the extents differ");
  __ycxx::__detail::__md_for_each_index(
      __src.extents(), [&](auto... i) { __ycxx::__detail::__md_elem(__dst, i...) = __ycxx::__detail::__md_elem(__src, i...); });
}
template <class _ExecutionPolicy, class _Src, class _Dst>
  requires(__ycxx::__detail::__execution_policy<_ExecutionPolicy> && __ycxx::__detail::__md_copyable<_Src, _Dst>)
void copy(_ExecutionPolicy&&, const _Src& __src, const _Dst& __dst) noexcept {
  std::copy(__src, __dst);
}

template <class _Dst, class _Tp = typename _Dst::value_type>
  requires __ycxx::__detail::__md_fillable<_Dst, _Tp>
constexpr void fill(const _Dst& __dst, const _Tp& value) {
  __ycxx::__detail::__md_for_each_index(__dst.extents(), [&](auto... i) { __ycxx::__detail::__md_elem(__dst, i...) = value; });
}
template <class _ExecutionPolicy, class _Dst, class _Tp = typename _Dst::value_type>
  requires(__ycxx::__detail::__execution_policy<_ExecutionPolicy> && __ycxx::__detail::__md_fillable<_Dst, _Tp>)
void fill(_ExecutionPolicy&&, const _Dst& __dst, const _Tp& value) noexcept {
  std::fill(__dst, value);
}

}} // namespace std

#include <ycxx/core/mdspan_sub.hpp>
