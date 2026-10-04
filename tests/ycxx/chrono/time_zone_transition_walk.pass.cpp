// Walking the transitions of every zone with sys_info::end.
// [time.zone.info.sys]/3: "The begin and end data members indicate that, for the associated
// time_zone and time_point, the offset and abbrev are in effect in the range [begin, end).
// This information can be used to efficiently iterate the transitions of a time_zone."
// [time.zone.members]/3: get_info(st) returns the sys_info i with st in [i.begin, i.end).
// So for i = get_info(t): i.begin <= t < i.end, get_info(i.begin) and get_info(i.end - 1s)
// describe the same period, and get_info(i.end).begin == i.end (the walk has no gaps or
// overlaps). Iterating "the transitions" this way only visits transitions if every boundary
// is one: the period starting at i.end has a different offset, save or abbrev (otherwise the
// walk would report a transition where nothing changes; e.g. Africa/Casablanca keeps +01
// from 2087-05-11 02:00 UTC on, after its last listed change). [time.zone.info.sys]/4:
// offset = local_time - sys_time, so to_local(t) == t + offset ([time.zone.members]/9);
// [time.zone.info.local]/2: a local time within a period whose neighbours do not overlap it
// maps back (to_sys(to_local(t)) == t whenever get_info(local) is unique).
// [time.zone.db.tzdb]: zones is sorted by name.
#include <chrono>
#include <cstdio>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

int main() {
  const tzdb& db = get_tzdb();
  CHECK(!db.zones.empty());
  const sys_seconds from = sys_days{1850y / January / 1};
  const sys_seconds to = sys_days{2200y / January / 1};
  long transitions = 0;
  for (const time_zone& z : db.zones) {
    sys_seconds t = from;
    sys_info prev = z.get_info(t);
    CHECK(prev.begin <= t && t < prev.end);
    while (prev.end < to) {
      const sys_info i = z.get_info(prev.end);
      if (i.begin != prev.end)
        dprintf(2, "%s: gap/overlap at the end of a period\n", std::string(z.name()).c_str());
      CHECK(i.begin == prev.end);
      CHECK(i.begin < i.end);
      CHECK(i.offset > -24h && i.offset < 24h);
      const bool changes = i.offset != prev.offset || i.save != prev.save || i.abbrev != prev.abbrev;
      if (!changes)
        dprintf(2, "%s: boundary without a change at %lld\n", std::string(z.name()).c_str(),
                static_cast<long long>(i.begin.time_since_epoch().count()));
      CHECK(changes);
      // The same period seen from inside.
      const sys_info last = z.get_info(i.end - 1s);
      CHECK(i.end == to || i.end > to || (last.begin == i.begin && last.end == i.end));
      // A time in the middle of the period maps back and forth.
      const sys_seconds mid = i.begin + (i.end < to ? (i.end - i.begin) / 2 : 1h);
      const local_seconds lt = z.to_local(mid);
      CHECK(lt == local_seconds{mid.time_since_epoch()} + i.offset);
      const local_info li = z.get_info(lt);
      if (li.result == local_info::unique)
        CHECK(z.to_sys(lt) == mid);
      ++transitions;
      prev = i;
    }
  }
  CHECK(transitions > 10000);
  return 0;
}
