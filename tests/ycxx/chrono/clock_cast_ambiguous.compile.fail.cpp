// EXPECT-ERROR: error: static assertion failed[^\n]*std::chrono::clock_cast: the conversion through system_clock and the one through utc_clock are equally good \(\[time\.clock\.cast\.fn\]/2\)
// [time.clock.cast.fn]/2: "Mandates: Among the well-formed clock time conversion expressions
// from the above list, there is a unique best expression." A and B both provide to_sys /
// from_sys and to_utc / from_utc, so clock_cast<B>(a) has two best expressions with two
// conversion calls each, (1.2) through system_clock and (1.3) through utc_clock: ill-formed.
// The controls have a unique best expression: (1.1) for clock_cast<B>(sys_time) and
// clock_cast<system_clock>(a).
#include <chrono>
#include <ratio>
#include <type_traits>

using namespace std::chrono;

template <int N>
struct BothClock {
  using rep = long long;
  using period = std::ratio<1>;
  using duration = seconds;
  using time_point = std::chrono::time_point<BothClock>;
  static constexpr bool is_steady = false;
  static time_point now() noexcept { return time_point{}; }
  template <class D>
  static sys_time<std::common_type_t<D, seconds>> to_sys(const std::chrono::time_point<BothClock, D>& t) {
    return sys_time<std::common_type_t<D, seconds>>{t.time_since_epoch()};
  }
  template <class D>
  static std::chrono::time_point<BothClock, std::common_type_t<D, seconds>> from_sys(const sys_time<D>& t) {
    return std::chrono::time_point<BothClock, std::common_type_t<D, seconds>>{t.time_since_epoch()};
  }
  template <class D>
  static utc_time<std::common_type_t<D, seconds>> to_utc(const std::chrono::time_point<BothClock, D>& t) {
    return utc_time<std::common_type_t<D, seconds>>{t.time_since_epoch()};
  }
  template <class D>
  static std::chrono::time_point<BothClock, std::common_type_t<D, seconds>> from_utc(const utc_time<D>& t) {
    return std::chrono::time_point<BothClock, std::common_type_t<D, seconds>>{t.time_since_epoch()};
  }
};
using A = BothClock<0>;
using B = BothClock<1>;

int main() {
  (void)clock_cast<B>(sys_seconds{});             // control
  (void)clock_cast<system_clock>(A::time_point{});  // control
#ifndef YCXX_CONTROL
  (void)clock_cast<B>(A::time_point{});
#endif
}
