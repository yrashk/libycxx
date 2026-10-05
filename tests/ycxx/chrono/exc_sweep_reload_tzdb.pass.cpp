// reload_tzdb() and remote_version() on an initialized time zone database while operator new
// fails at its k-th call, for every k until the call makes no allocation that fails.
//   [time.zone.db.remote]/2: reload_tzdb "first checks the version of the remote time zone
//     database. If the versions of the local and remote databases are the same, there are no
//     effects. Otherwise the remote database is pushed to the front of the tzdb_list"; /4:
//     "No pointers, references, or iterators are invalidated"; /5: "Returns:
//     get_tzdb_list().front()"; /6: "Throws: runtime_error if for any reason a reference
//     cannot be returned to a valid tzdb" (and bad_alloc for the allocation failure itself,
//     [res.on.exception.handling]); /7 remote_version(): "The latest remote database version".
//   [time.zone.db.list]/1: the tzdb_list is a singleton; /4 front() "A reference to the first
//     tzdb in the container".
// Nothing changes the installed database while the test runs, so the remote version is the
// local one (checked first): each call either throws, or returns front() with nothing else
// changed. Either way the list is unchanged afterwards (same front object, same number of
// databases, a zone pointer obtained before still valid and usable), nothing leaks (no
// effects), and the same call succeeds once memory is available.
// REQUIRES: exceptions
#include <chrono>
#include <iterator>
#include <stdexcept>
#include <string>
#include "exc_new.hpp"

using namespace exh;
namespace chr = std::chrono;

int main() {
  const chr::tzdb_list& list = chr::get_tzdb_list();
  const chr::tzdb& front = list.front();
  const std::string version = front.version;
  const chr::time_zone* zone = front.locate_zone("America/New_York");
  const auto offset = zone->get_info(chr::sys_days{chr::year{2024} / 1 / 15}).offset;
  const auto count = std::distance(list.begin(), list.end());
  if (chr::remote_version() != version) {
    dprintf(1, "the installed time zone database changed while the test started; nothing to check\n");
    return 0;
  }
  auto unchanged = [&](const char* what) {
    disarm();
    EXH_EXPECT(&chr::get_tzdb_list() == &list, "the tzdb_list singleton");
    EXH_EXPECT(&list.front() == &front, "front() is the same database");
    EXH_EXPECT(std::distance(list.begin(), list.end()) == count, "the number of databases");
    EXH_EXPECT(front.version == version, "the version");
    EXH_EXPECT(front.locate_zone("America/New_York") == zone, "a zone pointer");
    EXH_EXPECT(zone->get_info(chr::sys_days{chr::year{2024} / 1 / 15}).offset == offset, "the zone still works");
    (void)what;
  };
  long other_exceptions = 0;
  sweep_new("reload_tzdb", [&] {
    const chr::tzdb* got = nullptr;
    const bool threw = attempt([&] {
      try {
        got = &chr::reload_tzdb();
      } catch (const alloc_failure&) {
        throw;
      } catch (const std::runtime_error&) {
        if (!st.fired) ++other_exceptions;
        throw alloc_failure(gnew);
      }
    });
    const bool fired = st.fired;
    unchanged("reload_tzdb");
    if (!threw) EXH_EXPECT(got == &front, "reload_tzdb returns front()");
    return threw || fired;
  }, options{true, 4000});
  sweep_new("remote_version", [&] {
    std::string got;
    const bool threw = attempt([&] {
      try {
        got = chr::remote_version();
      } catch (const alloc_failure&) {
        throw;
      } catch (const std::runtime_error&) {
        if (!st.fired) ++other_exceptions;
        throw alloc_failure(gnew);
      }
    });
    const bool fired = st.fired;
    unchanged("remote_version");
    if (!threw) EXH_EXPECT(got == version, "remote_version");
    return threw || fired;
  }, options{true, 4000});
  EXH_EXPECT(other_exceptions == 0, "runtime_error without an allocation failure");
  return finish();
}
