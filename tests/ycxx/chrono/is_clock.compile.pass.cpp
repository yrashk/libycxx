// [time.traits.is.clock]: is_clock<T> is true_type if T meets the Cpp17Clock requirements; at a
// minimum T must provide the types rep, period, duration, time_point, the expression T::is_steady
// and the expression T::now(). is_clock_v<T> is is_clock<T>::value.
#include <chrono>
#include <type_traits>

using namespace std::chrono;

struct my_clock {
  using rep = long long;
  using period = std::milli;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<my_clock>;
  static constexpr bool is_steady = false;
  static time_point now() noexcept { return time_point(); }
};
struct no_now {
  using rep = long;
  using period = std::ratio<1>;
  using duration = std::chrono::duration<rep>;
  using time_point = std::chrono::time_point<no_now, duration>;
  static constexpr bool is_steady = false;
};
struct no_is_steady {
  using rep = long;
  using period = std::ratio<1>;
  using duration = std::chrono::duration<rep>;
  using time_point = std::chrono::time_point<no_is_steady, duration>;
  static time_point now();
};
struct no_rep {
  using period = std::ratio<1>;
  using duration = std::chrono::duration<long>;
  using time_point = std::chrono::time_point<no_rep, duration>;
  static constexpr bool is_steady = false;
  static time_point now();
};
struct no_time_point {
  using rep = long;
  using period = std::ratio<1>;
  using duration = std::chrono::duration<rep>;
  static constexpr bool is_steady = false;
  static int now();
};

static_assert(is_clock_v<my_clock> && is_clock<my_clock>::value);
static_assert(is_clock_v<system_clock> && is_clock_v<steady_clock> && is_clock_v<high_resolution_clock>);
static_assert(is_clock_v<utc_clock> && is_clock_v<tai_clock> && is_clock_v<gps_clock> && is_clock_v<file_clock>);
static_assert(!is_clock_v<no_now> && !is_clock_v<no_is_steady> && !is_clock_v<no_rep> && !is_clock_v<no_time_point>);
static_assert(!is_clock_v<int> && !is_clock_v<seconds> && !is_clock_v<sys_seconds> && !is_clock_v<void>);
static_assert(std::is_base_of_v<std::true_type, is_clock<steady_clock>>);
static_assert(std::is_base_of_v<std::false_type, is_clock<int>>);
static_assert(std::is_same_v<decltype(is_clock_v<int>), const bool>);

int main() {}
