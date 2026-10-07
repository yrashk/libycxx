// [time.format]/3 with L: a named locale's representation of %c (en_US.UTF-8's is
// "%a %d %b %Y %r %Z" in glibc, with a 12-hour time; Darwin's has a 24-hour time) shows the zone of the formatted value: UTC for a sys_time, the
// abbreviation of a local-time-format-t, and none for a local_time, which has no zone (never the
// process's own time zone, which the C library's strftime would otherwise write).
#include <chrono>
#include <format>
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
  const std::locale loc(require_locale("en_US.UTF-8"));
  const sys_seconds st = sys_days(2025y / March / 19) + 15h;
  std::string s = std::format(loc, "{:L%c}", st);
  CHECK(has_time(s, "03:00:00 PM", "15:00:00"));
  CHECK(s.find("UTC") != std::string::npos);

  const local_seconds lt = local_days(2025y / March / 19) + 11h;
  s = std::format(loc, "{:L%c}", lt);
  CHECK(has_time(s, "11:00:00 AM", "11:00:00"));
  CHECK(s.find("UTC") == std::string::npos && s.find("GMT") == std::string::npos);

  const std::string abbrev = "XYZ";
  const seconds off = 2h;
  s = std::format(loc, "{:L%c}", local_time_format(lt, &abbrev, &off));
  CHECK(s.find("XYZ") != std::string::npos);
  CHECK(s.find("UTC") == std::string::npos);
}
