// time_zone queries at the edges of transitions in zones with offsets that are not whole
// hours, a skipped calendar day, a 30-minute DST, historic war time, and times far past the
// last explicit transition (computed from the zone's continuing rules).
// [time.zone.members]/3: get_info(sys_time st) returns the sys_info i with st in
// [i.begin, i.end); /4: get_info(local_time) "for the local_time tp"; /5-6: to_sys throws
// for nonexistent/ambiguous local times; /7-8: with choose, an ambiguous time gives the
// earlier (choose::earliest) or later sys_time, a nonexistent time gives "the time_point of
// the transition" for both; to_local(tp) is tp + get_info(tp).offset.
// [time.zone.info.sys]/3-4: offset and abbrev are in effect in [begin, end); offset =
// local_time - sys_time; save != 0min for daylight saving time. [time.zone.info.local]/2:
// nonexistent: first is the sys_info that ends just prior to the local time, second the one
// that begins just after; ambiguous: first ends just after, second starts just before.
// [time.zone.zonedtime.members]/8: get_local_time() is zone_->to_local(tp_).
// [time.format] Table 133: %z "-0430"-style offsets, %Ez/%Oz with a colon; %Z the
// abbreviation.
// Data (IANA tzdata, checked with zdump): Pacific/Chatham +12:45 / +13:45 (DST from 2:45
// local on the last Sunday of September to 3:45 local on the first Sunday of April);
// America/St_Johns -3:30 / -2:30 (NST/NDT, the US dates at 2:00 local); Australia/Lord_Howe
// +10:30 / +11 (2:00 local, first Sundays of October and April); Pacific/Apia moved from -10
// (DST) to +14 at 2011-12-30 10:00 UTC, skipping local Friday 2011-12-30 entirely;
// Asia/Kolkata IST +5:30 with war time +6:30 (abbreviated "+0630") from 1941-09-30 18:30 UTC
// to 1942-05-14 17:30 UTC and from 1942-08-31 18:30 UTC to 1945-10-14 17:30 UTC.
#include <chrono>
#include <format>
#include <initializer_list>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

bool zero(const sys_info& i) {
  return i.begin == sys_seconds{} && i.end == sys_seconds{} && i.offset == 0s && i.save == 0min && i.abbrev.empty();
}

int main() {
  // Pacific/Chatham, 2100 (rule-computed): DST ends 2100-04-03 14:00 UTC (03:45 -> 02:45
  // local on Sunday 2100-04-04), starts 2100-09-25 14:00 UTC (02:45 -> 03:45 local).
  const time_zone* chatham = locate_zone("Pacific/Chatham");
  constexpr sys_seconds ch_end = sys_days{2100y / April / 3} + 14h;
  constexpr sys_seconds ch_start = sys_days{2100y / September / 25} + 14h;
  sys_info s = chatham->get_info(ch_end - 1s);
  CHECK(s.offset == 13h + 45min && s.save == 60min && s.abbrev == "+1345" && s.end == ch_end);
  s = chatham->get_info(ch_end);
  CHECK(s.offset == 12h + 45min && s.save == 0min && s.abbrev == "+1245");
  CHECK(s.begin == ch_end && s.end == ch_start);
  CHECK(chatham->get_info(ch_start).begin == ch_start && chatham->get_info(ch_start).offset == 13h + 45min);
  local_info li = chatham->get_info(local_days{2100y / April / 4} + 2h + 45min);  // [02:45, 03:45)
  CHECK(li.result == local_info::ambiguous && li.first.end == ch_end && li.second.begin == ch_end);
  CHECK(chatham->get_info(local_days{2100y / April / 4} + 3h + 44min + 59s).result == local_info::ambiguous);
  CHECK(chatham->get_info(local_days{2100y / April / 4} + 3h + 45min).result == local_info::unique);
  CHECK(chatham->get_info(local_days{2100y / April / 4} + 2h + 44min + 59s).result == local_info::unique);
  CHECK(chatham->to_sys(local_days{2100y / April / 4} + 3h, choose::earliest) == ch_end - 45min);
  CHECK(chatham->to_sys(local_days{2100y / April / 4} + 3h, choose::latest) == ch_end + 15min);
  li = chatham->get_info(local_days{2100y / September / 26} + 3h);                // [02:45, 03:45)
  CHECK(li.result == local_info::nonexistent && li.first.end == ch_start && li.second.begin == ch_start);
  CHECK(li.first.offset == 12h + 45min && li.second.offset == 13h + 45min);
  CHECK(chatham->to_sys(local_days{2100y / September / 26} + 3h, choose::earliest) == ch_start);
  CHECK(chatham->to_sys(local_days{2100y / September / 26} + 3h, choose::latest) == ch_start);
  CHECK(chatham->get_info(local_days{2100y / September / 26} + 2h + 45min).result == local_info::nonexistent);
  CHECK(chatham->get_info(local_days{2100y / September / 26} + 3h + 45min).result == local_info::unique);
  CHECK(chatham->to_local(ch_start) == local_days{2100y / September / 26} + 3h + 45min);
  CHECK(chatham->to_local(ch_start - 1s) == local_days{2100y / September / 26} + 2h + 44min + 59s);
  CHECK(std::format("{:%F %T %Z %z %Ez %Oz}", zoned_time{chatham, ch_start}) ==
        "2100-09-26 03:45:00 +1345 +1345 +13:45 +13:45");

  // America/St_Johns, 2100: NDT from 2100-03-14 05:30 UTC to 2100-11-07 04:30 UTC.
  const time_zone* sj = locate_zone("America/St_Johns");
  constexpr sys_seconds sj_start = sys_days{2100y / March / 14} + 5h + 30min;
  constexpr sys_seconds sj_end = sys_days{2100y / November / 7} + 4h + 30min;
  s = sj->get_info(sys_days{2100y / July / 1});
  CHECK(s.begin == sj_start && s.end == sj_end && s.offset == -(2h + 30min) && s.save == 60min && s.abbrev == "NDT");
  s = sj->get_info(sj_end);
  CHECK(s.begin == sj_end && s.offset == -(3h + 30min) && s.save == 0min && s.abbrev == "NST");
  CHECK(sj->to_sys(local_days{2100y / November / 7} + 1h + 15min, choose::earliest) == sj_end - 45min);
  CHECK(sj->to_sys(local_days{2100y / November / 7} + 1h + 15min, choose::latest) == sj_end + 15min);
  CHECK(sj->to_sys(local_days{2100y / March / 14} + 2h + 1min, choose::latest) == sj_start);
  CHECK(std::format("{:%T %Z %z %Ez}", zoned_time{sj, sj_end - 1s}) == "01:59:59 NDT -0230 -02:30");
  CHECK(std::format("{:%T %Z %z %Ez}", zoned_time{sj, sj_end}) == "01:00:00 NST -0330 -03:30");

  // Australia/Lord_Howe, 2100: the 30-minute DST ends 2100-04-03 15:00 UTC (02:00 +11 -> 01:30
  // +10:30) and starts 2100-10-02 15:30 UTC (02:00 +10:30 -> 02:30 +11).
  const time_zone* lh = locate_zone("Australia/Lord_Howe");
  constexpr sys_seconds lh_end = sys_days{2100y / April / 3} + 15h;
  constexpr sys_seconds lh_start = sys_days{2100y / October / 2} + 15h + 30min;
  s = lh->get_info(lh_end);
  CHECK(s.begin == lh_end && s.end == lh_start && s.offset == 10h + 30min && s.save == 0min);
  s = lh->get_info(lh_end - 1s);
  CHECK(s.end == lh_end && s.offset == 11h && s.save == 30min);
  li = lh->get_info(local_days{2100y / October / 3} + 2h + 29min + 59s);
  CHECK(li.result == local_info::nonexistent && li.first.end == lh_start && li.second.begin == lh_start);
  CHECK(lh->get_info(local_days{2100y / October / 3} + 2h + 30min).result == local_info::unique);
  CHECK(lh->get_info(local_days{2100y / April / 4} + 1h + 30min).result == local_info::ambiguous);
  CHECK(lh->get_info(local_days{2100y / April / 4} + 1h + 29min + 59s).result == local_info::unique);
  CHECK(lh->to_sys(local_days{2100y / April / 4} + 1h + 59min + 59s, choose::earliest) == lh_end - 1s);
  CHECK(lh->to_sys(local_days{2100y / April / 4} + 1h + 59min + 59s, choose::latest) == lh_end + 29min + 59s);
  CHECK(lh->get_info(local_days{2100y / April / 4} + 2h).result == local_info::unique);

  // Pacific/Apia: the whole local day 2011-12-30 does not exist.
  const time_zone* apia = locate_zone("Pacific/Apia");
  constexpr sys_seconds apia_jump = sys_days{2011y / December / 30} + 10h;
  for (const local_seconds lt : std::initializer_list<local_seconds>{local_days{2011y / December / 30} + 0s, local_days{2011y / December / 30} + 12h,
                                 local_days{2011y / December / 31} - 1s}) {
    li = apia->get_info(lt);
    CHECK(li.result == local_info::nonexistent);
    CHECK(li.first.end == apia_jump && li.second.begin == apia_jump);
    CHECK(li.first.offset == -10h && li.second.offset == 14h);
    CHECK(apia->to_sys(lt, choose::earliest) == apia_jump && apia->to_sys(lt, choose::latest) == apia_jump);
  }
  li = apia->get_info(local_days{2011y / December / 29} + 23h + 59min + 59s);
  CHECK(li.result == local_info::unique && li.first.end == apia_jump && zero(li.second));
  li = apia->get_info(local_days{2011y / December / 31});
  CHECK(li.result == local_info::unique && li.first.begin == apia_jump && li.first.offset == 14h);
  CHECK(apia->to_local(apia_jump - 1s) == local_days{2011y / December / 29} + 23h + 59min + 59s);
  CHECK(apia->to_local(apia_jump) == local_days{2011y / December / 31});
  CHECK(std::format("{:%F %a %z}", zoned_time{apia, apia_jump}) == "2011-12-31 Sat +1400");
  CHECK(std::format("{:%F %a %z}", zoned_time{apia, apia_jump - 1s}) == "2011-12-29 Thu -1000");

  // Asia/Kolkata war time: +0630 (save 60min) between IST periods; the 1942 gap/overlap.
  const time_zone* kol = locate_zone("Asia/Kolkata");
  s = kol->get_info(sys_days{1943y / January / 1});
  CHECK(s.begin == sys_days{1942y / August / 31} + 18h + 30min && s.end == sys_days{1945y / October / 14} + 17h + 30min);
  CHECK(s.offset == 6h + 30min && s.save == 60min && s.abbrev == "+0630");
  s = kol->get_info(sys_days{1942y / July / 1});
  CHECK(s.begin == sys_days{1942y / May / 14} + 17h + 30min && s.end == sys_days{1942y / August / 31} + 18h + 30min);
  CHECK(s.offset == 5h + 30min && s.save == 0min && s.abbrev == "IST");
  // 1942-05-14 23:00..24:00 local is ambiguous (+6:30 then +5:30); 1942-09-01 00:00..01:00
  // does not exist.
  li = kol->get_info(local_days{1942y / May / 14} + 23h + 30min);
  CHECK(li.result == local_info::ambiguous && li.first.abbrev == "+0630" && li.second.abbrev == "IST");
  CHECK(kol->to_sys(local_days{1942y / May / 14} + 23h + 30min, choose::earliest) == sys_days{1942y / May / 14} + 17h);
  CHECK(kol->to_sys(local_days{1942y / May / 14} + 23h + 30min, choose::latest) == sys_days{1942y / May / 14} + 18h);
  li = kol->get_info(local_days{1942y / September / 1} + 30min);
  CHECK(li.result == local_info::nonexistent && li.first.abbrev == "IST" && li.second.abbrev == "+0630");
  CHECK(kol->to_sys(local_days{1942y / September / 1} + 30min, choose::latest) == sys_days{1942y / August / 31} + 18h + 30min);
  CHECK(std::format("{:%F %T %Z %Ez}", zoned_time{kol, sys_days{1943y / January / 1}}) ==
        "1943-01-01 06:30:00 +0630 +06:30");

  // Far future: the US rules in 2400 (a leap year divisible by 400) and sub-second inputs.
  const time_zone* ny = locate_zone("America/New_York");
  s = ny->get_info(sys_days{2400y / July / 1});
  CHECK(s.begin == sys_days{2400y / March / 12} + 7h && s.end == sys_days{2400y / November / 5} + 6h);
  CHECK(s.abbrev == "EDT" && s.save == 60min);
  CHECK(ny->get_info(sys_time<nanoseconds>{sys_days{2100y / March / 14} + 7h - 1ns}).abbrev == "EST");
  CHECK(ny->to_sys(local_time<milliseconds>{local_days{2100y / November / 7} + 1h + 500ms}, choose::latest) ==
        sys_time<milliseconds>{sys_days{2100y / November / 7} + 6h + 500ms});
  return 0;
}
