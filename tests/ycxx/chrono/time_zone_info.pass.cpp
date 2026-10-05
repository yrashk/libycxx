// time_zone queries for established past dates in stable zones.
// [time.zone.members]/3: get_info(sys_time) returns the sys_info i with st in [i.begin, i.end);
// [time.zone.info.sys]/3-5: offset = local_time - sys_time, save != 0min on daylight saving
// time; [time.zone.info.local]/2: local_info result unique (first filled, second
// zero-initialized), nonexistent (first ends just prior to the local time, second begins just
// after), ambiguous (first ends just after, second starts just before);
// [time.zone.members]/5-8: to_sys keeps a precision at least as fine as seconds, throws
// ambiguous_local_time / nonexistent_local_time; with choose, the earlier / later sys_time for
// an ambiguous time and the transition point (the same for both) for a nonexistent one;
// to_local. [time.zone.exception.nonexist]/3-4 and [time.zone.exception.ambig]/3-4: what()
// is exactly the text produced by the given ostringstream code (the examples' output).
// Data (IANA tzdata): America/New_York follows the US rules (2:00 local on the second Sunday in
// March / first Sunday in November since 2007, EST -5 / EDT -4); Europe/London GMT / BST from
// 1:00 UTC on the last Sundays of March / October; Asia/Kolkata IST +5:30 without DST since
// 1945; Australia/Lord_Howe +10:30 standard and a 30-minute DST (+11, abbreviated "+11")
// from 2:00 local on the first Sundays of October / April since 2008.
// REQUIRES: exceptions
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

static_assert(local_info::unique == 0 && local_info::nonexistent == 1 && local_info::ambiguous == 2);
static_assert(std::is_base_of_v<std::runtime_error, nonexistent_local_time>);
static_assert(std::is_base_of_v<std::runtime_error, ambiguous_local_time>);
static_assert(std::is_same_v<decltype(std::declval<const time_zone&>().to_sys(local_time<milliseconds>{})),
                             sys_time<milliseconds>>);
static_assert(std::is_same_v<decltype(std::declval<const time_zone&>().to_sys(local_days{}, choose::latest)),
                             sys_seconds>);
static_assert(std::is_same_v<decltype(std::declval<const time_zone&>().to_local(sys_time<minutes>{})),
                             local_seconds>);
static_assert(std::is_same_v<decltype(std::declval<const time_zone&>().get_info(local_days{})), local_info>);

bool zero(const sys_info& i) {
  return i.begin == sys_seconds{} && i.end == sys_seconds{} && i.offset == 0s && i.save == 0min && i.abbrev.empty();
}

int main() {
  const time_zone* ny = locate_zone("America/New_York");
  constexpr sys_days spring2016 = sys_days{2016y / March / 13};   // Sunday[2]/March/2016
  constexpr sys_days fall2016 = sys_days{2016y / November / 6};   // Sunday[1]/November/2016
  static_assert(sys_days{Sunday[2] / March / 2016} == spring2016 && sys_days{Sunday[1] / November / 2016} == fall2016);

  // sys_info around the 2016 transitions (7:00 and 6:00 UTC).
  sys_info s = ny->get_info(spring2016 + 7h - 1s);
  CHECK(s.offset == -5h && s.save == 0min && s.abbrev == "EST");
  CHECK(s.begin == sys_days{2015y / November / 1} + 6h && s.end == spring2016 + 7h);
  s = ny->get_info(spring2016 + 7h);
  CHECK(s.offset == -4h && s.save == 60min && s.abbrev == "EDT");
  CHECK(s.begin == spring2016 + 7h && s.end == fall2016 + 6h);
  CHECK(ny->get_info(sys_time<milliseconds>{fall2016 + 6h - 1ms}).abbrev == "EDT");
  CHECK(ny->get_info(fall2016 + 6h).abbrev == "EST" && ny->get_info(fall2016 + 6h).begin == fall2016 + 6h);
  CHECK(ny->get_info(sys_days{2021y / July / 1}).begin == sys_days{2021y / March / 14} + 7h);
  CHECK(ny->get_info(sys_days{2021y / July / 1}).end == sys_days{2021y / November / 7} + 6h);

  // to_local / to_sys for unique times.
  CHECK(ny->to_local(spring2016 + 7h) == local_days{2016y / March / 13} + 3h);
  CHECK(ny->to_local(spring2016 + 7h - 1s) == local_days{2016y / March / 13} + 1h + 59min + 59s);
  CHECK(ny->to_sys(local_days{2016y / July / 4} + 12h) == sys_days{2016y / July / 4} + 16h);
  CHECK(ny->to_sys(local_time<milliseconds>{local_days{2016y / January / 1} + 1ms}) ==
        sys_time<milliseconds>{sys_days{2016y / January / 1} + 5h + 1ms});

  // local_info: unique.
  local_info li = ny->get_info(local_days{2016y / July / 4} + 12h);
  CHECK(li.result == local_info::unique && li.first.abbrev == "EDT" && li.first.offset == -4h && zero(li.second));
  CHECK(ny->get_info(local_days{2016y / March / 13} + 1h + 59min + 59s).result == local_info::unique);
  CHECK(ny->get_info(local_days{2016y / March / 13} + 3h).result == local_info::unique);
  CHECK(ny->get_info(local_days{2016y / November / 6} + 2h).result == local_info::unique);
  CHECK(ny->get_info(local_days{2016y / November / 6} + 2h).first.abbrev == "EST");
  CHECK(ny->get_info(local_days{2016y / November / 6} + 59min + 59s).first.abbrev == "EDT");

  // local_info: nonexistent ([2:00, 3:00) local on 2016-03-13).
  const local_seconds gap = local_days{2016y / March / 13} + 2h + 30min;
  li = ny->get_info(gap);
  CHECK(li.result == local_info::nonexistent);
  CHECK(li.first.abbrev == "EST" && li.first.end == spring2016 + 7h && li.first.offset == -5h);
  CHECK(li.second.abbrev == "EDT" && li.second.begin == spring2016 + 7h && li.second.offset == -4h);
  CHECK(ny->get_info(local_days{2016y / March / 13} + 2h).result == local_info::nonexistent);
  CHECK(ny->to_sys(gap, choose::earliest) == spring2016 + 7h);
  CHECK(ny->to_sys(gap, choose::latest) == spring2016 + 7h);

  // local_info: ambiguous ([1:00, 2:00) local on 2016-11-06).
  const local_seconds amb = local_days{2016y / November / 6} + 1h + 30min;
  li = ny->get_info(amb);
  CHECK(li.result == local_info::ambiguous);
  CHECK(li.first.abbrev == "EDT" && li.first.end == fall2016 + 6h && li.first.offset == -4h && li.first.save == 60min);
  CHECK(li.second.abbrev == "EST" && li.second.begin == fall2016 + 6h && li.second.offset == -5h);
  CHECK(ny->get_info(local_days{2016y / November / 6} + 1h).result == local_info::ambiguous);
  CHECK(ny->to_sys(amb, choose::earliest) == fall2016 + 5h + 30min);
  CHECK(ny->to_sys(amb, choose::latest) == fall2016 + 6h + 30min);
  CHECK(ny->to_sys(local_time<milliseconds>{amb + 250ms}, choose::latest) ==
        sys_time<milliseconds>{fall2016 + 6h + 30min + 250ms});

  // The exceptions and their what() (the examples' output).
  std::string what;
  try {
    (void)ny->to_sys(gap);
  } catch (const nonexistent_local_time& e) {
    what = e.what();
  }
  CHECK(what ==
        "2016-03-13 02:30:00 is in a gap between\n"
        "2016-03-13 02:00:00 EST and\n"
        "2016-03-13 03:00:00 EDT which are both equivalent to\n"
        "2016-03-13 07:00:00 UTC");
  what.clear();
  try {
    (void)ny->to_sys(amb);
  } catch (const ambiguous_local_time& e) {
    what = e.what();
  }
  CHECK(what ==
        "2016-11-06 01:30:00 is ambiguous.  It could be\n"
        "2016-11-06 01:30:00 EDT == 2016-11-06 05:30:00 UTC or\n"
        "2016-11-06 01:30:00 EST == 2016-11-06 06:30:00 UTC");
  // A finer duration is streamed with its precision (os << tp).
  const local_time<milliseconds> amb_ms = amb + 5ms;
  CHECK(std::string(ambiguous_local_time(amb_ms, ny->get_info(amb_ms)).what()) ==
        "2016-11-06 01:30:00.005 is ambiguous.  It could be\n"
        "2016-11-06 01:30:00.005 EDT == 2016-11-06 05:30:00.005 UTC or\n"
        "2016-11-06 01:30:00.005 EST == 2016-11-06 06:30:00.005 UTC");
  const local_time<minutes> gap_min = local_days{2016y / March / 13} + 2h + 1min;
  CHECK(std::string(nonexistent_local_time(gap_min, ny->get_info(gap_min)).what()) ==
        "2016-03-13 02:01:00 is in a gap between\n"
        "2016-03-13 02:00:00 EST and\n"
        "2016-03-13 03:00:00 EDT which are both equivalent to\n"
        "2016-03-13 07:00:00 UTC");
  bool thrown = false;
  try {
    (void)zoned_time{"America/New_York", local_days{Sunday[2] / March / 2016} + 2h + 30min};
  } catch (const nonexistent_local_time&) {
    thrown = true;
  }
  CHECK(thrown);
  thrown = false;
  try {
    (void)zoned_time{"America/New_York", local_days{Sunday[1] / November / 2016} + 1h + 30min};
  } catch (const std::runtime_error& e) {
    thrown = dynamic_cast<const ambiguous_local_time*>(&e) != nullptr;
  }
  CHECK(thrown);

  // Europe/London.
  const time_zone* london = locate_zone("Europe/London");
  s = london->get_info(sys_days{2019y / June / 1});
  CHECK(s.offset == 1h && s.save == 60min && s.abbrev == "BST");
  CHECK(s.begin == sys_days{2019y / March / 31} + 1h && s.end == sys_days{2019y / October / 27} + 1h);
  s = london->get_info(sys_days{2019y / December / 25});
  CHECK(s.offset == 0s && s.save == 0min && s.abbrev == "GMT");
  CHECK(london->to_sys(local_days{2019y / October / 27} + 1h + 30min, choose::earliest) ==
        sys_days{2019y / October / 27} + 30min);
  CHECK(london->get_info(local_days{2019y / March / 31} + 1h + 30min).result == local_info::nonexistent);

  // Asia/Kolkata: a fractional offset and no transitions since 1945.
  const time_zone* kolkata = locate_zone("Asia/Kolkata");
  s = kolkata->get_info(sys_days{2020y / January / 1});
  CHECK(s.offset == 5h + 30min && s.save == 0min && s.abbrev == "IST");
  CHECK(s.begin < sys_days{1946y / January / 1} && s.end > sys_days{2100y / January / 1});
  CHECK(kolkata->to_local(sys_days{2020y / January / 1}) == local_days{2020y / January / 1} + 5h + 30min);
  li = kolkata->get_info(local_days{2020y / June / 1});
  CHECK(li.result == local_info::unique && li.first.abbrev == "IST" && zero(li.second));

  // Australia/Lord_Howe: southern hemisphere, half-hour DST. In January 2020 DST (+11) is in
  // effect from 2019-10-06 02:00 +10:30 (2019-10-05 15:30 UTC) to 2020-04-05 02:00 +11
  // (2020-04-04 15:00 UTC); the clocks go back to 01:30 local.
  const time_zone* lh = locate_zone("Australia/Lord_Howe");
  s = lh->get_info(sys_days{2020y / January / 15});
  CHECK(s.offset == 11h && s.save == 30min && s.abbrev == "+11");
  CHECK(s.begin == sys_days{2019y / October / 5} + 15h + 30min && s.end == sys_days{2020y / April / 4} + 15h);
  s = lh->get_info(sys_days{2020y / June / 15});
  CHECK(s.offset == 10h + 30min && s.save == 0min && s.abbrev == "+1030");
  li = lh->get_info(local_days{2020y / April / 5} + 1h + 45min);
  CHECK(li.result == local_info::ambiguous && li.first.offset == 11h && li.second.offset == 10h + 30min);
  CHECK(lh->to_sys(local_days{2020y / April / 5} + 1h + 45min, choose::latest) ==
        sys_days{2020y / April / 4} + 15h + 15min);
  li = lh->get_info(local_days{2019y / October / 6} + 2h + 15min);
  CHECK(li.result == local_info::nonexistent && li.first.end == sys_days{2019y / October / 5} + 15h + 30min);

  // UTC.
  s = locate_zone("UTC")->get_info(sys_days{2020y / January / 1});
  CHECK(s.offset == 0s && s.save == 0min && s.abbrev == "UTC");

  // The stream operators exist (unspecified format).
  std::ostringstream os;
  os << s << ny->get_info(gap);
  CHECK(!os.str().empty());
  return 0;
}
