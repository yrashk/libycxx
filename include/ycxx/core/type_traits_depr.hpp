// libycxx core: the deprecated type traits of <type_traits> ([depr.meta.types], Annex D):
// is_trivial, is_pod, aligned_storage, aligned_union.
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/meta_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// default-alignment ([depr.meta.types]/11): an object type of size s has an alignment that
// divides s, so the most stringent one for size <= Len is the largest power of two <= Len,
// capped at the fundamental alignment.
consteval std::size_t __default_storage_alignment(std::size_t __len) {
  std::size_t a = 1;
  while (a * 2 <= __len && a * 2 <= alignof(std::max_align_t))
    a *= 2;
  return a;
}

template <std::size_t... _Vp>
inline constexpr std::size_t __max_size_v = [] {
  std::size_t r = 0;
  ((r = _Vp > r ? _Vp : r), ...);
  return r;
}();
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp>
struct [[deprecated("is_trivial is deprecated ([depr.meta.types]); use is_trivially_copyable and "
                    "is_trivially_default_constructible")]] is_trivial : bool_constant<__is_trivial(_Tp)> {};
template <class _Tp>
[[deprecated("is_trivial_v is deprecated ([depr.meta.types])")]] inline constexpr bool is_trivial_v = __is_trivial(_Tp);

template <class _Tp>
struct [[deprecated("is_pod is deprecated ([depr.meta.types]); use is_standard_layout and "
                    "is_trivially_copyable")]] is_pod : bool_constant<__is_pod(_Tp)> {};
template <class _Tp>
[[deprecated("is_pod_v is deprecated ([depr.meta.types])")]] inline constexpr bool is_pod_v = __is_pod(_Tp);

template <size_t _Len, size_t _Align = ::__ycxx::__detail::__default_storage_alignment(_Len)>
struct [[deprecated("aligned_storage is deprecated ([depr.meta.types]); use alignas(Align) std::byte[Len]")]]
    aligned_storage {
  static_assert(_Len != 0, "[depr.meta.types]/12: Len must not be zero");
  static_assert(_Align != 0 && (_Align & (_Align - 1)) == 0, "[depr.meta.types]/12: Align must be an alignment");
  struct type {
    alignas(_Align) unsigned char data[_Len];
  };
};
template <size_t _Len, size_t _Align = ::__ycxx::__detail::__default_storage_alignment(_Len)>
using aligned_storage_t [[deprecated("aligned_storage_t is deprecated ([depr.meta.types])")]] =
    typename aligned_storage<_Len, _Align>::type;

template <size_t _Len, class... _Types>
struct [[deprecated("aligned_union is deprecated ([depr.meta.types]); use alignas(Types...) std::byte[N]")]]
    aligned_union {
  static_assert(sizeof...(_Types) != 0, "[depr.meta.types]/16: at least one type must be provided");
  static_assert((... && (__is_object(_Types) && sizeof(_Types) != 0)),
                "[depr.meta.types]/16: each type must be a complete object type");
  static constexpr size_t alignment_value = ::__ycxx::__detail::__max_size_v<alignof(_Types)...>;
  struct type {
    alignas(alignment_value) unsigned char data[::__ycxx::__detail::__max_size_v<_Len, sizeof(_Types)...>];
  };
};
template <size_t _Len, class... _Types>
using aligned_union_t [[deprecated("aligned_union_t is deprecated ([depr.meta.types])")]] =
    typename aligned_union<_Len, _Types...>::type;

} // namespace std
