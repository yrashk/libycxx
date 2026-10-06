// libycxx core: integer_sequence ([intseq]); its tuple protocol lives in utility_base.hpp.
#pragma once

#include <ycxx/core/meta_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// [intseq]
template <class _Tp, _Tp... _Ip>
struct integer_sequence {
  static_assert(__ycxx::__detail::is_integral_v<_Tp>, "integer_sequence requires an integer type");
  using value_type = _Tp;
  static constexpr size_t size() noexcept { return sizeof...(_Ip); }
};
template <size_t... _Ip>
using index_sequence = integer_sequence<size_t, _Ip...>;
template <class _Tp, _Tp _Np>
using make_integer_sequence = __ycxx::__detail::__y_make_integer_seq<integer_sequence, _Tp, _Np>;
template <size_t _Np>
using make_index_sequence = make_integer_sequence<size_t, _Np>;
template <class... _Tp>
using index_sequence_for = make_index_sequence<sizeof...(_Tp)>;

} // namespace std
