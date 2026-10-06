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

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {
class utc_clock;
class tai_clock;
class gps_clock;
template <class _Duration>
using utc_time = time_point<utc_clock, _Duration>;
using utc_seconds = utc_time<seconds>;
template <class _Duration>
using tai_time = time_point<tai_clock, _Duration>;
using tai_seconds = tai_time<seconds>;
template <class _Duration>
using gps_time = time_point<gps_clock, _Duration>;
using gps_seconds = gps_time<seconds>;

struct sys_info;
struct local_info;
class time_zone;
class time_zone_link;
class leap_second;
struct tzdb;
class tzdb_list;
}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// The tag of the library's own constructors of time_zone, time_zone_link, leap_second, tzdb_list.
struct __tz_ctor_tag {
  explicit __tz_ctor_tag() = default;
};
// leap_second's constructor tag; implicitly constructible, so `leap_second({}, date, value)`
// works (the draft leaves the constructors unspecified).
struct __leap_second_tag {};
// A zone's data, loaded on first use (src/hosted/tzdb.cpp).
struct __tz_data;
struct __tz_data_deleter {
  void operator()(__tz_data* p) const noexcept;
};
struct __tzdb_node;

// The hosted runtime's entry points. The bool functions return false when the zone's data
// cannot be read; the pointer functions return null when nothing can be found or loaded.
bool __tz_get_sys_info(const std::chrono::time_zone& __tz, std::chrono::sys_seconds __st, std::chrono::sys_info& out);
bool __tz_get_local_info(const std::chrono::time_zone& __tz, std::chrono::local_seconds lt, std::chrono::local_info& out);
std::chrono::tzdb_list* __tzdb_list_instance() noexcept;
const std::chrono::tzdb* __tzdb_reload() noexcept;
std::string __tzdb_remote_version();
const std::chrono::time_zone* __tzdb_find(const std::chrono::tzdb& __db, std::string_view name) noexcept;
const std::chrono::time_zone* __tzdb_current(const std::chrono::tzdb& __db) noexcept;
const std::chrono::tzdb* __tzdb_erase_after(std::chrono::tzdb_list& list, const __tzdb_node* p) noexcept;

[[noreturn]] inline void __throw_tz_unknown(std::string_view name) {
  ::__ycxx::__detail::__raise_with(ycxx_error_runtime_error, "std::chrono::locate_zone: unknown time zone", [name] {
    std::string what("std::chrono::locate_zone: unknown time zone \"");
    what.append(name.data(), name.size());
    what.push_back('"');
    return std::runtime_error(what);
  });
}
[[noreturn]] inline void __throw_tz_unreadable(std::string_view name) {
  ::__ycxx::__detail::__raise_with(ycxx_error_runtime_error, "std::chrono::time_zone: cannot read the zone's data", [name] {
    std::string what("std::chrono::time_zone: cannot read the data of time zone \"");
    what.append(name.data(), name.size());
    what.push_back('"');
    return std::runtime_error(what);
  });
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

// [time.zone.leap]
class leap_second {
  sys_seconds __date_;
  seconds __value_;

public:
  constexpr leap_second(__ycxx::__detail::__leap_second_tag, sys_seconds date, seconds value) noexcept
      : __date_(date), __value_(value) {}
  leap_second(const leap_second&) = default;
  leap_second& operator=(const leap_second&) = default;
  constexpr sys_seconds date() const noexcept { return __date_; }
  constexpr seconds value() const noexcept { return __value_; }
};

constexpr bool operator==(const leap_second& __x, const leap_second& y) noexcept { return __x.date() == y.date(); }
constexpr strong_ordering operator<=>(const leap_second& __x, const leap_second& y) noexcept {
  return __x.date() <=> y.date();
}
template <class _Duration>
constexpr bool operator==(const leap_second& __x, const sys_time<_Duration>& y) noexcept {
  return __x.date() == y;
}
template <class _Duration>
constexpr bool operator<(const leap_second& __x, const sys_time<_Duration>& y) noexcept {
  return __x.date() < y;
}
template <class _Duration>
constexpr bool operator<(const sys_time<_Duration>& __x, const leap_second& y) noexcept {
  return __x < y.date();
}
template <class _Duration>
constexpr bool operator>(const leap_second& __x, const sys_time<_Duration>& y) noexcept {
  return y < __x;
}
template <class _Duration>
constexpr bool operator>(const sys_time<_Duration>& __x, const leap_second& y) noexcept {
  return y < __x;
}
template <class _Duration>
constexpr bool operator<=(const leap_second& __x, const sys_time<_Duration>& y) noexcept {
  return !(y < __x);
}
template <class _Duration>
constexpr bool operator<=(const sys_time<_Duration>& __x, const leap_second& y) noexcept {
  return !(y < __x);
}
template <class _Duration>
constexpr bool operator>=(const leap_second& __x, const sys_time<_Duration>& y) noexcept {
  return !(__x < y);
}
template <class _Duration>
constexpr bool operator>=(const sys_time<_Duration>& __x, const leap_second& y) noexcept {
  return !(__x < y);
}
template <class _Duration>
  requires three_way_comparable_with<sys_seconds, sys_time<_Duration>>
constexpr auto operator<=>(const leap_second& __x, const sys_time<_Duration>& y) noexcept {
  return __x.date() <=> y;
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
  template <class _Duration>
  nonexistent_local_time(const local_time<_Duration>& __tp, const local_info& i);
};
class ambiguous_local_time : public runtime_error {
public:
  template <class _Duration>
  ambiguous_local_time(const local_time<_Duration>& __tp, const local_info& i);
};

// [time.zone.timezone]
enum class choose { earliest, latest };

class time_zone {
  string __name_;
  unique_ptr<__ycxx::__detail::__tz_data, __ycxx::__detail::__tz_data_deleter> __data_;

  sys_info __info_at(sys_seconds __st) const {
    sys_info i{};
    if (!::__ycxx::__detail::__tz_get_sys_info(*this, __st, i))
      ::__ycxx::__detail::__throw_tz_unreadable(__name_);
    return i;
  }
  local_info __info_at(local_seconds lt) const {
    local_info i{};
    if (!::__ycxx::__detail::__tz_get_local_info(*this, lt, i))
      ::__ycxx::__detail::__throw_tz_unreadable(__name_);
    return i;
  }

public:
  time_zone(__ycxx::__detail::__tz_ctor_tag, string name, __ycxx::__detail::__tz_data* data) noexcept
      : __name_(static_cast<string&&>(name)), __data_(data) {}
  time_zone(time_zone&&) = default;
  time_zone& operator=(time_zone&&) = default;

  string_view name() const noexcept { return __name_; }
  // The runtime's data of this zone.
  __ycxx::__detail::__tz_data* data(__ycxx::__detail::__tz_ctor_tag) const noexcept { return __data_.get(); }

  template <class _Duration>
  sys_info get_info(const sys_time<_Duration>& __st) const {
    return __info_at(chrono::floor<seconds>(__st));
  }
  template <class _Duration>
  local_info get_info(const local_time<_Duration>& __tp) const {
    return __info_at(chrono::floor<seconds>(__tp));
  }
  template <class _Duration>
  sys_time<common_type_t<_Duration, seconds>> to_sys(const local_time<_Duration>& __tp) const {
    const local_info i = get_info(__tp);
    if (i.result == local_info::nonexistent)
      ::__ycxx::__detail::__raise_with(ycxx_error_nonexistent_local_time, "std::chrono::nonexistent_local_time",
                                 [&] { return nonexistent_local_time(__tp, i); });
    if (i.result == local_info::ambiguous)
      ::__ycxx::__detail::__raise_with(ycxx_error_ambiguous_local_time, "std::chrono::ambiguous_local_time",
                                 [&] { return ambiguous_local_time(__tp, i); });
    return sys_time<common_type_t<_Duration, seconds>>(__tp.time_since_epoch() - i.first.offset);
  }
  template <class _Duration>
  sys_time<common_type_t<_Duration, seconds>> to_sys(const local_time<_Duration>& __tp, choose __z) const {
    using result = sys_time<common_type_t<_Duration, seconds>>;
    const local_info i = get_info(__tp);
    if (i.result == local_info::nonexistent)
      return result(i.first.end); // the transition point, the same for both choices
    if (i.result == local_info::ambiguous && __z == choose::latest)
      return result(__tp.time_since_epoch() - i.second.offset);
    return result(__tp.time_since_epoch() - i.first.offset);
  }
  template <class _Duration>
  local_time<common_type_t<_Duration, seconds>> to_local(const sys_time<_Duration>& __tp) const {
    const sys_info i = get_info(__tp);
    return local_time<common_type_t<_Duration, seconds>>(__tp.time_since_epoch() + i.offset);
  }
};

inline bool operator==(const time_zone& __x, const time_zone& y) noexcept { return __x.name() == y.name(); }
inline strong_ordering operator<=>(const time_zone& __x, const time_zone& y) noexcept { return __x.name() <=> y.name(); }

// [time.zone.link]
class time_zone_link {
  string __name_;
  string __target_;

public:
  time_zone_link(__ycxx::__detail::__tz_ctor_tag, string name, string target) noexcept
      : __name_(static_cast<string&&>(name)), __target_(static_cast<string&&>(target)) {}
  time_zone_link(time_zone_link&&) = default;
  time_zone_link& operator=(time_zone_link&&) = default;
  string_view name() const noexcept { return __name_; }
  string_view target() const noexcept { return __target_; }
};
inline bool operator==(const time_zone_link& __x, const time_zone_link& y) noexcept { return __x.name() == y.name(); }
inline strong_ordering operator<=>(const time_zone_link& __x, const time_zone_link& y) noexcept {
  return __x.name() <=> y.name();
}

// [time.zone.db.tzdb]
struct tzdb {
  string version;
  vector<time_zone> zones;
  vector<time_zone_link> links;
  vector<leap_second> leap_seconds;

  const time_zone* locate_zone(string_view __tz_name) const {
    const time_zone* __z = ::__ycxx::__detail::__tzdb_find(*this, __tz_name);
    if (__z == nullptr)
      ::__ycxx::__detail::__throw_tz_unknown(__tz_name);
    return __z;
  }
  const time_zone* current_zone() const {
    const time_zone* __z = ::__ycxx::__detail::__tzdb_current(*this);
    if (__z == nullptr)
      ::__ycxx::__detail::__throw_tz_unknown("(the local time zone)");
    return __z;
  }
};

}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
struct __tzdb_node {
  std::chrono::tzdb __db;
  __tzdb_node* next;
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

// [time.zone.db.list]: a list the runtime pushes onto (reload_tzdb) and that is never destroyed.
class tzdb_list {
  __ycxx::__detail::__tzdb_node* __head_ = nullptr;
  friend const tzdb* __ycxx::__detail::__tzdb_erase_after(tzdb_list&, const __ycxx::__detail::__tzdb_node*) noexcept;

public:
  explicit tzdb_list(__ycxx::__detail::__tz_ctor_tag) noexcept {}
  tzdb_list(const tzdb_list&) = delete;
  tzdb_list& operator=(const tzdb_list&) = delete;

  // Pushes db onto the front of the list (the runtime's reload_tzdb, under its lock).
  void push_front(__ycxx::__detail::__tz_ctor_tag, __ycxx::__detail::__tzdb_node* n) noexcept {
    n->next = __head_;
    __atomic_store_n(&__head_, n, __ATOMIC_RELEASE);
  }

  class const_iterator {
    const __ycxx::__detail::__tzdb_node* __p_ = nullptr;
    friend tzdb_list;

  public:
    using iterator_category = forward_iterator_tag;
    using value_type = tzdb;
    using difference_type = ptrdiff_t;
    using pointer = const tzdb*;
    using reference = const tzdb&;

    const_iterator() = default;
    explicit const_iterator(__ycxx::__detail::__tz_ctor_tag, const __ycxx::__detail::__tzdb_node* p) noexcept : __p_(p) {}
    const tzdb& operator*() const noexcept { return __p_->__db; }
    const tzdb* operator->() const noexcept { return __builtin_addressof(__p_->__db); }
    const_iterator& operator++() noexcept {
      __p_ = __atomic_load_n(&__p_->next, __ATOMIC_ACQUIRE);
      return *this;
    }
    const_iterator operator++(int) noexcept {
      const_iterator t = *this;
      ++*this;
      return t;
    }
    friend bool operator==(const const_iterator& __x, const const_iterator& y) noexcept { return __x.__p_ == y.__p_; }
    const __ycxx::__detail::__tzdb_node* node(__ycxx::__detail::__tz_ctor_tag) const noexcept { return __p_; }
  };

  const tzdb& front() const noexcept { return __atomic_load_n(&__head_, __ATOMIC_ACQUIRE)->__db; }
  const_iterator erase_after(const_iterator p) {
    ::__ycxx::__detail::__tzdb_erase_after(*this, p.__p_);
    return const_iterator(__ycxx::__detail::__tz_ctor_tag{}, __atomic_load_n(&p.__p_->next, __ATOMIC_ACQUIRE));
  }
  const_iterator begin() const noexcept {
    return const_iterator(__ycxx::__detail::__tz_ctor_tag{}, __atomic_load_n(&__head_, __ATOMIC_ACQUIRE));
  }
  const_iterator end() const noexcept { return const_iterator(); }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }
};

// [time.zone.db.access]
inline tzdb_list& get_tzdb_list() {
  tzdb_list* __l = ::__ycxx::__detail::__tzdb_list_instance();
  if (__l == nullptr)
    ::__ycxx::__detail::__throw_runtime_error("std::chrono::get_tzdb_list: the time zone database cannot be loaded");
  return *__l;
}
inline const tzdb& get_tzdb() { return get_tzdb_list().front(); }
inline const time_zone* locate_zone(string_view __tz_name) { return get_tzdb().locate_zone(__tz_name); }
inline const time_zone* current_zone() { return get_tzdb().current_zone(); }

// [time.zone.db.remote]: the remote database is the zoneinfo directory as it is now.
inline const tzdb& reload_tzdb() {
  const tzdb* __db = ::__ycxx::__detail::__tzdb_reload();
  if (__db == nullptr)
    ::__ycxx::__detail::__throw_runtime_error("std::chrono::reload_tzdb: the time zone database cannot be loaded");
  return *__db;
}
inline string remote_version() { return ::__ycxx::__detail::__tzdb_remote_version(); }

// [time.clock.utc]
struct leap_second_info {
  bool is_leap_second;
  seconds elapsed;
};

class utc_clock {
public:
  using rep = long long;
  using __period = nano;
  using duration = chrono::duration<rep, __period>;
  using time_point = chrono::time_point<utc_clock>;
  static constexpr bool is_steady = false;

  static time_point now();

  template <class _Duration>
  static sys_time<common_type_t<_Duration, seconds>> to_sys(const utc_time<_Duration>& __u) {
    using __cd = common_type_t<_Duration, seconds>;
    seconds sum(0);
    for (const leap_second& __l : get_tzdb().leap_seconds) {
      const seconds start = __l.date().time_since_epoch() + sum; // the leap second's first instant
      if (__u.time_since_epoch() < start)
        break;
      if (__l.value() > seconds(0) && __u.time_since_epoch() < start + __l.value()) {
        // During the insertion: the last representable value before it.
        if constexpr (treat_as_floating_point_v<typename __cd::rep>)
          return sys_time<__cd>(__cd(__l.date().time_since_epoch()));
        else
          return sys_time<__cd>(__cd(__l.date().time_since_epoch()) - __cd(1));
      }
      sum += __l.value();
    }
    return sys_time<__cd>(__cd(__u.time_since_epoch()) - sum);
  }
  template <class _Duration>
  static utc_time<common_type_t<_Duration, seconds>> from_sys(const sys_time<_Duration>& t) {
    using __cd = common_type_t<_Duration, seconds>;
    seconds sum(0);
    for (const leap_second& __l : get_tzdb().leap_seconds) {
      if (__l > t)
        break;
      sum += __l.value();
    }
    return utc_time<__cd>(__cd(t.time_since_epoch()) + sum);
  }
};

inline utc_clock::time_point utc_clock::now() { return from_sys(system_clock::now()); }

template <class _Duration>
leap_second_info get_leap_second_info(const utc_time<_Duration>& __ut) {
  seconds sum(0);
  for (const leap_second& __l : get_tzdb().leap_seconds) {
    const seconds start = __l.date().time_since_epoch() + sum;
    if (__ut.time_since_epoch() < start)
      break;
    sum += __l.value();
    if (__l.value() > seconds(0) && __ut.time_since_epoch() < start + __l.value())
      return {true, sum};
  }
  return {false, sum};
}

// [time.clock.tai]
class tai_clock {
public:
  using rep = long long;
  using __period = nano;
  using duration = chrono::duration<rep, __period>;
  using time_point = chrono::time_point<tai_clock>;
  static constexpr bool is_steady = false;

  static time_point now();
  template <class _Duration>
  static utc_time<common_type_t<_Duration, seconds>> to_utc(const tai_time<_Duration>& t) noexcept {
    return utc_time<common_type_t<_Duration, seconds>>(t.time_since_epoch()) - seconds(378691210);
  }
  template <class _Duration>
  static tai_time<common_type_t<_Duration, seconds>> from_utc(const utc_time<_Duration>& t) noexcept {
    return tai_time<common_type_t<_Duration, seconds>>(t.time_since_epoch()) + seconds(378691210);
  }
};
inline tai_clock::time_point tai_clock::now() { return from_utc(utc_clock::now()); }

// [time.clock.gps]
class gps_clock {
public:
  using rep = long long;
  using __period = nano;
  using duration = chrono::duration<rep, __period>;
  using time_point = chrono::time_point<gps_clock>;
  static constexpr bool is_steady = false;

  static time_point now();
  template <class _Duration>
  static utc_time<common_type_t<_Duration, seconds>> to_utc(const gps_time<_Duration>& t) noexcept {
    return utc_time<common_type_t<_Duration, seconds>>(t.time_since_epoch()) + seconds(315964809);
  }
  template <class _Duration>
  static gps_time<common_type_t<_Duration, seconds>> from_utc(const utc_time<_Duration>& t) noexcept {
    return gps_time<common_type_t<_Duration, seconds>>(t.time_since_epoch()) - seconds(315964809);
  }
};
inline gps_clock::time_point gps_clock::now() { return from_utc(utc_clock::now()); }

// [time.clock.conv]
template <class _DestClock, class _SourceClock>
struct clock_time_conversion {};

// [time.clock.cast.id]
template <class _Clock>
struct clock_time_conversion<_Clock, _Clock> {
  template <class _Duration>
  time_point<_Clock, _Duration> operator()(const time_point<_Clock, _Duration>& t) const {
    return t;
  }
};
template <>
struct clock_time_conversion<system_clock, system_clock> {
  template <class _Duration>
  sys_time<_Duration> operator()(const sys_time<_Duration>& t) const {
    return t;
  }
};
template <>
struct clock_time_conversion<utc_clock, utc_clock> {
  template <class _Duration>
  utc_time<_Duration> operator()(const utc_time<_Duration>& t) const {
    return t;
  }
};

// [time.clock.cast.sys.utc]
template <>
struct clock_time_conversion<utc_clock, system_clock> {
  template <class _Duration>
  utc_time<common_type_t<_Duration, seconds>> operator()(const sys_time<_Duration>& t) const {
    return utc_clock::from_sys(t);
  }
};
template <>
struct clock_time_conversion<system_clock, utc_clock> {
  template <class _Duration>
  sys_time<common_type_t<_Duration, seconds>> operator()(const utc_time<_Duration>& t) const {
    return utc_clock::to_sys(t);
  }
};

}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp, class _Clock>
inline constexpr bool __is_time_point_of = false;
// T, as a type that depends on U: names looked up in it are looked up at the member template's
// instantiation (a failure is then a substitution failure).
template <class _Tp, class _Up>
struct __dependent_type {
  using type = _Tp;
};
template <class _Clock, class _Duration>
inline constexpr bool __is_time_point_of<std::chrono::time_point<_Clock, _Duration>, _Clock> = true;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

// [time.clock.cast.sys]
template <class _SourceClock>
struct clock_time_conversion<system_clock, _SourceClock> {
  template <class _Duration>
  auto operator()(const time_point<_SourceClock, _Duration>& t) const
      -> decltype(__ycxx::__detail::__dependent_type<_SourceClock, _Duration>::type::to_sys(t)) {
    static_assert(
        __ycxx::__detail::__is_time_point_of<decltype(__ycxx::__detail::__dependent_type<_SourceClock, _Duration>::type::to_sys(t)),
                                       system_clock>,
        "clock_time_conversion: SourceClock::to_sys must return a sys_time");
    return __ycxx::__detail::__dependent_type<_SourceClock, _Duration>::type::to_sys(t);
  }
};
template <class _DestClock>
struct clock_time_conversion<_DestClock, system_clock> {
  template <class _Duration>
  auto operator()(const sys_time<_Duration>& t) const
      -> decltype(__ycxx::__detail::__dependent_type<_DestClock, _Duration>::type::from_sys(t)) {
    static_assert(
        __ycxx::__detail::__is_time_point_of<decltype(__ycxx::__detail::__dependent_type<_DestClock, _Duration>::type::from_sys(t)),
                                       _DestClock>,
        "clock_time_conversion: DestClock::from_sys must return a time_point of DestClock");
    return __ycxx::__detail::__dependent_type<_DestClock, _Duration>::type::from_sys(t);
  }
};

// [time.clock.cast.utc]
template <class _SourceClock>
struct clock_time_conversion<utc_clock, _SourceClock> {
  template <class _Duration>
  auto operator()(const time_point<_SourceClock, _Duration>& t) const
      -> decltype(__ycxx::__detail::__dependent_type<_SourceClock, _Duration>::type::to_utc(t)) {
    static_assert(
        __ycxx::__detail::__is_time_point_of<decltype(__ycxx::__detail::__dependent_type<_SourceClock, _Duration>::type::to_utc(t)),
                                       utc_clock>,
        "clock_time_conversion: SourceClock::to_utc must return a utc_time");
    return __ycxx::__detail::__dependent_type<_SourceClock, _Duration>::type::to_utc(t);
  }
};
template <class _DestClock>
struct clock_time_conversion<_DestClock, utc_clock> {
  template <class _Duration>
  auto operator()(const utc_time<_Duration>& t) const
      -> decltype(__ycxx::__detail::__dependent_type<_DestClock, _Duration>::type::from_utc(t)) {
    static_assert(
        __ycxx::__detail::__is_time_point_of<decltype(__ycxx::__detail::__dependent_type<_DestClock, _Duration>::type::from_utc(t)),
                                       _DestClock>,
        "clock_time_conversion: DestClock::from_utc must return a time_point of DestClock");
    return __ycxx::__detail::__dependent_type<_DestClock, _Duration>::type::from_utc(t);
  }
};

}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// The conversion expressions of [time.clock.cast.fn]/1, (1.1) to (1.5).
template <class _Dp, class _Sp, class _Tp>
concept __clock_cast_1 = requires(const _Tp& t) { std::chrono::clock_time_conversion<_Dp, _Sp>{}(t); };
template <class _Dp, class _Sp, class _Tp>
concept __clock_cast_2 = requires(const _Tp& t) {
  std::chrono::clock_time_conversion<_Dp, std::chrono::system_clock>{}(
      std::chrono::clock_time_conversion<std::chrono::system_clock, _Sp>{}(t));
};
template <class _Dp, class _Sp, class _Tp>
concept __clock_cast_3 = requires(const _Tp& t) {
  std::chrono::clock_time_conversion<_Dp, std::chrono::utc_clock>{}(
      std::chrono::clock_time_conversion<std::chrono::utc_clock, _Sp>{}(t));
};
template <class _Dp, class _Sp, class _Tp>
concept __clock_cast_4 = requires(const _Tp& t) {
  std::chrono::clock_time_conversion<_Dp, std::chrono::utc_clock>{}(
      std::chrono::clock_time_conversion<std::chrono::utc_clock, std::chrono::system_clock>{}(
          std::chrono::clock_time_conversion<std::chrono::system_clock, _Sp>{}(t)));
};
template <class _Dp, class _Sp, class _Tp>
concept __clock_cast_5 = requires(const _Tp& t) {
  std::chrono::clock_time_conversion<_Dp, std::chrono::system_clock>{}(
      std::chrono::clock_time_conversion<std::chrono::system_clock, std::chrono::utc_clock>{}(
          std::chrono::clock_time_conversion<std::chrono::utc_clock, _Sp>{}(t)));
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

// [time.clock.cast.fn]: the expression with the fewest conversion calls; it must be unique.
template <class _DestClock, class _SourceClock, class _Duration>
  requires __ycxx::__detail::__clock_cast_1<_DestClock, _SourceClock, time_point<_SourceClock, _Duration>> ||
           __ycxx::__detail::__clock_cast_2<_DestClock, _SourceClock, time_point<_SourceClock, _Duration>> ||
           __ycxx::__detail::__clock_cast_3<_DestClock, _SourceClock, time_point<_SourceClock, _Duration>> ||
           __ycxx::__detail::__clock_cast_4<_DestClock, _SourceClock, time_point<_SourceClock, _Duration>> ||
           __ycxx::__detail::__clock_cast_5<_DestClock, _SourceClock, time_point<_SourceClock, _Duration>>
auto clock_cast(const time_point<_SourceClock, _Duration>& t) {
  using __tp = time_point<_SourceClock, _Duration>;
  constexpr bool __c2 = __ycxx::__detail::__clock_cast_2<_DestClock, _SourceClock, __tp>;
  constexpr bool __c3 = __ycxx::__detail::__clock_cast_3<_DestClock, _SourceClock, __tp>;
  constexpr bool __c4 = __ycxx::__detail::__clock_cast_4<_DestClock, _SourceClock, __tp>;
  constexpr bool __c5 = __ycxx::__detail::__clock_cast_5<_DestClock, _SourceClock, __tp>;
  if constexpr (__ycxx::__detail::__clock_cast_1<_DestClock, _SourceClock, __tp>) {
    return clock_time_conversion<_DestClock, _SourceClock>{}(t);
  } else if constexpr (__c2 || __c3) {
    static_assert(!(__c2 && __c3), "std::chrono::clock_cast: the conversion through system_clock and the one "
                               "through utc_clock are equally good ([time.clock.cast.fn]/2)");
    if constexpr (__c2)
      return clock_time_conversion<_DestClock, system_clock>{}(clock_time_conversion<system_clock, _SourceClock>{}(t));
    else
      return clock_time_conversion<_DestClock, utc_clock>{}(clock_time_conversion<utc_clock, _SourceClock>{}(t));
  } else {
    static_assert(!(__c4 && __c5), "std::chrono::clock_cast: the conversion through system_clock then utc_clock and "
                               "the one through utc_clock then system_clock are equally good "
                               "([time.clock.cast.fn]/2)");
    if constexpr (__c4)
      return clock_time_conversion<_DestClock, utc_clock>{}(clock_time_conversion<utc_clock, system_clock>{}(
          clock_time_conversion<system_clock, _SourceClock>{}(t)));
    else
      return clock_time_conversion<_DestClock, system_clock>{}(clock_time_conversion<system_clock, utc_clock>{}(
          clock_time_conversion<utc_clock, _SourceClock>{}(t)));
  }
}

// [time.zone.zonedtraits]
template <class _Tp>
struct zoned_traits {};
template <>
struct zoned_traits<const time_zone*> {
  static const time_zone* default_zone() { return chrono::locate_zone("UTC"); }
  static const time_zone* locate_zone(string_view name) { return chrono::locate_zone(name); }
};

}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Traits>
concept __zt_has_default = requires { _Traits::default_zone(); };
template <class _Traits, class _TimeZonePtr>
concept __zt_has_locate = requires(std::string_view n) {
  { _Traits::locate_zone(n) } -> std::convertible_to<_TimeZonePtr>;
};
template <class _TimeZonePtr, class _Duration, class _SysDuration>
concept __zt_local_ok = requires(_TimeZonePtr& __z) {
  { __z->to_sys(std::chrono::local_time<_Duration>{}) } -> std::convertible_to<std::chrono::sys_time<_SysDuration>>;
};
template <class _TimeZonePtr, class _Duration, class _SysDuration>
concept __zt_local_choose_ok = requires(_TimeZonePtr& __z) {
  {
    __z->to_sys(std::chrono::local_time<_Duration>{}, std::chrono::choose::earliest)
  } -> std::convertible_to<std::chrono::sys_time<_SysDuration>>;
};
template <class _Tp>
using __zt_representation = std::conditional_t<std::is_convertible_v<_Tp, std::string_view>, const std::chrono::time_zone*,
                                             std::remove_cvref_t<_Tp>>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

// [time.zone.zonedtime]
template <class _Duration, class _TimeZonePtr = const time_zone*>
class zoned_time {
  static_assert(__ycxx::__detail::__is_duration<_Duration>, "zoned_time: Duration must be a specialization of duration");

public:
  using duration = common_type_t<_Duration, seconds>;

private:
  _TimeZonePtr __zone_;
  sys_time<duration> __tp_;
  using __traits_ = zoned_traits<_TimeZonePtr>;

  template <class, class>
  friend class zoned_time;

public:
  zoned_time()
    requires __ycxx::__detail::__zt_has_default<__traits_>
      : __zone_(__traits_::default_zone()), __tp_() {}
  zoned_time(const zoned_time&) = default;
  zoned_time& operator=(const zoned_time&) = default;
  zoned_time(const sys_time<_Duration>& __st)
    requires __ycxx::__detail::__zt_has_default<__traits_>
      : __zone_(__traits_::default_zone()), __tp_(__st) {}
  explicit zoned_time(_TimeZonePtr __z) : __zone_(static_cast<_TimeZonePtr&&>(__z)), __tp_() {}
  explicit zoned_time(string_view name)
    requires __ycxx::__detail::__zt_has_locate<__traits_, _TimeZonePtr>
      : __zone_(__traits_::locate_zone(name)), __tp_() {}
  template <class _Duration2>
    requires is_convertible_v<sys_time<_Duration2>, sys_time<_Duration>>
  zoned_time(const zoned_time<_Duration2, _TimeZonePtr>& y) : __zone_(y.__zone_), __tp_(y.__tp_) {}
  zoned_time(_TimeZonePtr __z, const sys_time<_Duration>& __st) : __zone_(static_cast<_TimeZonePtr&&>(__z)), __tp_(__st) {}
  zoned_time(string_view name, const sys_time<_Duration>& __st)
    requires __ycxx::__detail::__zt_has_locate<__traits_, _TimeZonePtr>
      : zoned_time(__traits_::locate_zone(name), __st) {}
  zoned_time(_TimeZonePtr __z, const local_time<_Duration>& __tp)
    requires __ycxx::__detail::__zt_local_ok<_TimeZonePtr, _Duration, duration>
      : __zone_(static_cast<_TimeZonePtr&&>(__z)), __tp_(__zone_->to_sys(__tp)) {}
  zoned_time(string_view name, const local_time<_Duration>& __tp)
    requires __ycxx::__detail::__zt_has_locate<__traits_, _TimeZonePtr> &&
             __ycxx::__detail::__zt_local_ok<_TimeZonePtr, _Duration, duration>
      : zoned_time(__traits_::locate_zone(name), __tp) {}
  zoned_time(_TimeZonePtr __z, const local_time<_Duration>& __tp, choose c)
    requires __ycxx::__detail::__zt_local_choose_ok<_TimeZonePtr, _Duration, duration>
      : __zone_(static_cast<_TimeZonePtr&&>(__z)), __tp_(__zone_->to_sys(__tp, c)) {}
  zoned_time(string_view name, const local_time<_Duration>& __tp, choose c)
    requires __ycxx::__detail::__zt_has_locate<__traits_, _TimeZonePtr> &&
             __ycxx::__detail::__zt_local_choose_ok<_TimeZonePtr, _Duration, duration>
      : zoned_time(__traits_::locate_zone(name), __tp, c) {}
  template <class _Duration2, class _TimeZonePtr2>
    requires is_convertible_v<sys_time<_Duration2>, sys_time<_Duration>>
  zoned_time(_TimeZonePtr __z, const zoned_time<_Duration2, _TimeZonePtr2>& y)
      : __zone_(static_cast<_TimeZonePtr&&>(__z)), __tp_(y.__tp_) {}
  template <class _Duration2, class _TimeZonePtr2>
    requires is_convertible_v<sys_time<_Duration2>, sys_time<_Duration>>
  zoned_time(_TimeZonePtr __z, const zoned_time<_Duration2, _TimeZonePtr2>& y, choose)
      : zoned_time(static_cast<_TimeZonePtr&&>(__z), y) {}
  template <class _Duration2, class _TimeZonePtr2>
    requires __ycxx::__detail::__zt_has_locate<__traits_, _TimeZonePtr> &&
             is_convertible_v<sys_time<_Duration2>, sys_time<_Duration>>
  zoned_time(string_view name, const zoned_time<_Duration2, _TimeZonePtr2>& y)
      : zoned_time(__traits_::locate_zone(name), y) {}
  template <class _Duration2, class _TimeZonePtr2>
    requires __ycxx::__detail::__zt_has_locate<__traits_, _TimeZonePtr> &&
             is_convertible_v<sys_time<_Duration2>, sys_time<_Duration>>
  zoned_time(string_view name, const zoned_time<_Duration2, _TimeZonePtr2>& y, choose c)
      : zoned_time(__traits_::locate_zone(name), y, c) {}

  zoned_time& operator=(const sys_time<_Duration>& __st) {
    __tp_ = __st;
    return *this;
  }
  zoned_time& operator=(const local_time<_Duration>& lt) {
    __tp_ = __zone_->to_sys(lt);
    return *this;
  }

  operator sys_time<duration>() const { return get_sys_time(); }
  explicit operator local_time<duration>() const { return get_local_time(); }

  _TimeZonePtr get_time_zone() const { return __zone_; }
  local_time<duration> get_local_time() const { return __zone_->to_local(__tp_); }
  sys_time<duration> get_sys_time() const { return __tp_; }
  sys_info get_info() const { return __zone_->get_info(__tp_); }
};

zoned_time() -> zoned_time<seconds>;
template <class _Duration>
zoned_time(sys_time<_Duration>) -> zoned_time<common_type_t<_Duration, seconds>>;
template <class _TimeZonePtrOrName>
zoned_time(_TimeZonePtrOrName&&) -> zoned_time<seconds, __ycxx::__detail::__zt_representation<_TimeZonePtrOrName>>;
template <class _TimeZonePtrOrName, class _Duration>
zoned_time(_TimeZonePtrOrName&&, sys_time<_Duration>)
    -> zoned_time<common_type_t<_Duration, seconds>, __ycxx::__detail::__zt_representation<_TimeZonePtrOrName>>;
template <class _TimeZonePtrOrName, class _Duration>
zoned_time(_TimeZonePtrOrName&&, local_time<_Duration>, choose = choose::earliest)
    -> zoned_time<common_type_t<_Duration, seconds>, __ycxx::__detail::__zt_representation<_TimeZonePtrOrName>>;
template <class _Duration, class _TimeZonePtrOrName, class _TimeZonePtr2>
zoned_time(_TimeZonePtrOrName&&, zoned_time<_Duration, _TimeZonePtr2>, choose = choose::earliest)
    -> zoned_time<common_type_t<_Duration, seconds>, __ycxx::__detail::__zt_representation<_TimeZonePtrOrName>>;

using zoned_seconds = zoned_time<seconds>;

template <class _Duration1, class _Duration2, class _TimeZonePtr>
bool operator==(const zoned_time<_Duration1, _TimeZonePtr>& __x, const zoned_time<_Duration2, _TimeZonePtr>& y) {
  return __x.get_time_zone() == y.get_time_zone() && __x.get_sys_time() == y.get_sys_time();
}

}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] std {
// [time.hash]/4-5
template <class _Duration, class _TimeZonePtr>
  requires __ycxx::__detail::__hash_enabled<_Duration> && __ycxx::__detail::__hash_enabled<_TimeZonePtr>
struct hash<chrono::zoned_time<_Duration, _TimeZonePtr>> {
  size_t operator()(const chrono::zoned_time<_Duration, _TimeZonePtr>& __z) const {
    const size_t a = hash<typename chrono::zoned_time<_Duration, _TimeZonePtr>::duration>{}(
        __z.get_sys_time().time_since_epoch());
    const size_t b = hash<_TimeZonePtr>{}(__z.get_time_zone());
    return a ^ (b + 0x9e3779b97f4a7c15ull + (a << 6) + (a >> 2));
  }
};
template <>
struct hash<chrono::leap_second> {
  size_t operator()(const chrono::leap_second& __l) const noexcept {
    return hash<chrono::seconds::rep>{}(__l.date().time_since_epoch().count());
  }
};
} // namespace std
