// [time.syn]: the convenience typedefs: nanoseconds .. hours with signed integer representations
// of at least 64/55/45/35/29/23 bits; days, weeks, years, months with at least 25/22/17/20 bits
// and periods ratio<86400>, ratio<604800>, ratio<31556952> (146097/400 days), ratio<2629746>.
// [time.duration.general]: rep is Rep, period is Period::type.
#include <chrono>
#include <limits>
#include <ratio>
#include <type_traits>

using namespace std::chrono;

template <class D, int bits>
constexpr bool rep_ok = std::is_integral_v<typename D::rep> && std::is_signed_v<typename D::rep> &&
                        std::numeric_limits<typename D::rep>::digits + 1 >= bits;

static_assert(rep_ok<nanoseconds, 64> && std::is_same_v<nanoseconds::period, std::nano>);
static_assert(rep_ok<microseconds, 55> && std::is_same_v<microseconds::period, std::micro>);
static_assert(rep_ok<milliseconds, 45> && std::is_same_v<milliseconds::period, std::milli>);
static_assert(rep_ok<seconds, 35> && std::is_same_v<seconds::period, std::ratio<1>>);
static_assert(rep_ok<minutes, 29> && std::is_same_v<minutes::period, std::ratio<60>>);
static_assert(rep_ok<hours, 23> && std::is_same_v<hours::period, std::ratio<3600>>);
static_assert(rep_ok<days, 25> && std::is_same_v<days::period, std::ratio<86400>>);
static_assert(rep_ok<weeks, 22> && std::is_same_v<weeks::period, std::ratio<604800>>);
static_assert(rep_ok<years, 17> && std::is_same_v<years::period, std::ratio<31556952>>);
static_assert(rep_ok<months, 20> && std::is_same_v<months::period, std::ratio<2629746>>);

// period is Period::type (reduced).
static_assert(std::is_same_v<duration<int, std::ratio<2, 4>>::period, std::ratio<1, 2>>);
static_assert(std::is_same_v<duration<int, std::ratio<-3, -6>>::period, std::ratio<1, 2>>);
static_assert(std::is_same_v<duration<long>::period, std::ratio<1>>);
static_assert(std::is_same_v<duration<double, std::milli>::rep, double>);
static_assert(std::is_same_v<duration<short>::rep, short>);

// Trivially copyable; the copy operations are defaulted.
static_assert(std::is_trivially_copyable_v<seconds>);
static_assert(std::is_trivially_copyable_v<duration<double>>);
static_assert(std::is_nothrow_default_constructible_v<seconds>);
// Storage layout and padding are unspecified; count() exposes the representation value.
static_assert(seconds{42}.count() == 42);
static_assert(duration<double>{1.5}.count() == 1.5);

int main() {}
