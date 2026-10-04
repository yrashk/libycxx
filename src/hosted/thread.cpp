// libycxx hosted runtime: the out-of-line parts of the thread support library ([thread]).
#include <ycxx/core/atomic_base.hpp>
#include <ycxx/pal.h>
#include "../runtime/atomic/wait_table.hpp"

namespace ycxx::detail {

// The timed form of atomic_wait_block (atomic_base.hpp): false when it returned because the
// deadline passed.
bool atomic_wait_block_until(const volatile void* addr, std::uint32_t ticket, int clock, long long sec,
                             long long nsec) noexcept {
  std::uint32_t* waiters = nullptr;
  std::uint32_t* version = ::ycxx::detail::atomic_wait_entry(addr, waiters);
  const int r = ::ycxx_pal_wait_until(version, ticket, clock, sec, nsec);
  __atomic_fetch_sub(waiters, 1, __ATOMIC_RELAXED);
  return r == 0;
}

} // namespace ycxx::detail
