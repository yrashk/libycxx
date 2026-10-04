// [time.duration.literals]: h, min, s, ms, us, ns on integer literals give hours, minutes, seconds,
// milliseconds, microseconds, nanoseconds; on floating literals a duration with an unspecified
// floating-point rep and the matching period. They live in std::literals::chrono_literals (both
// inline namespaces) and are visible through using namespace std::chrono ([time.syn]).
#include <chrono>
#include <ratio>
#include <type_traits>
#include "check.hpp"

namespace a {
using namespace std::chrono_literals;
static_assert(std::is_same_v<decltype(1h), std::chrono::hours>);
static_assert(std::is_same_v<decltype(1min), std::chrono::minutes>);
static_assert(std::is_same_v<decltype(1s), std::chrono::seconds>);
static_assert(std::is_same_v<decltype(1ms), std::chrono::milliseconds>);
static_assert(std::is_same_v<decltype(1us), std::chrono::microseconds>);
static_assert(std::is_same_v<decltype(1ns), std::chrono::nanoseconds>);
static_assert(std::is_same_v<decltype(1.5h)::period, std::ratio<3600>>);
static_assert(std::is_same_v<decltype(1.5min)::period, std::ratio<60>>);
static_assert(std::is_same_v<decltype(1.5s)::period, std::ratio<1>>);
static_assert(std::is_same_v<decltype(1.5ms)::period, std::milli>);
static_assert(std::is_same_v<decltype(1.5us)::period, std::micro>);
static_assert(std::is_same_v<decltype(1.5ns)::period, std::nano>);
static_assert(std::is_floating_point_v<decltype(2.0h)::rep> && std::is_floating_point_v<decltype(2.0ns)::rep>);
static_assert(24h == std::chrono::days(1));
static_assert(0.5h == 30min);
static_assert(1.5s == 1500ms);
static_assert(1us == 1000ns);
static_assert(45min + 15min == 1h);
static_assert(1'000'000us == 1s);
static_assert((2.5ms).count() == 2.5);
}  // namespace a

namespace b {
using namespace std::literals;
static_assert(std::is_same_v<decltype(5s), std::chrono::seconds>);
}
namespace c {
using namespace std::literals::chrono_literals;
static_assert(std::is_same_v<decltype(5ms), std::chrono::milliseconds>);
}
namespace d {
using namespace std::chrono;
static_assert(std::is_same_v<decltype(5min), minutes>);
}

int main() {
  using namespace std::chrono_literals;
  auto constexpr aday = 24h;
  auto constexpr lesson = 45min;
  auto constexpr halfanhour = 0.5h;
  CHECK(aday.count() == 24 && lesson.count() == 45 && halfanhour.count() == 0.5);
  CHECK(std::chrono::duration_cast<std::chrono::seconds>(1h + 1min + 1s).count() == 3661);
}
