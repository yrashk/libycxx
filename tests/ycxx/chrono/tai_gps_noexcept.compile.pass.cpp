// [time.clock.tai.overview], [time.clock.gps.overview]: to_utc and from_utc of tai_clock and
// gps_clock are declared noexcept:
//   template<class Duration> static utc_time<common_type_t<Duration, seconds>>
//     to_utc(const tai_time<Duration>&) noexcept;   (and from_utc, and the gps_clock forms)
#include <chrono>

using namespace std::chrono;

static_assert(noexcept(tai_clock::to_utc(tai_seconds())));
static_assert(noexcept(tai_clock::from_utc(utc_seconds())));
static_assert(noexcept(gps_clock::to_utc(gps_seconds())));
static_assert(noexcept(gps_clock::from_utc(utc_seconds())));

int main() {}
