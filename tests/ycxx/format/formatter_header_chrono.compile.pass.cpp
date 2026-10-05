// [time.syn] declares formatter specializations for duration, sys_time, utc_time, tai_time,
// gps_time, file_time, local_time, the calendar types, hh_mm_ss, sys_info, local_info and
// zoned_time ([time.format]): with only <chrono> included each is enabled (semiregular) for
// char and wchar_t, and [format.formatter.spec]/2 applies (formatter_spec.hpp).
// [time.format]/8-9: enable_nonlocking_formatter_optimization of duration is that of Rep, of
// zoned_time<Duration, const time_zone*> true; /3: true for the others (not specified otherwise).
#include <chrono>
#include "formatter_spec.hpp"

namespace c = std::chrono;
using Sec = c::seconds;

template <class charT, class... Ts>
constexpr bool all = (std::semiregular<std::formatter<Ts, charT>> && ...);
template <class... Ts>
constexpr bool both = all<char, Ts...> && all<wchar_t, Ts...>;
template <class... Ts>
constexpr bool nl = (std::enable_nonlocking_formatter_optimization<Ts> && ...);

static_assert(both<c::duration<int>, c::duration<double, std::milli>, c::sys_time<Sec>, c::sys_days, c::utc_time<Sec>,
                   c::tai_time<Sec>, c::gps_time<Sec>, c::file_time<Sec>, c::local_time<Sec>, c::local_days>);
static_assert(both<c::day, c::month, c::year, c::weekday, c::weekday_indexed, c::weekday_last, c::month_day,
                   c::month_day_last, c::month_weekday, c::month_weekday_last, c::year_month, c::year_month_day,
                   c::year_month_day_last, c::year_month_weekday, c::year_month_weekday_last>);
static_assert(both<c::hh_mm_ss<Sec>, c::hh_mm_ss<c::duration<long long, std::nano>>, c::sys_info, c::local_info,
                   c::zoned_time<Sec>>);
static_assert(nl<c::duration<int>, c::duration<double>, c::zoned_time<Sec>>);
static_assert(nl<c::sys_time<Sec>, c::utc_time<Sec>, c::tai_time<Sec>, c::gps_time<Sec>, c::file_time<Sec>,
                 c::local_time<Sec>, c::hh_mm_ss<Sec>, c::sys_info, c::local_info>);
static_assert(nl<c::day, c::month, c::year, c::weekday, c::weekday_indexed, c::weekday_last, c::month_day,
                 c::month_day_last, c::month_weekday, c::month_weekday_last, c::year_month, c::year_month_day,
                 c::year_month_day_last, c::year_month_weekday, c::year_month_weekday_last>);
static_assert(formatter_spec::check());
