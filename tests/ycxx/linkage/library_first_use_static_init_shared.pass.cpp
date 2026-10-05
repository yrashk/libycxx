// The same first uses as linkage/library_first_use_static_init, made by the constructor of a
// static object of a shared library the program links to (it is initialized before main, and
// before the program's own static objects in practice; the standard has no shared libraries,
// so the requirements checked are those of a single program: every library facility works
// during dynamic initialization, [basic.start.dynamic]/7 leaving its order relative to the
// library's own state unspecified). The shared library is linked through the same wrapper as
// the program, so with a static libycxx it has its own copy of the library: what it sets up
// there (its global locale) is then a property of that copy, and is checked through the
// library's own functions only.
// FLAGS: -fPIC
// SHARED: ../support/linkage/static_init_use_tu.cpp
#include "../support/linkage/static_init_use.hpp"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <locale>
#include "check.hpp"

int main() {
  const StaticInitReport& r = static_init_report();
  if (r.failures) dprintf(2, "failed during static initialization: %s\n", r.failed.c_str());
  CHECK(r.constructed);
  CHECK(r.failures == 0);
  CHECK(static_init_decimal_point_there() == ',');
  CHECK(std::chrono::get_tzdb().version == r.tz_version);
  CHECK(std::filesystem::current_path().native() == r.cwd);
  CHECK(std::text_encoding::environment() == r.env);
  return 0;
}
