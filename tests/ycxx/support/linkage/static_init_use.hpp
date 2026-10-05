// Interface between linkage/library_first_use_static_init{,_shared}.pass.cpp and
// static_init_use_tu.cpp, whose static object's constructor makes the first library uses of
// the program (or of the shared library) during dynamic initialization, before main.
#pragma once
#include <locale>
#include <string>
#include <text_encoding>

struct __attribute__((visibility("default"))) StaticInitReport {
  int failures = 0;      // checks that failed inside the constructor
  std::string failed;    // their names
  std::string cwd;       // filesystem::current_path() as seen then
  std::text_encoding env{std::text_encoding::id::unknown};  // text_encoding::environment() then
  std::string tz_version;  // get_tzdb().version
  bool constructed = false;
};

// The report filled in by the static constructor (constructed before main returns it).
__attribute__((visibility("default"))) const StaticInitReport& static_init_report();
// The global locale the static constructor installed (a copy of classic() with a numpunct
// whose decimal point is ','), as seen by code compiled with that translation unit.
__attribute__((visibility("default"))) std::locale static_init_global_locale_seen_there();
// The decimal point of the global locale's numpunct<char> as seen there.
__attribute__((visibility("default"))) char static_init_decimal_point_there();
