// [time.clock.tai.members]: tai_clock::to_utc(t) is utc_time<...>{t.time_since_epoch()} -
// 378691210s, from_utc(t) adds 378691210s. [time.clock.gps.members]: gps_clock::to_utc(t) adds
// 315964809s, from_utc(t) subtracts it. The result duration is common_type_t<Duration, seconds>.
// The rep of tai_clock and gps_clock is a signed arithmetic type.
#include <chrono>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_signed_v<tai_clock::rep> && std::is_signed_v<gps_clock::rep>);
static_assert(std::is_same_v<tai_clock::time_point, time_point<tai_clock>>);
static_assert(std::is_same_v<gps_clock::time_point, time_point<gps_clock>>);
static_assert(std::is_same_v<decltype(tai_clock::to_utc(tai_time<minutes>())), utc_time<seconds>>);
static_assert(std::is_same_v<decltype(tai_clock::from_utc(utc_time<milliseconds>())), tai_time<milliseconds>>);
static_assert(std::is_same_v<decltype(gps_clock::to_utc(gps_time<hours>())), utc_time<seconds>>);
static_assert(std::is_same_v<tai_seconds, tai_time<seconds>> && std::is_same_v<gps_seconds, gps_time<seconds>>);

int main() {
  CHECK(tai_clock::to_utc(tai_seconds{}).time_since_epoch() == -378691210s);
  CHECK(tai_clock::from_utc(utc_seconds{}).time_since_epoch() == 378691210s);
  CHECK(gps_clock::to_utc(gps_seconds{}).time_since_epoch() == 315964809s);
  CHECK(gps_clock::from_utc(utc_seconds{}).time_since_epoch() == -315964809s);
  CHECK(tai_clock::from_utc(utc_time<milliseconds>(1500ms)).time_since_epoch() == 378691211500ms);
  // The notes: 378691210s == sys_days{1970y/January/1} - sys_days{1958y/January/1} + 10s, and
  // 315964809s == sys_days{1980y/January/Sunday[1]} - sys_days{1970y/January/1} + 9s.
  CHECK(sys_days{1970y / January / 1} - sys_days{1958y / January / 1} + 10s == 378691210s);
  CHECK(sys_days{1980y / January / Sunday[1]} - sys_days{1970y / January / 1} + 9s == 315964809s);
  // Round trips.
  utc_seconds u(1234567890s);
  CHECK(tai_clock::to_utc(tai_clock::from_utc(u)) == u);
  CHECK(gps_clock::to_utc(gps_clock::from_utc(u)) == u);
  CHECK(clock_cast<utc_clock>(clock_cast<tai_clock>(u)) == u);
  CHECK(clock_cast<tai_clock>(clock_cast<gps_clock>(tai_seconds(5s))) == tai_seconds(5s));
  // TAI is 19s ahead of GPS.
  CHECK(clock_cast<tai_clock>(gps_seconds(0s)).time_since_epoch() ==
        sys_days{1980y / January / Sunday[1]} - sys_days{1958y / January / 1} + 19s);
}
