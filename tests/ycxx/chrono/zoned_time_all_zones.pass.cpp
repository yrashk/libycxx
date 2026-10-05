// Every time zone of the tzdb at pseudo-random instants between 1900 and 2100, plus the instants
// around each zone's transitions.
// [time.zone.info.sys]: get_info(tp) returns the sys_info with begin <= tp < end.
// [time.zone.members]: to_local(tp) is tp + get_info(tp).offset; to_sys(local) for a unique
// local_info gives back the sys time; for an ambiguous one, choose::earliest / choose::latest
// pick first / second ([time.zone.info.local]), one of which is the original instant.
// [time.zone.zonedtime.members]: get_local_time() == zone->to_local(get_sys_time()), get_info()
// == zone->get_info(get_sys_time()). [time.format]: formatting a zoned_time uses the local time
// with the abbreviation and offset of get_info(): %Z is the abbreviation, %z the offset as
// [+-]hhmm (+0000 for zero), %Ez and %Oz the same with a colon; %F %T of a zoned_time equals
// %F %T of its local time. (Offsets with a seconds part, as in some LMT periods, are skipped for
// %z: ISO 8601 gives only hours and minutes.)
#include <chrono>
#include <format>
#include <string>
#include "check.hpp"

using namespace std::chrono;

unsigned st = 12321u;
unsigned rnd(unsigned n) {
  st ^= st << 13;
  st ^= st >> 17;
  st ^= st << 5;
  return st % n;
}

std::string offset_str(seconds off, bool colon) {
  char sign = off < 0s ? '-' : '+';
  long s = off.count() < 0 ? -off.count() : off.count();
  long h = s / 3600, m = (s % 3600) / 60;
  return colon ? std::format("{}{:02}:{:02}", sign, h, m) : std::format("{}{:02}{:02}", sign, h, m);
}

void check_instant(const time_zone* z, sys_seconds t) {
  sys_info info = z->get_info(t);
  CHECK(info.begin <= t && t < info.end);
  local_seconds lt = z->to_local(t);
  CHECK(lt.time_since_epoch() == t.time_since_epoch() + info.offset);
  local_info li = z->get_info(lt);
  if (li.result == local_info::unique) {
    CHECK(z->to_sys(lt) == t);
    CHECK(li.first.offset == info.offset);
  } else {
    CHECK(li.result == local_info::ambiguous);  // it exists: t maps to it
    sys_seconds e = z->to_sys(lt, choose::earliest), l = z->to_sys(lt, choose::latest);
    CHECK(e <= l && (e == t || l == t));
  }
  zoned_time<seconds> zt(z, t);
  CHECK(zt.get_local_time() == lt && zt.get_sys_time() == t);
  CHECK(zt.get_info().begin == info.begin && zt.get_info().abbrev == info.abbrev);
  CHECK(std::format("{:%Z}", zt) == info.abbrev);
  if (info.offset.count() % 60 == 0) {
    CHECK(std::format("{:%z}", zt) == offset_str(info.offset, false));
    CHECK(std::format("{:%Ez|%Oz}", zt) == offset_str(info.offset, true) + "|" + offset_str(info.offset, true));
  }
  CHECK(std::format("{:%F %T}", zt) == std::format("{:%F %T}", lt));
}

int main() {
  const tzdb& db = get_tzdb();
  CHECK(!db.zones.empty());
  const sys_seconds lo = sys_days{1900y / January / 1};
  const sys_seconds hi = sys_days{2100y / January / 1};
  const long span = (hi - lo).count();
  int zones = 0;
  for (const time_zone& z : db.zones) {
    ++zones;
    for (int k = 0; k < 12; ++k) {
      long r = (static_cast<long>(rnd(1u << 30)) * 4) % span;
      check_instant(&z, lo + seconds(r));
    }
    // walk a few transitions from 1970: the instants just before and at each one
    sys_seconds t = sys_days{1970y / January / 1};
    for (int k = 0; k < 6; ++k) {
      sys_info i = z.get_info(t);
      if (i.end >= hi) break;
      check_instant(&z, i.end - 1s);
      check_instant(&z, i.end);
      CHECK(z.get_info(i.end).begin == i.end);
      t = i.end;
    }
  }
  CHECK(zones > 300);
  // the links resolve to zones with the same rules
  for (const time_zone_link& l : db.links) {
    const time_zone* target = db.locate_zone(l.target());
    CHECK(db.locate_zone(l.name()) == target);
  }
}
