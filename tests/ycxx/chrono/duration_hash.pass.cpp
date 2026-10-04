// [time.hash]/1: "The specialization hash<chrono::duration<Rep, Period>> is enabled if and only if
// hash<Rep> is enabled." /2: hash<time_point<Clock, Duration>> is enabled iff hash<Duration> is.
// Enabled specializations meet Cpp17Hash: equal keys give equal hashes ([unord.hash]).
#include <chrono>
#include <cstddef>
#include <functional>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

struct no_hash_rep {
  long v;
};

template <class T>
constexpr bool enabled = std::is_default_constructible_v<std::hash<T>> &&
                         std::is_invocable_r_v<std::size_t, const std::hash<T>&, const T&>;

static_assert(enabled<seconds> && enabled<duration<double, std::milli>> && enabled<sys_seconds>);
static_assert(enabled<time_point<steady_clock, nanoseconds>>);
static_assert(!enabled<duration<no_hash_rep>>);
static_assert(!std::is_default_constructible_v<std::hash<duration<no_hash_rep>>>);
static_assert(!std::is_default_constructible_v<std::hash<time_point<system_clock, duration<no_hash_rep>>>>);

int main() {
  std::hash<seconds> h;
  CHECK(h(seconds(42)) == h(seconds(42)));
  std::hash<minutes> hm;
  CHECK(hm(minutes(-7)) == std::hash<minutes>{}(minutes(-7)));
  std::hash<sys_seconds> ht;
  CHECK(ht(sys_seconds(seconds(5))) == ht(sys_seconds(seconds(5))));
}
