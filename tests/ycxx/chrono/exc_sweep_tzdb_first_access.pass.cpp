// The first access to the time zone database while operator new fails at its k-th call, for
// every k until the access succeeds; nothing else in the program touches the database before.
//   [time.zone.db.access]/1 get_tzdb_list: "If this is the first access to the time zone
//     database, initializes the database. If this call initializes the database, the resulting
//     database will be a tzdb_list holding a single initialized tzdb." "Throws: runtime_error if
//     for any reason a reference cannot be returned to a valid tzdb_list containing one or more
//     valid tzdbs." A failed first access therefore reports an exception (runtime_error, or
//     bad_alloc for the allocation failure itself) and initializes nothing: the next access is
//     again a first access, and it succeeds once memory is available; the database is then
//     complete (its zones and links can be located, the list holds exactly one tzdb).
//   /? get_tzdb() is get_tzdb_list().front(); locate_zone(name) is
//     get_tzdb().locate_zone(name); [time.zone.db.tzdb] locate_zone finds a zone or a link.
//   current_zone() after a failure likewise works once memory is available.
// The same then for the first call of current_zone() (if it initializes anything lazily).
#include <algorithm>
#include <chrono>
#include <exception>
#include <iterator>
#include <stdexcept>
#include <string>
#include "exc_new.hpp"

using namespace exh;
namespace chr = std::chrono;

static long other_exceptions = 0;

template <class F>
void sw(const char* name, F op) {
  // No leak accounting: the successful run creates the database, which stays.
  sweep(name, gnew, [&] {
    bool threw = attempt([&] {
      try {
        op();
      } catch (const alloc_failure&) {
        throw;
      } catch (const std::runtime_error&) {
        if (!st.fired) ++other_exceptions;
        throw alloc_failure(gnew);  // the failure reported as runtime_error
      }
    });
    return threw;
  });
}

int main() {
  sw("first get_tzdb", [] {
    const chr::tzdb& db = chr::get_tzdb();
    (void)db;
  });
  disarm();
  const chr::tzdb_list& list = chr::get_tzdb_list();
  CHECK(std::distance(list.begin(), list.end()) == 1);
  const chr::tzdb& db = chr::get_tzdb();
  CHECK(&db == &list.front());
  CHECK(!db.version.empty() && !db.zones.empty());
  CHECK(std::is_sorted(db.zones.begin(), db.zones.end(),
                       [](const chr::time_zone& a, const chr::time_zone& b) { return a.name() < b.name(); }));
  const chr::time_zone* berlin = chr::locate_zone("Europe/Berlin");
  CHECK(berlin != nullptr && berlin->name() == "Europe/Berlin");
  CHECK(berlin->get_info(chr::sys_days(chr::year(2024) / 7 / 1)).offset == chr::hours(2));
  CHECK(chr::locate_zone("Etc/UTC") != nullptr);
  for (const chr::time_zone& z : db.zones) CHECK(chr::locate_zone(z.name()) == &z);

  // A current_zone() that returns under the failure returns the local zone, not a fallback.
  const chr::time_zone* seen[4000] = {};
  int nseen = 0;
  sw("first current_zone", [&] {
    const chr::time_zone* z = chr::current_zone();
    if (nseen < 4000) seen[nseen++] = z;
  });
  disarm();
  const chr::time_zone* local = chr::current_zone();
  CHECK(local != nullptr);
  for (int i = 0; i < nseen; ++i) CHECK(seen[i] == local);
  CHECK(other_exceptions == 0);
  return finish();
}
