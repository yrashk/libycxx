// [time.traits.specializations]: common_type of two durations is duration<common_type_t<Rep1,
// Rep2>, P> with P the greatest common divisor of Period1 and Period2 (gcd of the numerators over
// lcm of the denominators); common_type of two time_points of one clock is the time_point of the
// common duration.
#include <chrono>
#include <cstdint>
#include <ratio>
#include <type_traits>

using namespace std::chrono;
using std::common_type_t;
using std::ratio;

static_assert(std::is_same_v<common_type_t<seconds, milliseconds>, milliseconds>);
static_assert(std::is_same_v<common_type_t<hours, minutes, seconds>, seconds>);
static_assert(std::is_same_v<common_type_t<duration<int, ratio<1, 2>>, duration<int, ratio<1, 3>>>,
                             duration<int, ratio<1, 6>>>);
static_assert(std::is_same_v<common_type_t<duration<int, ratio<2>>, duration<int, ratio<3>>>, duration<int>>);
static_assert(std::is_same_v<common_type_t<duration<int, ratio<4, 3>>, duration<long, ratio<6, 5>>>,
                             duration<long, ratio<2, 15>>>);
static_assert(std::is_same_v<common_type_t<duration<short>, duration<double, std::milli>>,
                             duration<double, std::milli>>);
static_assert(std::is_same_v<common_type_t<duration<int, ratio<6>>, duration<int, ratio<10>>>,
                             duration<int, ratio<2>>>);
static_assert(std::is_same_v<common_type_t<days, weeks>, days>);
static_assert(std::is_same_v<common_type_t<years, months>::period, months::period>);
static_assert(std::is_same_v<common_type_t<years, days>::period, ratio<216>>);  // gcd(31556952, 86400)
static_assert(std::is_same_v<common_type_t<duration<std::int8_t>, duration<std::int8_t>>,
                             duration<common_type_t<std::int8_t, std::int8_t>>>);
// One argument: the duration with its period reduced.
static_assert(std::is_same_v<common_type_t<duration<int, ratio<2, 4>>>, duration<int, ratio<1, 2>>>);

static_assert(std::is_same_v<common_type_t<sys_time<seconds>, sys_time<milliseconds>>, sys_time<milliseconds>>);
static_assert(std::is_same_v<common_type_t<time_point<steady_clock, minutes>, time_point<steady_clock, hours>>,
                             time_point<steady_clock, minutes>>);

template <class... T> concept has_common = requires { typename std::common_type<T...>::type; };
static_assert(!has_common<sys_seconds, time_point<steady_clock, seconds>>);
static_assert(!has_common<seconds, int>);

int main() {}
