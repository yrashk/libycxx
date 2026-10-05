// [time.format]/15: formatting a local_time with %Z, %z or a modified %z throws format_error
// (a local_time carries no zone). /16: local_time_format(time, abbrev, offset_sec) returns
// {time, abbrev, offset_sec}; /18: with chrono-specs omitted it formats as %F %T %Z; %Z is
// *abbrev_ and %z the offset *offset-sec, and either throws format_error when its pointer is
// null. /5: the zone information appears only when requested. (Checked at run time through
// vformat, [format.err.report]/1.)
// REQUIRES: exceptions
#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class T>
bool throws(std::string_view fmt, const T& v) {
  try {
    (void)std::vformat(fmt, std::make_format_args(v));
  } catch (const std::format_error&) {
    return true;
  }
  return false;
}

int main() {
  const local_seconds lt = local_days{2021y / July / 4} + 8h + 5min;
  CHECK(std::format("{}", lt) == "2021-07-04 08:05:00");  // no zone information (/5)
  CHECK(std::format("{:%F %R}", lt) == "2021-07-04 08:05");
  CHECK(throws("{:%Z}", lt) && throws("{:%z}", lt) && throws("{:%Ez}", lt) && throws("{:%Oz}", lt));
  CHECK(!throws("{:%T}", lt));

  const std::string abbrev = "XYZ";
  const seconds off = -4h - 30min;
  const auto full = local_time_format(lt, &abbrev, &off);
  CHECK(std::format("{}", full) == "2021-07-04 08:05:00 XYZ");
  CHECK(std::format("{:%T %Z %z %Ez}", full) == "08:05:00 XYZ -0430 -04:30");
  const auto no_off = local_time_format(lt, &abbrev);
  CHECK(std::format("{:%Z}", no_off) == "XYZ" && throws("{:%z}", no_off) && !throws("{}", no_off));
  const auto none = local_time_format(lt);
  CHECK(throws("{}", none) && throws("{:%Z}", none) && throws("{:%Ez}", none));
  CHECK(std::format("{:%F}", none) == "2021-07-04");
  const seconds zero = 0s;
  CHECK(std::format("{:%z}", local_time_format(lt, nullptr, &zero)) == "+0000");
  const auto ms = local_time_format(local_time<milliseconds>{lt + 7ms}, &abbrev, &off);
  CHECK(std::format("{}", ms) == "2021-07-04 08:05:00.007 XYZ");
  return 0;
}
