// libycxx hosted runtime: timespec_getres (C23 7.29.2.7) for C libraries that lack it
// (YCXX_C_HAS_TIMESPEC_GETRES 0 in config.hpp: Darwin). <ctime> then declares
// std::timespec_getres as a call of this; elsewhere it is unused, but built everywhere so that
// every platform compiles it.
//
// C23 7.29.2.7: if ts is not null and base is supported by timespec_get, sets *ts to the
// resolution of the time base base; returns base if it is supported, else 0. The only base
// supported here is TIME_UTC, the one C23 requires, whose timespec_get is the realtime clock
// (POSIX clock_getres, in the C library's <time.h>).
#include <ctime>

int ycxx::detail::c_timespec_getres(::timespec* ts, int base) noexcept {
  if (base != TIME_UTC)
    return 0;
  ::timespec res{};
  if (::clock_getres(CLOCK_REALTIME, &res) != 0)
    return 0;
  if (ts != nullptr)
    *ts = res;
  return base;
}
