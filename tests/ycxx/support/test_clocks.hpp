// User-defined clocks for the timed waiting functions ([thread.req.timing]).
//
// offset_clock: a Cpp17Clock ([time.clock.req]) distinct from every library clock: millisecond
// ticks (rep long long), an epoch one hour before steady_clock's, is_steady == true. It advances
// with steady_clock (now() is steady_clock::now() truncated to milliseconds, plus one hour), so
// "Clock::now() >= Ct after a timeout" ([thread.req.timing]/4) is meaningful for it.
//
// throwing_clock: like offset_clock, but now() throws clock_error while throwing_clock::armed is
// true, for [thread.req.timing]/8: "A function that takes an argument which specifies a timeout
// will throw if, during its execution, a clock, time point, or time duration throws an
// exception."
#pragma once

#include <atomic>
#include <chrono>
#include <ratio>

struct offset_clock {
  using rep = long long;
  using period = std::milli;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<offset_clock>;
  static constexpr bool is_steady = true;
  static time_point now() {
    return time_point(std::chrono::duration_cast<duration>(
                          std::chrono::steady_clock::now().time_since_epoch()) +
                      std::chrono::hours(1));
  }
};

struct clock_error {};

struct throwing_clock {
  using rep = long long;
  using period = std::milli;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<throwing_clock>;
  static constexpr bool is_steady = true;
  static inline std::atomic<bool> armed{false};
  static inline std::atomic<int> calls{0};
  static time_point now() {
    calls.fetch_add(1);
    if (armed.load()) throw clock_error{};
    return time_point(std::chrono::duration_cast<duration>(
        std::chrono::steady_clock::now().time_since_epoch()));
  }
};
