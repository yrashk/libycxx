// [time.zone.zonedtime.overview]/2: "If Duration is not a specialization of chrono::duration,
// the program is ill-formed." The control uses seconds.
#include <chrono>

int main() {
  std::chrono::zoned_time<std::chrono::seconds> ok;  // control
  (void)ok;
#ifndef YCXX_CONTROL
  std::chrono::zoned_time<long> bad;
  (void)bad;
#endif
}
