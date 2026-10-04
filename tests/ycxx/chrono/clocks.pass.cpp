// [time.clock.req], [time.clock.system], [time.clock.steady], [time.clock.hires], [time.clock.file]:
// member types per Table 131 (duration is duration<rep, period>, time_point is a time_point of the
// clock or of another clock with the same epoch), is_steady usable in constant expressions,
// now() noexcept; system_clock::rep is signed (duration::min() < duration::zero()) and
// time_point is time_point<system_clock>; steady_clock::is_steady is true and its values never
// decrease; to_time_t/from_time_t map the same point in time; file_clock uses a signed
// arithmetic rep and noexcept(file_clock::now()); is_clock_v holds for all of them.
#include <chrono>
#include <ctime>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

template <class C>
constexpr bool clock_types =
    std::is_same_v<typename C::duration, duration<typename C::rep, typename C::period>> &&
    std::is_same_v<typename C::time_point::duration, typename C::duration> &&
    std::is_same_v<decltype(C::now()), typename C::time_point> &&
    std::is_same_v<decltype(C::is_steady), const bool> && is_clock_v<C>;

static_assert(clock_types<system_clock> && clock_types<steady_clock> && clock_types<high_resolution_clock>);
static_assert(clock_types<file_clock>);
static_assert(std::is_same_v<system_clock::time_point, time_point<system_clock>>);
static_assert(system_clock::duration::min() < system_clock::duration::zero());
static_assert(steady_clock::is_steady);
constexpr bool sys_steady = system_clock::is_steady;  // usable in constant expressions
constexpr bool hr_steady = high_resolution_clock::is_steady;
static_assert(noexcept(system_clock::now()) && noexcept(steady_clock::now()) &&
              noexcept(high_resolution_clock::now()) && noexcept(file_clock::now()));
static_assert(noexcept(system_clock::to_time_t(system_clock::now())));
static_assert(noexcept(system_clock::from_time_t(std::time_t())));
static_assert(std::is_same_v<decltype(system_clock::to_time_t(system_clock::now())), std::time_t>);
static_assert(std::is_signed_v<file_clock::rep>);
static_assert(std::is_same_v<sys_seconds, time_point<system_clock, seconds>>);
static_assert(std::is_same_v<sys_days, time_point<system_clock, days>>);
static_assert(std::is_same_v<sys_time<minutes>, time_point<system_clock, minutes>>);
static_assert(std::is_same_v<local_days, time_point<local_t, days>>);
static_assert(std::is_same_v<local_seconds, local_time<seconds>>);
static_assert(std::is_same_v<file_time<seconds>, time_point<file_clock, seconds>>);
static_assert(!is_clock_v<local_t>);  // local_t has no now()

int main() {
  (void)sys_steady;
  (void)hr_steady;
  auto s1 = steady_clock::now();
  for (int i = 0; i < 1000; ++i) {
    auto s2 = steady_clock::now();
    CHECK(s1 <= s2);
    s1 = s2;
  }
  // The system clock is Unix time: the current time is after 2020-01-01 and before 2200.
  auto now = system_clock::now();
  CHECK(now > sys_days(days(18262)));
  CHECK(now < sys_days(days(84006)));
  // to_time_t / from_time_t on whole seconds.
  std::time_t t = 1700000000;
  auto tp = system_clock::from_time_t(t);
  CHECK(duration_cast<seconds>(tp.time_since_epoch()).count() == 1700000000);
  CHECK(system_clock::to_time_t(tp) == t);
  CHECK(system_clock::to_time_t(system_clock::from_time_t(0)) == 0);
  CHECK(system_clock::from_time_t(0) == system_clock::time_point());
  // time(nullptr) and system_clock agree to within a minute.
  std::time_t ct = std::time(nullptr);
  CHECK(std::chrono::abs(seconds(ct - system_clock::to_time_t(system_clock::now()))) < minutes(1));
  (void)file_clock::now();
}
