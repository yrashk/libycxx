// libycxx runtime, internal: access to the wait slot table of atomic.cpp for the hosted timed
// wait (src/hosted/thread.cpp).
#pragma once

#include <ycxx/core/cstdint.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// The version counter of the slot of `__addr`; stores the slot's waiter count in `__waiters`.
std::uint32_t* __atomic_wait_entry(const volatile void* __addr, std::uint32_t*& __waiters) noexcept;
}} // namespace __ycxx::__detail
