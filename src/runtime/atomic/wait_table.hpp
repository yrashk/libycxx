// libycxx runtime, internal: access to the wait slot table of atomic.cpp for the hosted timed
// wait (src/hosted/thread.cpp).
#pragma once

#include <ycxx/core/cstdint.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// The version counter of the slot of `addr`; stores the slot's waiter count in `waiters`.
std::uint32_t* atomic_wait_entry(const volatile void* addr, std::uint32_t*& waiters) noexcept;
}} // namespace ycxx::detail
