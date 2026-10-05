// The first library uses of linkage/library_first_use_static_init, made by the static object's
// constructor in a member of a static archive linked into the program (the member is linked
// because main refers to it; its translation unit is then part of the program like any other,
// [lex.phases]/1.8, and its static objects are initialized before main uses them,
// [basic.start.dynamic]/7). The archive comes before the library's own archive on the link
// line, so its initialization may run before the library's: the library must be usable then.
// What the constructor sets up (the global locale) is what the whole program sees
// ([locale.statics]/1, /3), as are the time zone database, the working directory and the
// environment's text encoding.
// ARCHIVE: ../support/linkage/static_init_use_tu.cpp
// REQUIRES: exceptions
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
  std::locale here;
  CHECK(std::use_facet<std::numpunct<char>>(here).decimal_point() == ',');
  CHECK(here == static_init_global_locale_seen_there());
  std::ostringstream os;
  os << 0.5;
  CHECK(os.str() == "0,5");
  CHECK(std::chrono::get_tzdb().version == r.tz_version);
  CHECK(std::filesystem::current_path().native() == r.cwd);
  CHECK(std::text_encoding::environment() == r.env);
  return 0;
}
