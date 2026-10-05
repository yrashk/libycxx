// The first uses of the library's lazily or statically initialized state (global locale,
// classic locale and its facets, error categories, default memory resource, time zone
// database, text encoding, regex, format, random engines) happen in the constructor of a
// static object of another translation unit, during dynamic initialization before main
// ([basic.start.dynamic]/7: its order relative to the library's own initialization is
// unspecified, so the library must be usable either way). The constructor checks its results
// itself (support/linkage/static_init_use_tu.cpp); main then checks that what it set up is
// what the rest of the program sees:
//   [basic.start.dynamic]/? : the static object is initialized before main uses it (it is
//     odr-used by main through static_init_report);
//   [locale.statics]/1, /3: locale::global, called in that constructor, "Causes future calls
//     to the constructor locale() to return a copy of the argument", and "No library function
//     other than locale::global() affects the value returned by locale()";
//   [syserr.errcat.objects]/1, /3: generic_category() and system_category() return the same
//     object on every call; [time.zone.db.access]/2: get_tzdb() is the same database;
//   [fs.op.current.path]: the same working directory; [text.encoding.members]/14-15:
//     environment() is the same encoding ("not affected by calls to setlocale", which the
//     constructor's locale::global of an unnamed locale may or may not make).
// FILES: ../support/linkage/static_init_use_tu.cpp
#include "../support/linkage/static_init_use.hpp"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <locale>
#include <sstream>
#include "check.hpp"

int main() {
  const StaticInitReport& r = static_init_report();
  if (r.failures) dprintf(2, "failed during static initialization: %s\n", r.failed.c_str());
  CHECK(r.constructed);
  CHECK(r.failures == 0);
  // The global locale installed before main.
  std::locale here;
  CHECK(std::use_facet<std::numpunct<char>>(here).decimal_point() == ',');
  CHECK(here == static_init_global_locale_seen_there());
  CHECK(here != std::locale::classic());
  std::ostringstream os;
  os << 3.5;
  CHECK(os.str() == "3,5");
  CHECK(std::use_facet<std::numpunct<char>>(std::locale::classic()).decimal_point() == '.');
  // The same program-wide state.
  CHECK(std::chrono::get_tzdb().version == r.tz_version);
  CHECK(std::filesystem::current_path().native() == r.cwd);
  CHECK(std::text_encoding::environment() == r.env);
  return 0;
}
