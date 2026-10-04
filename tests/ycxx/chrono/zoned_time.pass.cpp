// zoned_time. [time.zone.zonedtime.overview]: duration is common_type_t<Duration, seconds>;
// the deduction guides (a name convertible to string_view deduces const time_zone*); /3:
// constructors taking a string_view first do not take part in class template argument
// deduction. [time.zone.zonedtime.ctor]/1-36: the default zone is traits_::default_zone();
// local times go through zone_->to_sys(tp) or to_sys(tp, c); the choose argument has no
// effect when converting from another zoned_time; constraints on default_zone /
// locate_zone. [time.zone.zonedtime.members]/1-10: assignment from sys_time / local_time
// keeps the zone, conversions, get_local_time() == zone_->to_local(tp_), get_info() ==
// zone_->get_info(tp_). [time.zone.zonedtime.nonmembers]/1: == compares the zone pointers and
// the sys times; /2: os << t is format(os.getloc(), "{:L%F %T %Z}", t). [time.format]/18-19:
// formatting uses get_local_time() with the abbreviation and offset of get_info(); the
// default chrono-specs is %F %T %Z; %z / %Ez format the offset ([time.format] Table 133).
// [time.zone.zonedtraits]/1: zoned_traits customises the default zone and the lookup by name
// for a program-defined TimeZonePtr.
// (IANA data: America/New_York EST -5 / EDT -4, Asia/Kolkata IST +5:30; established dates.)
#include <chrono>
#include <format>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

// A program-defined time zone with a fixed offset and abbreviation.
struct FixedZone {
  minutes off;
  const char* abbr;
  template <class D>
  sys_time<std::common_type_t<D, seconds>> to_sys(local_time<D> tp) const {
    return sys_time<std::common_type_t<D, seconds>>{tp.time_since_epoch() - off};
  }
  template <class D>
  sys_time<std::common_type_t<D, seconds>> to_sys(local_time<D> tp, choose) const {
    return to_sys(tp);
  }
  template <class D>
  local_time<std::common_type_t<D, seconds>> to_local(sys_time<D> tp) const {
    return local_time<std::common_type_t<D, seconds>>{tp.time_since_epoch() + off};
  }
  template <class D>
  sys_info get_info(sys_time<D>) const {
    return {sys_seconds::min(), sys_seconds::max(), off, 0min, abbr};
  }
};
inline const FixedZone plus0530{5h + 30min, "P530"};
inline const FixedZone minus0100{-1h, "M1"};
template <>
struct std::chrono::zoned_traits<const FixedZone*> {
  static const FixedZone* default_zone() { return &minus0100; }
  static const FixedZone* locate_zone(std::string_view name) {
    return name == "P530" ? &plus0530 : &minus0100;
  }
};
// A pointer-like zone type without zoned_traits: no default constructor, no lookup by name.
struct NoTraitsZone : FixedZone {};

using ZT = zoned_time<seconds>;
static_assert(std::is_same_v<ZT, zoned_time<seconds, const time_zone*>>);
static_assert(std::is_same_v<zoned_time<milliseconds>::duration, milliseconds>);
static_assert(std::is_same_v<zoned_time<minutes>::duration, seconds>);
static_assert(std::is_same_v<zoned_time<days>::duration, seconds>);
static_assert(std::is_same_v<zoned_time<duration<double>>::duration, duration<double>>);
// Deduction ([time.zone.zonedtime.overview] guides).
static_assert(std::is_same_v<decltype(zoned_time{}), zoned_time<seconds>>);
static_assert(std::is_same_v<decltype(zoned_time{sys_days{}}), zoned_time<seconds>>);
static_assert(std::is_same_v<decltype(zoned_time{sys_time<milliseconds>{}}), zoned_time<milliseconds>>);
static_assert(std::is_same_v<decltype(zoned_time{"UTC"}), zoned_time<seconds>>);
static_assert(std::is_same_v<decltype(zoned_time{std::string("UTC")}), zoned_time<seconds>>);
static_assert(std::is_same_v<decltype(zoned_time{"UTC", sys_days{}}), zoned_time<seconds>>);
static_assert(std::is_same_v<decltype(zoned_time{"UTC", local_days{}}), zoned_time<seconds>>);
static_assert(std::is_same_v<decltype(zoned_time{"UTC", local_time<microseconds>{}}), zoned_time<microseconds>>);
static_assert(std::is_same_v<decltype(zoned_time{"UTC", local_days{}, choose::latest}), zoned_time<seconds>>);
static_assert(std::is_same_v<decltype(zoned_time{std::declval<const time_zone*>(), sys_time<minutes>{}}),
                             zoned_time<seconds>>);
static_assert(std::is_same_v<decltype(zoned_time{&plus0530, sys_time<milliseconds>{}}),
                             zoned_time<milliseconds, const FixedZone*>>);
static_assert(std::is_same_v<decltype(zoned_time{"UTC", std::declval<zoned_time<milliseconds>>()}),
                             zoned_time<milliseconds>>);
// Implicit conversions: sys_time converts implicitly, local_time only explicitly.
static_assert(std::is_convertible_v<ZT, sys_seconds> && !std::is_convertible_v<ZT, local_seconds>);
static_assert(std::is_constructible_v<local_seconds, ZT>);
static_assert(std::is_convertible_v<sys_seconds, ZT>);
static_assert(!std::is_convertible_v<const time_zone*, ZT> && !std::is_convertible_v<std::string_view, ZT>);
static_assert(std::is_constructible_v<zoned_time<milliseconds>, zoned_time<seconds>>);
static_assert(!std::is_constructible_v<zoned_time<seconds>, zoned_time<milliseconds>>);  // /9
// Constraints on zoned_traits ([time.zone.zonedtime.ctor]/1, /3, /7).
static_assert(std::is_default_constructible_v<zoned_time<seconds, const FixedZone*>>);
static_assert(!std::is_default_constructible_v<zoned_time<seconds, const NoTraitsZone*>>);
static_assert(!std::is_constructible_v<zoned_time<seconds, const NoTraitsZone*>, std::string_view>);
static_assert(!std::is_constructible_v<zoned_time<seconds, const NoTraitsZone*>, sys_seconds>);
static_assert(std::is_constructible_v<zoned_time<seconds, const NoTraitsZone*>, const NoTraitsZone*, sys_seconds>);

int main() {
  const time_zone* ny = locate_zone("America/New_York");
  const time_zone* kol = locate_zone("Asia/Kolkata");

  // Defaults: the "UTC" zone and the epoch.
  ZT d;
  CHECK(d.get_time_zone() == locate_zone("UTC") && d.get_sys_time() == sys_seconds{});
  ZT fromsys = sys_seconds{sys_days{2020y / January / 1}};  // implicit from sys_time<Duration>
  CHECK(fromsys.get_time_zone() == locate_zone("UTC") && fromsys.get_sys_time() == sys_days{2020y / January / 1});
  ZT named{"Asia/Kolkata"};
  CHECK(named.get_time_zone() == kol && named.get_sys_time() == sys_seconds{});
  ZT ptr{ny};
  CHECK(ptr.get_time_zone() == ny && ptr.get_sys_time() == sys_seconds{});

  // From sys_time and local_time.
  const sys_seconds noon_utc = sys_days{2021y / July / 4} + 12h;
  ZT a{ny, noon_utc};
  ZT b{"America/New_York", local_days{2021y / July / 4} + 8h};
  CHECK(a == b && a.get_sys_time() == noon_utc);
  CHECK(a.get_local_time() == local_days{2021y / July / 4} + 8h);
  CHECK(static_cast<local_seconds>(a) == a.get_local_time());
  sys_seconds conv = a;
  CHECK(conv == noon_utc);
  const sys_info info = a.get_info();
  CHECK(info.abbrev == "EDT" && info.offset == -4h && info.save == 60min);
  ZT gap{ny, local_days{2016y / March / 13} + 2h + 30min, choose::latest};
  CHECK(gap.get_sys_time() == sys_days{2016y / March / 13} + 7h);
  ZT amb_e{"America/New_York", local_days{2016y / November / 6} + 1h + 30min, choose::earliest};
  ZT amb_l{"America/New_York", local_days{2016y / November / 6} + 1h + 30min, choose::latest};
  CHECK(amb_e.get_sys_time() == sys_days{2016y / November / 6} + 5h + 30min);
  CHECK(amb_l.get_sys_time() == amb_e.get_sys_time() + 1h);
  CHECK(amb_e.get_local_time() == amb_l.get_local_time() && !(amb_e == amb_l));

  // From another zoned_time: the same point in time in another zone; choose has no effect.
  ZT k{kol, amb_l};
  CHECK(k.get_time_zone() == kol && k.get_sys_time() == amb_l.get_sys_time());
  CHECK(k.get_local_time() == local_days{2016y / November / 6} + 12h);
  ZT k2{"Asia/Kolkata", amb_l, choose::earliest};
  CHECK(k2 == k);
  zoned_time<milliseconds> fine{a};
  CHECK(fine.get_sys_time() == noon_utc && fine.get_time_zone() == ny);
  CHECK(fine == a);  // == across durations
  zoned_time<milliseconds> fine2{"America/New_York", local_time<milliseconds>{local_days{2021y / July / 4} + 8h + 1ms}};
  CHECK(!(fine2 == a) && fine2.get_sys_time() == noon_utc + 1ms);

  // A link names the same time_zone object: == compares the pointers.
  auto& links = get_tzdb().links;
  for (auto& l : links)
    if (l.name() == "US/Eastern") CHECK((ZT{"US/Eastern", noon_utc} == a));
  CHECK(!(ZT{kol, noon_utc} == a));

  // Assignment keeps the zone.
  ZT c{ny};
  c = noon_utc;
  CHECK(c == a && c.get_time_zone() == ny);
  c = local_days{2021y / January / 1};
  CHECK(c.get_local_time() == local_days{2021y / January / 1} && c.get_time_zone() == ny);
  CHECK(c.get_sys_time() == sys_days{2021y / January / 1} + 5h);
  ZT copy = c;
  CHECK(copy == c);
  copy = a;
  CHECK(copy == a);

  // Formatting and streaming.
  CHECK(std::format("{}", a) == "2021-07-04 08:00:00 EDT");
  CHECK(std::format("{:%F %T %z}", a) == "2021-07-04 08:00:00 -0400");
  CHECK(std::format("{:%H:%M %Ez %Z}", ZT{kol, noon_utc}) == "17:30 +05:30 IST");
  CHECK(std::format("{:%z}", ZT{"UTC", noon_utc}) == "+0000");
  CHECK(std::format("{}", fine2) == "2021-07-04 08:00:00.001 EDT");
  CHECK(std::format("{:%Y%m%d}", amb_l) == "20161106");
  CHECK(std::format(L"{}", a) == L"2021-07-04 08:00:00 EDT");
  std::ostringstream os;
  os << amb_e << '|' << amb_l;
  CHECK(os.str() == "2016-11-06 01:30:00 EDT|2016-11-06 01:30:00 EST");
  std::wostringstream wos;
  wos << a;
  CHECK(wos.str() == L"2021-07-04 08:00:00 EDT");

  // A program-defined TimeZonePtr with zoned_traits.
  zoned_time<seconds, const FixedZone*> f;
  CHECK(f.get_time_zone() == &minus0100 && f.get_sys_time() == sys_seconds{});
  zoned_time<seconds, const FixedZone*> g{"P530", local_days{2020y / January / 1}};
  CHECK(g.get_time_zone() == &plus0530 && g.get_sys_time() == sys_days{2019y / December / 31} + 18h + 30min);
  CHECK(std::format("{}", g) == "2020-01-01 00:00:00 P530");
  CHECK(std::format("{:%R %z}", g) == "00:00 +0530");
  zoned_time<seconds, const FixedZone*> h{&minus0100, g};
  CHECK(h.get_sys_time() == g.get_sys_time() && h.get_local_time() == local_days{2019y / December / 31} + 17h + 30min);
  CHECK(std::format("{:%T %Ez}", h) == "17:30:00 -01:00");
  static const NoTraitsZone plus2{{2h, "P2"}};
  zoned_time<seconds, const NoTraitsZone*> n{&plus2, local_days{2020y / January / 1}};
  CHECK(n.get_time_zone() == &plus2 && n.get_sys_time() == sys_days{2019y / December / 31} + 22h);
  return 0;
}
