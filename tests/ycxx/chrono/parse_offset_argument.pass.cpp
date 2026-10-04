// The offset (and abbreviation) arguments of parse. [time.parse]/9-14: parse(fmt, tp, abbrev,
// offset) calls from_stream(is, fmt, tp, addressof(abbrev), &offset).
// [time.clock.system.nonmembers]/3, [time.clock.utc.nonmembers]/3, [time.clock.tai.nonmembers],
// [time.clock.gps.nonmembers], [time.clock.file.nonmembers]: "If %z (or a modified variant) is
// used and successfully parsed, that value will be assigned to *offset if offset is non-null.
// Additionally, the parsed offset will be subtracted from the successfully parsed timestamp";
// "If %Z is used and successfully parsed, that value will be assigned to *abbrev". So without
// %z / %Z in the format the arguments are neither read nor written, and the time point is not
// adjusted by the incoming value of *offset. [time.clock.local]/5: for local_time the parsed
// offset is assigned but not subtracted; [time.cal.ymd.nonmembers] from_stream likewise
// assigns only. [time.parse] Table 134: %z is [+|-]hh[mm], %Ez [+|-]h[h][:mm].
#include <chrono>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class TP>
bool parses(const char* fmt, const char* in, TP& tp, std::string& abbrev, minutes& off) {
  std::istringstream is(in);
  is >> parse(fmt, tp, abbrev, off);
  return !is.fail();
}

int main() {
  const sys_seconds noon = sys_days{2021y / March / 4} + 12h;
  {
    sys_seconds tp{};
    std::string abbrev = "keep";
    minutes off = 90min;
    CHECK(parses("%F %T", "2021-03-04 12:00:00", tp, abbrev, off));
    CHECK(tp == noon && off == 90min && abbrev == "keep");
    CHECK(parses("%F %T %z", "2021-03-04 12:00:00 -0130", tp, abbrev, off));
    CHECK(tp == noon + 1h + 30min && off == -90min && abbrev == "keep");
    CHECK(parses("%F %T %Z", "2021-03-04 12:00:00 XYZ", tp, abbrev, off));
    CHECK(tp == noon && off == -90min && abbrev == "XYZ");
    CHECK(parses("%F %T %Ez %Z", "2021-03-04 12:00:00 +5 PLUS5", tp, abbrev, off));
    CHECK(tp == noon - 5h && off == 300min && abbrev == "PLUS5");
  }
  {
    sys_time<milliseconds> tp{};
    std::string abbrev;
    minutes off = -60min;
    CHECK(parses("%F %T", "2021-03-04 12:00:00.250", tp, abbrev, off));
    CHECK(tp == noon + 250ms && off == -60min);
  }
  {
    utc_seconds tp{};
    std::string abbrev;
    minutes off = 45min;
    CHECK(parses("%F %T", "2021-03-04 12:00:00", tp, abbrev, off));
    CHECK(tp == utc_clock::from_sys(noon) && off == 45min);
    CHECK(parses("%F %T %z", "2021-03-04 12:00:00 +0100", tp, abbrev, off));
    CHECK(tp == utc_clock::from_sys(noon - 1h) && off == 60min);
  }
  {
    file_time<seconds> tp{};
    std::string abbrev;
    minutes off = 45min;
    CHECK(parses("%F %T", "2021-03-04 12:00:00", tp, abbrev, off));
    CHECK(clock_cast<system_clock>(tp) == noon && off == 45min);
  }
  {
    tai_seconds tp{};
    std::string abbrev;
    minutes off = 45min;
    CHECK(parses("%F %T", "2021-03-04 12:00:00", tp, abbrev, off));
    CHECK(tp == tai_seconds{sys_days{2021y / March / 4}.time_since_epoch() + 12h +
                            (sys_days{1970y / January / 1} - sys_days{1958y / January / 1})});
    CHECK(off == 45min);
  }
  {
    // local_time: assigned, not subtracted.
    local_seconds tp{};
    std::string abbrev;
    minutes off = 45min;
    CHECK(parses("%F %T", "2021-03-04 12:00:00", tp, abbrev, off));
    CHECK(tp == local_days{2021y / March / 4} + 12h && off == 45min);
    CHECK(parses("%F %T %z", "2021-03-04 12:00:00 +0530", tp, abbrev, off));
    CHECK(tp == local_days{2021y / March / 4} + 12h && off == 5h + 30min);
  }
  {
    year_month_day ymd{};
    std::string abbrev;
    minutes off = 45min;
    CHECK(parses("%F %z", "2021-03-04 -0200", ymd, abbrev, off));
    CHECK(ymd == 2021y / March / 4 && off == -120min);
  }
  return 0;
}
