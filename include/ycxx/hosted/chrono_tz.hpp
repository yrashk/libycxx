// libycxx hosted: the leap-second clocks, clock_cast and time zones of <chrono>
// ([time.clock.utc], [time.clock.tai], [time.clock.gps], [time.clock.cast], [time.zone]).
//
// The time zone database is read by the hosted runtime (src/hosted/tzdb.cpp) from the system's
// compiled zoneinfo directory: zone and link names from tzdata.zi (or the directory tree), each
// zone's transitions lazily from its TZif file, leap seconds from leapseconds or
// leap-seconds.list (or an embedded table). The classes here hold only names and opaque
// pointers; every query that needs the data is one out-of-line call. Errors the draft reports by
// throwing are thrown here, in the header, through raise_with (so a -fno-exceptions program
// reaches ycxx_error_handler).
//
// The exception classes' constructors and the stream operators need the chrono formatters and
// are defined in ycxx/hosted/chrono_io.hpp.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/core/chrono_cal.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/stdexcept.hpp>
#include <ycxx/core/string_view.hpp>
#include <ycxx/core/unique_ptr.hpp>
#include <ycxx/core/vector.hpp>
#include <ycxx/hosted/chrono_clocks.hpp>

namespace std::chrono {
class utc_clock;
class tai_clock;
class gps_clock;
template <class Duration>
using utc_time = time_point<utc_clock, Duration>;
using utc_seconds = utc_time<seconds>;
template <class Duration>
using tai_time = time_point<tai_clock, Duration>;
using tai_seconds = tai_time<seconds>;
template <class Duration>
using gps_time = time_point<gps_clock, Duration>;
using gps_seconds = gps_time<seconds>;

struct sys_info;
struct local_info;
class time_zone;
class time_zone_link;
class leap_second;
struct tzdb;
class tzdb_list;
} // namespace std::chrono

namespace ycxx::detail {
// The tag of the library's own constructors of time_zone, time_zone_link, leap_second, tzdb_list.
struct tz_ctor_tag {
  explicit tz_ctor_tag() = default;
};
// leap_second's constructor tag; implicitly constructible, so `leap_second({}, date, value)`
// works (the draft leaves the constructors unspecified).
struct leap_second_tag {};
// A zone's data, loaded on first use (src/hosted/tzdb.cpp).
struct tz_data;
struct tz_data_deleter {
  void operator()(tz_data* p) const noexcept;
};
struct tzdb_node;

// The hosted runtime's entry points. The bool functions return false when the zone's data
// cannot be read; the pointer functions return null when nothing can be found or loaded.
bool tz_get_sys_info(const std::chrono::time_zone& tz, std::chrono::sys_seconds st, std::chrono::sys_info& out);
bool tz_get_local_info(const std::chrono::time_zone& tz, std::chrono::local_seconds lt, std::chrono::local_info& out);
std::chrono::tzdb_list* tzdb_list_instance() noexcept;
const std::chrono::tzdb* tzdb_reload() noexcept;
std::string tzdb_remote_version();
const std::chrono::time_zone* tzdb_find(const std::chrono::tzdb& db, std::string_view name) noexcept;
const std::chrono::time_zone* tzdb_current(const std::chrono::tzdb& db) noexcept;
const std::chrono::tzdb* tzdb_erase_after(std::chrono::tzdb_list& list, const tzdb_node* p) noexcept;

[[noreturn]] inline void throw_tz_unknown(std::string_view name) {
  ::ycxx::detail::raise_with(ycxx_error_runtime_error, "std::chrono::locate_zone: unknown time zone", [name] {
    std::string what("std::chrono::locate_zone: unknown time zone \"");
    what.append(name.data(), name.size());
    what.push_back('"');
    return std::runtime_error(what);
  });
}
[[noreturn]] inline void throw_tz_unreadable(std::string_view name) {
  ::ycxx::detail::raise_with(ycxx_error_runtime_error, "std::chrono::time_zone: cannot read the zone's data", [name] {
    std::string what("std::chrono::time_zone: cannot read the data of time zone \"");
    what.append(name.data(), name.size());
    what.push_back('"');
    return std::runtime_error(what);
  });
}
} // namespace ycxx::detail

namespace std::chrono {

// [time.zone.leap]
class leap_second {
  sys_seconds date_;
  seconds value_;

public:
  constexpr leap_second(ycxx::detail::leap_second_tag, sys_seconds date, seconds value) noexcept
      : date_(date), value_(value) {}
  leap_second(const leap_second&) = default;
  leap_second& operator=(const leap_second&) = default;
  constexpr sys_seconds date() const noexcept { return date_; }
  constexpr seconds value() const noexcept { return value_; }
};

constexpr bool operator==(const leap_second& x, const leap_second& y) noexcept { return x.date() == y.date(); }
constexpr strong_ordering operator<=>(const leap_second& x, const leap_second& y) noexcept {
  return x.date() <=> y.date();
}
template <class Duration>
constexpr bool operator==(const leap_second& x, const sys_time<Duration>& y) noexcept {
  return x.date() == y;
}
template <class Duration>
constexpr bool operator<(const leap_second& x, const sys_time<Duration>& y) noexcept {
  return x.date() < y;
}
template <class Duration>
constexpr bool operator<(const sys_time<Duration>& x, const leap_second& y) noexcept {
  return x < y.date();
}
template <class Duration>
constexpr bool operator>(const leap_second& x, const sys_time<Duration>& y) noexcept {
  return y < x;
}
template <class Duration>
constexpr bool operator>(const sys_time<Duration>& x, const leap_second& y) noexcept {
  return y < x;
}
template <class Duration>
constexpr bool operator<=(const leap_second& x, const sys_time<Duration>& y) noexcept {
  return !(y < x);
}
template <class Duration>
constexpr bool operator<=(const sys_time<Duration>& x, const leap_second& y) noexcept {
  return !(y < x);
}
template <class Duration>
constexpr bool operator>=(const leap_second& x, const sys_time<Duration>& y) noexcept {
  return !(x < y);
}
template <class Duration>
constexpr bool operator>=(const sys_time<Duration>& x, const leap_second& y) noexcept {
  return !(x < y);
}
template <class Duration>
  requires three_way_comparable_with<sys_seconds, sys_time<Duration>>
constexpr auto operator<=>(const leap_second& x, const sys_time<Duration>& y) noexcept {
  return x.date() <=> y;
}

// [time.zone.info]
struct sys_info {
  sys_seconds begin;
  sys_seconds end;
  seconds offset;
  minutes save;
  string abbrev;
};
struct local_info {
  static constexpr int unique = 0;
  static constexpr int nonexistent = 1;
  static constexpr int ambiguous = 2;
  int result;
  sys_info first;
  sys_info second;
};

// [time.zone.exception]: the constructors are defined in ycxx/hosted/chrono_io.hpp.
class nonexistent_local_time : public runtime_error {
public:
  template <class Duration>
  nonexistent_local_time(const local_time<Duration>& tp, const local_info& i);
};
class ambiguous_local_time : public runtime_error {
public:
  template <class Duration>
  ambiguous_local_time(const local_time<Duration>& tp, const local_info& i);
};

// [time.zone.timezone]
enum class choose { earliest, latest };

class time_zone {
  string name_;
  unique_ptr<ycxx::detail::tz_data, ycxx::detail::tz_data_deleter> data_;

  sys_info info_at(sys_seconds st) const {
    sys_info i{};
    if (!::ycxx::detail::tz_get_sys_info(*this, st, i))
      ::ycxx::detail::throw_tz_unreadable(name_);
    return i;
  }
  local_info info_at(local_seconds lt) const {
    local_info i{};
    if (!::ycxx::detail::tz_get_local_info(*this, lt, i))
      ::ycxx::detail::throw_tz_unreadable(name_);
    return i;
  }

public:
  time_zone(ycxx::detail::tz_ctor_tag, string name, ycxx::detail::tz_data* data) noexcept
      : name_(static_cast<string&&>(name)), data_(data) {}
  time_zone(time_zone&&) = default;
  time_zone& operator=(time_zone&&) = default;

  string_view name() const noexcept { return name_; }
  // The runtime's data of this zone.
  ycxx::detail::tz_data* data(ycxx::detail::tz_ctor_tag) const noexcept { return data_.get(); }

  template <class Duration>
  sys_info get_info(const sys_time<Duration>& st) const {
    return info_at(chrono::floor<seconds>(st));
  }
  template <class Duration>
  local_info get_info(const local_time<Duration>& tp) const {
    return info_at(chrono::floor<seconds>(tp));
  }
  template <class Duration>
  sys_time<common_type_t<Duration, seconds>> to_sys(const local_time<Duration>& tp) const {
    const local_info i = get_info(tp);
    if (i.result == local_info::nonexistent)
      ::ycxx::detail::raise_with(ycxx_error_nonexistent_local_time, "std::chrono::nonexistent_local_time",
                                 [&] { return nonexistent_local_time(tp, i); });
    if (i.result == local_info::ambiguous)
      ::ycxx::detail::raise_with(ycxx_error_ambiguous_local_time, "std::chrono::ambiguous_local_time",
                                 [&] { return ambiguous_local_time(tp, i); });
    return sys_time<common_type_t<Duration, seconds>>(tp.time_since_epoch() - i.first.offset);
  }
  template <class Duration>
  sys_time<common_type_t<Duration, seconds>> to_sys(const local_time<Duration>& tp, choose z) const {
    using result = sys_time<common_type_t<Duration, seconds>>;
    const local_info i = get_info(tp);
    if (i.result == local_info::nonexistent)
      return result(i.first.end); // the transition point, the same for both choices
    if (i.result == local_info::ambiguous && z == choose::latest)
      return result(tp.time_since_epoch() - i.second.offset);
    return result(tp.time_since_epoch() - i.first.offset);
  }
  template <class Duration>
  local_time<common_type_t<Duration, seconds>> to_local(const sys_time<Duration>& tp) const {
    const sys_info i = get_info(tp);
    return local_time<common_type_t<Duration, seconds>>(tp.time_since_epoch() + i.offset);
  }
};

inline bool operator==(const time_zone& x, const time_zone& y) noexcept { return x.name() == y.name(); }
inline strong_ordering operator<=>(const time_zone& x, const time_zone& y) noexcept { return x.name() <=> y.name(); }

// [time.zone.link]
class time_zone_link {
  string name_;
  string target_;

public:
  time_zone_link(ycxx::detail::tz_ctor_tag, string name, string target) noexcept
      : name_(static_cast<string&&>(name)), target_(static_cast<string&&>(target)) {}
  time_zone_link(time_zone_link&&) = default;
  time_zone_link& operator=(time_zone_link&&) = default;
  string_view name() const noexcept { return name_; }
  string_view target() const noexcept { return target_; }
};
inline bool operator==(const time_zone_link& x, const time_zone_link& y) noexcept { return x.name() == y.name(); }
inline strong_ordering operator<=>(const time_zone_link& x, const time_zone_link& y) noexcept {
  return x.name() <=> y.name();
}

// [time.zone.db.tzdb]
struct tzdb {
  string version;
  vector<time_zone> zones;
  vector<time_zone_link> links;
  vector<leap_second> leap_seconds;

  const time_zone* locate_zone(string_view tz_name) const {
    const time_zone* z = ::ycxx::detail::tzdb_find(*this, tz_name);
    if (z == nullptr)
      ::ycxx::detail::throw_tz_unknown(tz_name);
    return z;
  }
  const time_zone* current_zone() const {
    const time_zone* z = ::ycxx::detail::tzdb_current(*this);
    if (z == nullptr)
      ::ycxx::detail::throw_tz_unknown("(the local time zone)");
    return z;
  }
};

} // namespace std::chrono

namespace ycxx::detail {
struct tzdb_node {
  std::chrono::tzdb db;
  tzdb_node* next;
};
} // namespace ycxx::detail

namespace std::chrono {

// [time.zone.db.list]: a list the runtime pushes onto (reload_tzdb) and that is never destroyed.
class tzdb_list {
  ycxx::detail::tzdb_node* head_ = nullptr;
  friend const tzdb* ycxx::detail::tzdb_erase_after(tzdb_list&, const ycxx::detail::tzdb_node*) noexcept;

public:
  explicit tzdb_list(ycxx::detail::tz_ctor_tag) noexcept {}
  tzdb_list(const tzdb_list&) = delete;
  tzdb_list& operator=(const tzdb_list&) = delete;

  // Pushes db onto the front of the list (the runtime's reload_tzdb, under its lock).
  void push_front(ycxx::detail::tz_ctor_tag, ycxx::detail::tzdb_node* n) noexcept {
    n->next = head_;
    __atomic_store_n(&head_, n, __ATOMIC_RELEASE);
  }

  class const_iterator {
    const ycxx::detail::tzdb_node* p_ = nullptr;
    friend tzdb_list;

  public:
    using iterator_category = forward_iterator_tag;
    using value_type = tzdb;
    using difference_type = ptrdiff_t;
    using pointer = const tzdb*;
    using reference = const tzdb&;

    const_iterator() = default;
    explicit const_iterator(ycxx::detail::tz_ctor_tag, const ycxx::detail::tzdb_node* p) noexcept : p_(p) {}
    const tzdb& operator*() const noexcept { return p_->db; }
    const tzdb* operator->() const noexcept { return __builtin_addressof(p_->db); }
    const_iterator& operator++() noexcept {
      p_ = __atomic_load_n(&p_->next, __ATOMIC_ACQUIRE);
      return *this;
    }
    const_iterator operator++(int) noexcept {
      const_iterator t = *this;
      ++*this;
      return t;
    }
    friend bool operator==(const const_iterator& x, const const_iterator& y) noexcept { return x.p_ == y.p_; }
    const ycxx::detail::tzdb_node* node(ycxx::detail::tz_ctor_tag) const noexcept { return p_; }
  };

  const tzdb& front() const noexcept { return __atomic_load_n(&head_, __ATOMIC_ACQUIRE)->db; }
  const_iterator erase_after(const_iterator p) {
    ::ycxx::detail::tzdb_erase_after(*this, p.p_);
    return const_iterator(ycxx::detail::tz_ctor_tag{}, __atomic_load_n(&p.p_->next, __ATOMIC_ACQUIRE));
  }
  const_iterator begin() const noexcept {
    return const_iterator(ycxx::detail::tz_ctor_tag{}, __atomic_load_n(&head_, __ATOMIC_ACQUIRE));
  }
  const_iterator end() const noexcept { return const_iterator(); }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }
};

// [time.zone.db.access]
inline tzdb_list& get_tzdb_list() {
  tzdb_list* l = ::ycxx::detail::tzdb_list_instance();
  if (l == nullptr)
    ::ycxx::detail::throw_runtime_error("std::chrono::get_tzdb_list: the time zone database cannot be loaded");
  return *l;
}
inline const tzdb& get_tzdb() { return get_tzdb_list().front(); }
inline const time_zone* locate_zone(string_view tz_name) { return get_tzdb().locate_zone(tz_name); }
inline const time_zone* current_zone() { return get_tzdb().current_zone(); }

// [time.zone.db.remote]: the remote database is the zoneinfo directory as it is now.
inline const tzdb& reload_tzdb() {
  const tzdb* db = ::ycxx::detail::tzdb_reload();
  if (db == nullptr)
    ::ycxx::detail::throw_runtime_error("std::chrono::reload_tzdb: the time zone database cannot be loaded");
  return *db;
}
inline string remote_version() { return ::ycxx::detail::tzdb_remote_version(); }

// [time.clock.utc]
struct leap_second_info {
  bool is_leap_second;
  seconds elapsed;
};

class utc_clock {
public:
  using rep = long long;
  using period = nano;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<utc_clock>;
  static constexpr bool is_steady = false;

  static time_point now();

  template <class Duration>
  static sys_time<common_type_t<Duration, seconds>> to_sys(const utc_time<Duration>& u) {
    using cd = common_type_t<Duration, seconds>;
    seconds sum(0);
    for (const leap_second& l : get_tzdb().leap_seconds) {
      const seconds start = l.date().time_since_epoch() + sum; // the leap second's first instant
      if (u.time_since_epoch() < start)
        break;
      if (l.value() > seconds(0) && u.time_since_epoch() < start + l.value()) {
        // During the insertion: the last representable value before it.
        if constexpr (treat_as_floating_point_v<typename cd::rep>)
          return sys_time<cd>(cd(l.date().time_since_epoch()));
        else
          return sys_time<cd>(cd(l.date().time_since_epoch()) - cd(1));
      }
      sum += l.value();
    }
    return sys_time<cd>(cd(u.time_since_epoch()) - sum);
  }
  template <class Duration>
  static utc_time<common_type_t<Duration, seconds>> from_sys(const sys_time<Duration>& t) {
    using cd = common_type_t<Duration, seconds>;
    seconds sum(0);
    for (const leap_second& l : get_tzdb().leap_seconds) {
      if (l > t)
        break;
      sum += l.value();
    }
    return utc_time<cd>(cd(t.time_since_epoch()) + sum);
  }
};

inline utc_clock::time_point utc_clock::now() { return from_sys(system_clock::now()); }

template <class Duration>
leap_second_info get_leap_second_info(const utc_time<Duration>& ut) {
  seconds sum(0);
  for (const leap_second& l : get_tzdb().leap_seconds) {
    const seconds start = l.date().time_since_epoch() + sum;
    if (ut.time_since_epoch() < start)
      break;
    sum += l.value();
    if (l.value() > seconds(0) && ut.time_since_epoch() < start + l.value())
      return {true, sum};
  }
  return {false, sum};
}

// [time.clock.tai]
class tai_clock {
public:
  using rep = long long;
  using period = nano;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<tai_clock>;
  static constexpr bool is_steady = false;

  static time_point now();
  template <class Duration>
  static utc_time<common_type_t<Duration, seconds>> to_utc(const tai_time<Duration>& t) noexcept {
    return utc_time<common_type_t<Duration, seconds>>(t.time_since_epoch()) - seconds(378691210);
  }
  template <class Duration>
  static tai_time<common_type_t<Duration, seconds>> from_utc(const utc_time<Duration>& t) noexcept {
    return tai_time<common_type_t<Duration, seconds>>(t.time_since_epoch()) + seconds(378691210);
  }
};
inline tai_clock::time_point tai_clock::now() { return from_utc(utc_clock::now()); }

// [time.clock.gps]
class gps_clock {
public:
  using rep = long long;
  using period = nano;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<gps_clock>;
  static constexpr bool is_steady = false;

  static time_point now();
  template <class Duration>
  static utc_time<common_type_t<Duration, seconds>> to_utc(const gps_time<Duration>& t) noexcept {
    return utc_time<common_type_t<Duration, seconds>>(t.time_since_epoch()) + seconds(315964809);
  }
  template <class Duration>
  static gps_time<common_type_t<Duration, seconds>> from_utc(const utc_time<Duration>& t) noexcept {
    return gps_time<common_type_t<Duration, seconds>>(t.time_since_epoch()) - seconds(315964809);
  }
};
inline gps_clock::time_point gps_clock::now() { return from_utc(utc_clock::now()); }

// [time.clock.conv]
template <class DestClock, class SourceClock>
struct clock_time_conversion {};

// [time.clock.cast.id]
template <class Clock>
struct clock_time_conversion<Clock, Clock> {
  template <class Duration>
  time_point<Clock, Duration> operator()(const time_point<Clock, Duration>& t) const {
    return t;
  }
};
template <>
struct clock_time_conversion<system_clock, system_clock> {
  template <class Duration>
  sys_time<Duration> operator()(const sys_time<Duration>& t) const {
    return t;
  }
};
template <>
struct clock_time_conversion<utc_clock, utc_clock> {
  template <class Duration>
  utc_time<Duration> operator()(const utc_time<Duration>& t) const {
    return t;
  }
};

// [time.clock.cast.sys.utc]
template <>
struct clock_time_conversion<utc_clock, system_clock> {
  template <class Duration>
  utc_time<common_type_t<Duration, seconds>> operator()(const sys_time<Duration>& t) const {
    return utc_clock::from_sys(t);
  }
};
template <>
struct clock_time_conversion<system_clock, utc_clock> {
  template <class Duration>
  sys_time<common_type_t<Duration, seconds>> operator()(const utc_time<Duration>& t) const {
    return utc_clock::to_sys(t);
  }
};

} // namespace std::chrono

namespace ycxx::detail {
template <class T, class Clock>
inline constexpr bool is_time_point_of = false;
// T, as a type that depends on U: names looked up in it are looked up at the member template's
// instantiation (a failure is then a substitution failure).
template <class T, class U>
struct dependent_type {
  using type = T;
};
template <class Clock, class Duration>
inline constexpr bool is_time_point_of<std::chrono::time_point<Clock, Duration>, Clock> = true;
} // namespace ycxx::detail

namespace std::chrono {

// [time.clock.cast.sys]
template <class SourceClock>
struct clock_time_conversion<system_clock, SourceClock> {
  template <class Duration>
  auto operator()(const time_point<SourceClock, Duration>& t) const
      -> decltype(ycxx::detail::dependent_type<SourceClock, Duration>::type::to_sys(t)) {
    static_assert(
        ycxx::detail::is_time_point_of<decltype(ycxx::detail::dependent_type<SourceClock, Duration>::type::to_sys(t)),
                                       system_clock>,
        "clock_time_conversion: SourceClock::to_sys must return a sys_time");
    return ycxx::detail::dependent_type<SourceClock, Duration>::type::to_sys(t);
  }
};
template <class DestClock>
struct clock_time_conversion<DestClock, system_clock> {
  template <class Duration>
  auto operator()(const sys_time<Duration>& t) const
      -> decltype(ycxx::detail::dependent_type<DestClock, Duration>::type::from_sys(t)) {
    static_assert(
        ycxx::detail::is_time_point_of<decltype(ycxx::detail::dependent_type<DestClock, Duration>::type::from_sys(t)),
                                       DestClock>,
        "clock_time_conversion: DestClock::from_sys must return a time_point of DestClock");
    return ycxx::detail::dependent_type<DestClock, Duration>::type::from_sys(t);
  }
};

// [time.clock.cast.utc]
template <class SourceClock>
struct clock_time_conversion<utc_clock, SourceClock> {
  template <class Duration>
  auto operator()(const time_point<SourceClock, Duration>& t) const
      -> decltype(ycxx::detail::dependent_type<SourceClock, Duration>::type::to_utc(t)) {
    static_assert(
        ycxx::detail::is_time_point_of<decltype(ycxx::detail::dependent_type<SourceClock, Duration>::type::to_utc(t)),
                                       utc_clock>,
        "clock_time_conversion: SourceClock::to_utc must return a utc_time");
    return ycxx::detail::dependent_type<SourceClock, Duration>::type::to_utc(t);
  }
};
template <class DestClock>
struct clock_time_conversion<DestClock, utc_clock> {
  template <class Duration>
  auto operator()(const utc_time<Duration>& t) const
      -> decltype(ycxx::detail::dependent_type<DestClock, Duration>::type::from_utc(t)) {
    static_assert(
        ycxx::detail::is_time_point_of<decltype(ycxx::detail::dependent_type<DestClock, Duration>::type::from_utc(t)),
                                       DestClock>,
        "clock_time_conversion: DestClock::from_utc must return a time_point of DestClock");
    return ycxx::detail::dependent_type<DestClock, Duration>::type::from_utc(t);
  }
};

} // namespace std::chrono

namespace ycxx::detail {
// The conversion expressions of [time.clock.cast.fn]/1, (1.1) to (1.5).
template <class D, class S, class T>
concept clock_cast_1 = requires(const T& t) { std::chrono::clock_time_conversion<D, S>{}(t); };
template <class D, class S, class T>
concept clock_cast_2 = requires(const T& t) {
  std::chrono::clock_time_conversion<D, std::chrono::system_clock>{}(
      std::chrono::clock_time_conversion<std::chrono::system_clock, S>{}(t));
};
template <class D, class S, class T>
concept clock_cast_3 = requires(const T& t) {
  std::chrono::clock_time_conversion<D, std::chrono::utc_clock>{}(
      std::chrono::clock_time_conversion<std::chrono::utc_clock, S>{}(t));
};
template <class D, class S, class T>
concept clock_cast_4 = requires(const T& t) {
  std::chrono::clock_time_conversion<D, std::chrono::utc_clock>{}(
      std::chrono::clock_time_conversion<std::chrono::utc_clock, std::chrono::system_clock>{}(
          std::chrono::clock_time_conversion<std::chrono::system_clock, S>{}(t)));
};
template <class D, class S, class T>
concept clock_cast_5 = requires(const T& t) {
  std::chrono::clock_time_conversion<D, std::chrono::system_clock>{}(
      std::chrono::clock_time_conversion<std::chrono::system_clock, std::chrono::utc_clock>{}(
          std::chrono::clock_time_conversion<std::chrono::utc_clock, S>{}(t)));
};
} // namespace ycxx::detail

namespace std::chrono {

// [time.clock.cast.fn]: the expression with the fewest conversion calls; it must be unique.
template <class DestClock, class SourceClock, class Duration>
  requires ycxx::detail::clock_cast_1<DestClock, SourceClock, time_point<SourceClock, Duration>> ||
           ycxx::detail::clock_cast_2<DestClock, SourceClock, time_point<SourceClock, Duration>> ||
           ycxx::detail::clock_cast_3<DestClock, SourceClock, time_point<SourceClock, Duration>> ||
           ycxx::detail::clock_cast_4<DestClock, SourceClock, time_point<SourceClock, Duration>> ||
           ycxx::detail::clock_cast_5<DestClock, SourceClock, time_point<SourceClock, Duration>>
auto clock_cast(const time_point<SourceClock, Duration>& t) {
  using tp = time_point<SourceClock, Duration>;
  constexpr bool c2 = ycxx::detail::clock_cast_2<DestClock, SourceClock, tp>;
  constexpr bool c3 = ycxx::detail::clock_cast_3<DestClock, SourceClock, tp>;
  constexpr bool c4 = ycxx::detail::clock_cast_4<DestClock, SourceClock, tp>;
  constexpr bool c5 = ycxx::detail::clock_cast_5<DestClock, SourceClock, tp>;
  if constexpr (ycxx::detail::clock_cast_1<DestClock, SourceClock, tp>) {
    return clock_time_conversion<DestClock, SourceClock>{}(t);
  } else if constexpr (c2 || c3) {
    static_assert(!(c2 && c3), "std::chrono::clock_cast: the conversion through system_clock and the one "
                               "through utc_clock are equally good ([time.clock.cast.fn]/2)");
    if constexpr (c2)
      return clock_time_conversion<DestClock, system_clock>{}(clock_time_conversion<system_clock, SourceClock>{}(t));
    else
      return clock_time_conversion<DestClock, utc_clock>{}(clock_time_conversion<utc_clock, SourceClock>{}(t));
  } else {
    static_assert(!(c4 && c5), "std::chrono::clock_cast: the conversion through system_clock then utc_clock and "
                               "the one through utc_clock then system_clock are equally good "
                               "([time.clock.cast.fn]/2)");
    if constexpr (c4)
      return clock_time_conversion<DestClock, utc_clock>{}(clock_time_conversion<utc_clock, system_clock>{}(
          clock_time_conversion<system_clock, SourceClock>{}(t)));
    else
      return clock_time_conversion<DestClock, system_clock>{}(clock_time_conversion<system_clock, utc_clock>{}(
          clock_time_conversion<utc_clock, SourceClock>{}(t)));
  }
}

// [time.zone.zonedtraits]
template <class T>
struct zoned_traits {};
template <>
struct zoned_traits<const time_zone*> {
  static const time_zone* default_zone() { return chrono::locate_zone("UTC"); }
  static const time_zone* locate_zone(string_view name) { return chrono::locate_zone(name); }
};

} // namespace std::chrono

namespace ycxx::detail {
template <class Traits>
concept zt_has_default = requires { Traits::default_zone(); };
template <class Traits, class TimeZonePtr>
concept zt_has_locate = requires(std::string_view n) {
  { Traits::locate_zone(n) } -> std::convertible_to<TimeZonePtr>;
};
template <class TimeZonePtr, class Duration, class SysDuration>
concept zt_local_ok = requires(TimeZonePtr& z) {
  { z->to_sys(std::chrono::local_time<Duration>{}) } -> std::convertible_to<std::chrono::sys_time<SysDuration>>;
};
template <class TimeZonePtr, class Duration, class SysDuration>
concept zt_local_choose_ok = requires(TimeZonePtr& z) {
  {
    z->to_sys(std::chrono::local_time<Duration>{}, std::chrono::choose::earliest)
  } -> std::convertible_to<std::chrono::sys_time<SysDuration>>;
};
template <class T>
using zt_representation = std::conditional_t<std::is_convertible_v<T, std::string_view>, const std::chrono::time_zone*,
                                             std::remove_cvref_t<T>>;
} // namespace ycxx::detail

namespace std::chrono {

// [time.zone.zonedtime]
template <class Duration, class TimeZonePtr = const time_zone*>
class zoned_time {
  static_assert(ycxx::detail::is_duration<Duration>, "zoned_time: Duration must be a specialization of duration");

public:
  using duration = common_type_t<Duration, seconds>;

private:
  TimeZonePtr zone_;
  sys_time<duration> tp_;
  using traits_ = zoned_traits<TimeZonePtr>;

  template <class, class>
  friend class zoned_time;

public:
  zoned_time()
    requires ycxx::detail::zt_has_default<traits_>
      : zone_(traits_::default_zone()), tp_() {}
  zoned_time(const zoned_time&) = default;
  zoned_time& operator=(const zoned_time&) = default;
  zoned_time(const sys_time<Duration>& st)
    requires ycxx::detail::zt_has_default<traits_>
      : zone_(traits_::default_zone()), tp_(st) {}
  explicit zoned_time(TimeZonePtr z) : zone_(static_cast<TimeZonePtr&&>(z)), tp_() {}
  explicit zoned_time(string_view name)
    requires ycxx::detail::zt_has_locate<traits_, TimeZonePtr>
      : zone_(traits_::locate_zone(name)), tp_() {}
  template <class Duration2>
    requires is_convertible_v<sys_time<Duration2>, sys_time<Duration>>
  zoned_time(const zoned_time<Duration2, TimeZonePtr>& y) : zone_(y.zone_), tp_(y.tp_) {}
  zoned_time(TimeZonePtr z, const sys_time<Duration>& st) : zone_(static_cast<TimeZonePtr&&>(z)), tp_(st) {}
  zoned_time(string_view name, const sys_time<Duration>& st)
    requires ycxx::detail::zt_has_locate<traits_, TimeZonePtr>
      : zoned_time(traits_::locate_zone(name), st) {}
  zoned_time(TimeZonePtr z, const local_time<Duration>& tp)
    requires ycxx::detail::zt_local_ok<TimeZonePtr, Duration, duration>
      : zone_(static_cast<TimeZonePtr&&>(z)), tp_(zone_->to_sys(tp)) {}
  zoned_time(string_view name, const local_time<Duration>& tp)
    requires ycxx::detail::zt_has_locate<traits_, TimeZonePtr> &&
             ycxx::detail::zt_local_ok<TimeZonePtr, Duration, duration>
      : zoned_time(traits_::locate_zone(name), tp) {}
  zoned_time(TimeZonePtr z, const local_time<Duration>& tp, choose c)
    requires ycxx::detail::zt_local_choose_ok<TimeZonePtr, Duration, duration>
      : zone_(static_cast<TimeZonePtr&&>(z)), tp_(zone_->to_sys(tp, c)) {}
  zoned_time(string_view name, const local_time<Duration>& tp, choose c)
    requires ycxx::detail::zt_has_locate<traits_, TimeZonePtr> &&
             ycxx::detail::zt_local_choose_ok<TimeZonePtr, Duration, duration>
      : zoned_time(traits_::locate_zone(name), tp, c) {}
  template <class Duration2, class TimeZonePtr2>
    requires is_convertible_v<sys_time<Duration2>, sys_time<Duration>>
  zoned_time(TimeZonePtr z, const zoned_time<Duration2, TimeZonePtr2>& y)
      : zone_(static_cast<TimeZonePtr&&>(z)), tp_(y.tp_) {}
  template <class Duration2, class TimeZonePtr2>
    requires is_convertible_v<sys_time<Duration2>, sys_time<Duration>>
  zoned_time(TimeZonePtr z, const zoned_time<Duration2, TimeZonePtr2>& y, choose)
      : zoned_time(static_cast<TimeZonePtr&&>(z), y) {}
  template <class Duration2, class TimeZonePtr2>
    requires ycxx::detail::zt_has_locate<traits_, TimeZonePtr> &&
             is_convertible_v<sys_time<Duration2>, sys_time<Duration>>
  zoned_time(string_view name, const zoned_time<Duration2, TimeZonePtr2>& y)
      : zoned_time(traits_::locate_zone(name), y) {}
  template <class Duration2, class TimeZonePtr2>
    requires ycxx::detail::zt_has_locate<traits_, TimeZonePtr> &&
             is_convertible_v<sys_time<Duration2>, sys_time<Duration>>
  zoned_time(string_view name, const zoned_time<Duration2, TimeZonePtr2>& y, choose c)
      : zoned_time(traits_::locate_zone(name), y, c) {}

  zoned_time& operator=(const sys_time<Duration>& st) {
    tp_ = st;
    return *this;
  }
  zoned_time& operator=(const local_time<Duration>& lt) {
    tp_ = zone_->to_sys(lt);
    return *this;
  }

  operator sys_time<duration>() const { return get_sys_time(); }
  explicit operator local_time<duration>() const { return get_local_time(); }

  TimeZonePtr get_time_zone() const { return zone_; }
  local_time<duration> get_local_time() const { return zone_->to_local(tp_); }
  sys_time<duration> get_sys_time() const { return tp_; }
  sys_info get_info() const { return zone_->get_info(tp_); }
};

zoned_time() -> zoned_time<seconds>;
template <class Duration>
zoned_time(sys_time<Duration>) -> zoned_time<common_type_t<Duration, seconds>>;
template <class TimeZonePtrOrName>
zoned_time(TimeZonePtrOrName&&) -> zoned_time<seconds, ycxx::detail::zt_representation<TimeZonePtrOrName>>;
template <class TimeZonePtrOrName, class Duration>
zoned_time(TimeZonePtrOrName&&, sys_time<Duration>)
    -> zoned_time<common_type_t<Duration, seconds>, ycxx::detail::zt_representation<TimeZonePtrOrName>>;
template <class TimeZonePtrOrName, class Duration>
zoned_time(TimeZonePtrOrName&&, local_time<Duration>, choose = choose::earliest)
    -> zoned_time<common_type_t<Duration, seconds>, ycxx::detail::zt_representation<TimeZonePtrOrName>>;
template <class Duration, class TimeZonePtrOrName, class TimeZonePtr2>
zoned_time(TimeZonePtrOrName&&, zoned_time<Duration, TimeZonePtr2>, choose = choose::earliest)
    -> zoned_time<common_type_t<Duration, seconds>, ycxx::detail::zt_representation<TimeZonePtrOrName>>;

using zoned_seconds = zoned_time<seconds>;

template <class Duration1, class Duration2, class TimeZonePtr>
bool operator==(const zoned_time<Duration1, TimeZonePtr>& x, const zoned_time<Duration2, TimeZonePtr>& y) {
  return x.get_time_zone() == y.get_time_zone() && x.get_sys_time() == y.get_sys_time();
}

} // namespace std::chrono

namespace std {
// [time.hash]/4-5
template <class Duration, class TimeZonePtr>
  requires ycxx::detail::hash_enabled<Duration> && ycxx::detail::hash_enabled<TimeZonePtr>
struct hash<chrono::zoned_time<Duration, TimeZonePtr>> {
  size_t operator()(const chrono::zoned_time<Duration, TimeZonePtr>& z) const {
    const size_t a = hash<typename chrono::zoned_time<Duration, TimeZonePtr>::duration>{}(
        z.get_sys_time().time_since_epoch());
    const size_t b = hash<TimeZonePtr>{}(z.get_time_zone());
    return a ^ (b + 0x9e3779b97f4a7c15ull + (a << 6) + (a >> 2));
  }
};
template <>
struct hash<chrono::leap_second> {
  size_t operator()(const chrono::leap_second& l) const noexcept {
    return hash<chrono::seconds::rep>{}(l.date().time_since_epoch().count());
  }
};
} // namespace std
