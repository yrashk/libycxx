// [basic.ios.members]/15-16: copyfmt copies the contents of the iword/pword arrays (not the
// pointers) and the registered callbacks. The k-th allocation it makes fails, for every k: the
// failure propagates, nothing it allocated is leaked ([res.on.exception.handling]/3), and the
// destination's own arrays are left as they were.
// REQUIRES: exceptions
#include <ios>
#include <sstream>
#include "exc_new.hpp"

using namespace exh;

static void callback(std::ios_base::event, std::ios_base&, int) {}

int main() {
  std::ostringstream src;
  src.iword(5) = 42;
  src.pword(3) = &src;
  src.register_callback(callback, 0);
  sweep_new("copyfmt", [&] {
    std::ostringstream dst;
    dst.iword(1) = 7;
    const bool threw = attempt([&] { dst.copyfmt(src); });
    if (threw && dst.iword(1) != 7)
      report("a failed copyfmt changed the destination's iword array", __LINE__);
    if (!threw && (dst.iword(5) != 42 || dst.pword(3) != &src))
      report("copyfmt did not copy the iword/pword arrays", __LINE__);
    return threw;
  });
  return finish();
}
