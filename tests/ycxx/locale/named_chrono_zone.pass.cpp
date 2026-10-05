// [time.format]/3 with L: a named locale's representation of %c (en_US.UTF-8's is
// "%a %d %b %Y %r %Z") shows the zone of the formatted value: UTC for a sys_time, the
// abbreviation of a local-time-format-t, and none for a local_time, which has no zone (never the
// process's own time zone, which the C library's strftime would otherwise write).
#include <chrono>
#include <format>
#include <locale>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

using namespace std::chrono;

int main() {
  const std::locale loc(require_locale("en_US.UTF-8"));
  const sys_seconds st = sys_days(2025y / March / 19) + 15h;
  std::string s = std::format(loc, "{:L%c}", st);
  CHECK(s.find("03:00:00 PM") != std::string::npos);
  CHECK(s.find("UTC") != std::string::npos);

  const local_seconds lt = local_days(2025y / March / 19) + 11h;
  s = std::format(loc, "{:L%c}", lt);
  CHECK(s.find("11:00:00 AM") != std::string::npos);
  CHECK(s.find("UTC") == std::string::npos && s.find("GMT") == std::string::npos);

  const std::string abbrev = "XYZ";
  const seconds off = 2h;
  s = std::format(loc, "{:L%c}", local_time_format(lt, &abbrev, &off));
  CHECK(s.find("XYZ") != std::string::npos);
  CHECK(s.find("UTC") == std::string::npos);
}
