// [time.format]/3 with L: a named locale's representation of %c (en_US.UTF-8's is
// "%a %d %b %Y %r %Z" in glibc, with a 12-hour time; Darwin's has a 24-hour time) shows the zone of the formatted value: UTC for a sys_time, the
// abbreviation of a local-time-format-t, and none for a local_time, which has no zone (never the
// process's own time zone, which the C library's strftime would otherwise write).
#include <chrono>
#include <format>
#include <langinfo.h>
#include <locale>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

using namespace std::chrono;

// The time of day as the locale's %c writes it: 12-hour ("03:00:00 PM") or 24-hour ("15:00:00").
static bool has_time(const std::string& s, const char* h12, const char* h24) {
  return s.find(h12) != std::string::npos || s.find(h24) != std::string::npos;
}

int main() {
  const char* name = require_locale("en_US.UTF-8");
  const std::locale loc(name);
  // Whether the locale's %c shows a zone at all: glibc's en_US has %Z, Darwin's has none. Without
  // one, what is checked is that no zone appears.
  const bool zoned = in_c_locale(name, [] { return std::string(nl_langinfo(D_T_FMT)).find("%Z") != std::string::npos; });
  const sys_seconds st = sys_days(2025y / March / 19) + 15h;
  std::string s = std::format(loc, "{:L%c}", st);
  CHECK(has_time(s, "03:00:00 PM", "15:00:00"));
  CHECK((s.find("UTC") != std::string::npos) == zoned);

  const local_seconds lt = local_days(2025y / March / 19) + 11h;
  s = std::format(loc, "{:L%c}", lt);
  CHECK(has_time(s, "11:00:00 AM", "11:00:00"));
  CHECK(s.find("UTC") == std::string::npos && s.find("GMT") == std::string::npos);

  const std::string abbrev = "XYZ";
  const seconds off = 2h;
  s = std::format(loc, "{:L%c}", local_time_format(lt, &abbrev, &off));
  CHECK((s.find("XYZ") != std::string::npos) == zoned);
  CHECK(s.find("UTC") == std::string::npos);
}
