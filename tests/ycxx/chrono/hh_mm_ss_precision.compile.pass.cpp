// [time.hms.members]/1-2: fractional_width is the smallest integer in [0, 18] for which precision
// exactly represents all values of Duration, or 6 if there is none (Table 132: hours, minutes and
// seconds 0; milliseconds 3; microseconds 6; nanoseconds 9; 1/2 1; 1/3 6; 1/4 2; 1/5 1; 1/6 6;
// 1/7 6; 1/8 3; 1/9 6; 1/10 1; 756/625 4); precision is
// duration<common_type_t<Duration::rep, seconds::rep>, ratio<1, 10^fractional_width>>.
#include <chrono>
#include <cstdint>
#include <ratio>
#include <type_traits>

using namespace std::chrono;
using std::ratio;

template <class D, unsigned W>
constexpr bool width = hh_mm_ss<D>::fractional_width == W;

static_assert(width<hours, 0> && width<minutes, 0> && width<seconds, 0>);
static_assert(width<milliseconds, 3> && width<microseconds, 6> && width<nanoseconds, 9>);
static_assert(width<duration<int, ratio<1, 2>>, 1> && width<duration<int, ratio<1, 3>>, 6>);
static_assert(width<duration<int, ratio<1, 4>>, 2> && width<duration<int, ratio<1, 5>>, 1>);
static_assert(width<duration<int, ratio<1, 6>>, 6> && width<duration<int, ratio<1, 7>>, 6>);
static_assert(width<duration<int, ratio<1, 8>>, 3> && width<duration<int, ratio<1, 9>>, 6>);
static_assert(width<duration<int, ratio<1, 10>>, 1> && width<duration<int, ratio<756, 625>>, 4>);
static_assert(width<duration<int, ratio<1, 1024>>, 10> && width<duration<long long, std::atto>, 18>);
static_assert(width<days, 0> && width<duration<double>, 0> && width<duration<float, std::milli>, 3>);
static_assert(std::is_same_v<decltype(hh_mm_ss<seconds>::fractional_width), const unsigned>);

static_assert(std::is_same_v<hh_mm_ss<milliseconds>::precision,
                             duration<std::common_type_t<milliseconds::rep, seconds::rep>, std::milli>>);
static_assert(std::is_same_v<hh_mm_ss<minutes>::precision, duration<std::common_type_t<minutes::rep, seconds::rep>>>);
static_assert(std::is_same_v<hh_mm_ss<duration<int, ratio<1, 3>>>::precision,
                             duration<std::common_type_t<int, seconds::rep>, std::micro>>);
static_assert(std::is_same_v<hh_mm_ss<duration<int, ratio<756, 625>>>::precision,
                             duration<std::common_type_t<int, seconds::rep>, ratio<1, 10000>>>);
static_assert(std::is_same_v<hh_mm_ss<duration<double>>::precision, duration<double>>);
static_assert(std::is_same_v<hh_mm_ss<duration<float, std::milli>>::precision,
                             duration<std::common_type_t<float, seconds::rep>, std::milli>>);

int main() {}
