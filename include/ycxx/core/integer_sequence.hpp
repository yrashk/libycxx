// libycxx core: integer_sequence ([intseq]); its tuple protocol lives in utility_base.hpp.
#pragma once

#include <ycxx/core/meta_base.hpp>

namespace std {

// [intseq]
template <class T, T... I>
struct integer_sequence {
  static_assert(ycxx::detail::is_integral_v<T>, "integer_sequence requires an integer type");
  using value_type = T;
  static constexpr size_t size() noexcept { return sizeof...(I); }
};
template <size_t... I>
using index_sequence = integer_sequence<size_t, I...>;
template <class T, T N>
using make_integer_sequence = ycxx::detail::make_integer_seq<integer_sequence, T, N>;
template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;
template <class... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

} // namespace std
