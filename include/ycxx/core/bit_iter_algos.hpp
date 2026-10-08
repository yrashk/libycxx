// libycxx core: the hook through which fill, find and count work a word at a time on
// vector<bool>'s iterators. vector_bool.hpp specializes bit_algos for them (enabled, with static
// members fill/find/count over [first, last) and a bool value); the algorithms in algo_base.hpp
// and algo_nonmod.hpp use it when the value is a bool and there is no projection.
#pragma once

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Ip>
struct __bit_algos {
  static constexpr bool __enabled = false;
};

}} // namespace __ycxx::__detail
