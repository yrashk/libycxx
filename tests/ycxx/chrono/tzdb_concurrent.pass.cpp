// [time.zone.db.access]/2: get_tzdb_list(): "It is safe to call this function from multiple
// threads at one time", also for the first access, which initializes the database (/1);
// get_tzdb, locate_zone and current_zone go through it. [time.zone.db.list]/3: front() "is
// thread-safe with respect to reload_tzdb()"; [time.zone.db.remote]/3: reload_tzdb "is
// thread-safe with respect to get_tzdb_list().front() and get_tzdb_list().erase_after()", /4:
// no pointers, references or iterators are invalidated. The tzdb, time_zone and zoned_time
// objects are then used through const member functions only ([res.on.data.races]/3):
// time_zone::get_info, to_sys, to_local, name ([time.zone.members]), tzdb::locate_zone
// ([time.zone.db.tzdb]), zoned_time conversions and formatting ([time.zone.zonedtime],
// [time.format]), get_leap_second_info and utc_clock conversions ([time.clock.utc]).
// A child process starts its threads before anything touches the database, releases them
// together, and checks every result against fixed values; one thread also calls reload_tzdb()
// and remote_version() while the others use front(). Meant to be run under TSan.
// FLAGS: -pthread
#include <atomic>
#include <chrono>
#include <format>
#include <latch>
#include <string>
#include <thread>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"
#include "watchdog.hpp"

using namespace std::chrono;
constexpr int K = 4;

static bool use_db(int k, int r) {
  const tzdb& db = get_tzdb();
  if (db.zones.empty()) return false;
  const time_zone* berlin = locate_zone("Europe/Berlin");
  const time_zone* ny = db.locate_zone("America/New_York");
  if (berlin->name() != "Europe/Berlin" || ny->name() != "America/New_York") return false;
  if (locate_zone("UTC")->name() != "UTC" && locate_zone("UTC")->name() != "Etc/UTC") return false;
  (void)current_zone();
  const sys_seconds t = sys_days{2026y / July / (1 + (k + r) % 20)} + 12h;
  sys_info bi = berlin->get_info(t);
  if (bi.offset != 2h || bi.abbrev != "CEST" || !(bi.begin <= t && t < bi.end)) return false;
  sys_info ni = ny->get_info(sys_days{2026y / January / 15});
  if (ni.offset != -5h || ni.abbrev != "EST") return false;
  local_seconds lt = berlin->to_local(t);
  if (lt.time_since_epoch() - t.time_since_epoch() != 2h) return false;
  if (berlin->to_sys(lt) != t) return false;
  zoned_time<seconds> z(ny, t);
  if (z.get_local_time().time_since_epoch() - t.time_since_epoch() != -4h) return false;
  if (std::format("{:%Z %z}", z) != "EDT -0400") return false;
  zoned_time<seconds> zb("Europe/Berlin", z);
  if (zb.get_sys_time() != t) return false;
  // leap seconds
  auto u = utc_clock::from_sys(sys_days{2017y / January / 1});
  leap_second_info li = get_leap_second_info(u);
  if (li.elapsed != 27s || li.is_leap_second) return false;
  if (db.leap_seconds.size() < 27) return false;
  return true;
}

static int child() {
  std::latch go(K + 1);
  std::atomic<int> failures{0};
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      for (int r = 0; r < 30; ++r) {
        if (!use_db(k, r)) ++failures;
        const tzdb& front = get_tzdb_list().front();
        if (front.locate_zone("Asia/Tokyo")->get_info(sys_days{2026y / March / 1}).offset != 9h) ++failures;
      }
    });
  ts.emplace_back([&] {
    go.arrive_and_wait();
    for (int r = 0; r < 5; ++r) {
      const std::string v = remote_version();
      const tzdb& db = reload_tzdb();
      if (v.empty() || db.version != v || db.locate_zone("Europe/Berlin") == nullptr) ++failures;
    }
  });
  for (auto& t : ts) t.join();
  // nothing was invalidated: every tzdb in the list is still usable
  int n = 0;
  for (const tzdb& db : get_tzdb_list()) {
    ++n;
    if (db.locate_zone("Europe/Paris")->name() != "Europe/Paris") ++failures;
  }
  if (n < 1) ++failures;
  return failures == 0 ? 0 : 1;
}

int main(int argc, char** argv) {
  if (child_mode()) {
    watchdog(240);
    return child();
  }
  (void)argc;
  (void)argv;
  ChildResult r = run_self("tz");
  if (r.status != 0) dprintf(2, "child status %d, stderr:\n%s\n", r.status, r.err.c_str());
  CHECK(r.status == 0);
  CHECK(r.err.empty());
}
