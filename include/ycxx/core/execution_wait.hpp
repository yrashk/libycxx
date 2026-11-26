// Wait operations used by the execution run loop.
#pragma once

#include <ycxx/core/atomic_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail { namespace __exec {

inline unsigned __load_signal(const unsigned& __signal) noexcept {
  return __atomic_load_n(&__signal, __ATOMIC_RELAXED);
}

inline void __wait_for_signal(unsigned& __signal, unsigned __seen) noexcept {
  ::__ycxx::__detail::__atomic_wait_until_done(&__signal, [&] {
    return __atomic_load_n(&__signal, __ATOMIC_ACQUIRE) != __seen;
  });
}

inline void __advance_signal(unsigned& __signal) noexcept {
  __atomic_fetch_add(&__signal, 1u, __ATOMIC_RELEASE);
  ::__ycxx::__detail::__atomic_notify(&__signal);
}

}}} // namespace __ycxx::__detail::__exec
