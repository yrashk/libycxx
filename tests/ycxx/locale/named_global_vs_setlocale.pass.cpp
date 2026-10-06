// [locale.statics]/1-3: locale::global(loc) makes future locale() calls return a copy of loc and
// returns the previous locale(); "No library function other than locale::global() affects the
// value returned by locale()": setlocale changes the C locale only.
// [basic.ios.cons] (Table: basic_ios::init effects): getloc() is locale() at the time the stream is
// constructed, so a stream keeps the global locale it was made with; [iostream.objects.overview]:
// the standard stream objects already exist, and locale::global does not imbue them.
#include <locale.h>
#include <string.h>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

int main() {
  const char* de = require_locale("de_DE.UTF-8");
  const char* fr = require_locale("fr_FR.UTF-8");

  // setlocale does not change locale()
  CHECK(setlocale(LC_ALL, de) != nullptr);
  CHECK(std::locale() == std::locale::classic());
  CHECK(std::locale().name() == "C");
  std::ostringstream before;
  before << 1.5;
  CHECK(before.str() == "1.5");
  CHECK(setlocale(LC_ALL, "C") != nullptr);

  std::ostringstream old_stream;
  const std::locale prev = std::locale::global(std::locale(de));
  CHECK(prev == std::locale::classic());
  CHECK(std::locale().name() == de);

  // a stream constructed now gets the new global locale; one constructed before keeps its own
  std::ostringstream fresh;
  CHECK(fresh.getloc() == std::locale(de));
  fresh << 1.5;
  CHECK(fresh.str() == "1,5");
  old_stream << 1.5;
  CHECK(old_stream.str() == "1.5");
  std::wostringstream wfresh;
  wfresh << 2.25;
  CHECK(wfresh.str() == L"2,25");

  // the standard streams are not imbued by locale::global
  CHECK(std::cout.getloc() == std::locale::classic());
  CHECK(std::wcout.getloc() == std::locale::classic());
  CHECK(std::wcerr.getloc() == std::locale::classic());

  // global returns the previous value of locale(), a copy that compares equal
  const std::locale p2 = std::locale::global(std::locale(fr));
  CHECK(p2 == std::locale(de));
  CHECK(p2.name() == de);
  CHECK(strcmp(setlocale(LC_ALL, nullptr), fr) == 0);
  // setlocale back to "C" leaves locale() at fr
  setlocale(LC_ALL, "C");
  CHECK(std::locale().name() == fr);
  CHECK(std::use_facet<std::numpunct<char>>(std::locale()).decimal_point() == ',');

  std::locale::global(std::locale::classic());
  CHECK(std::locale() == std::locale::classic());
}
