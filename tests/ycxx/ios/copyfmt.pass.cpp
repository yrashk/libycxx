// [basic.ios.members]/16-17, Table 143: copyfmt copies tie, exceptions, flags, width,
// precision, fill, locale and the iword/pword arrays, not rdbuf or rdstate; registered
// callbacks are called with erase_event before and copyfmt_event after the copy; the exception
// mask is copied last ("then, calls exceptions(rhs.exceptions())").
// REQUIRES: exceptions
#include <ios>
#include <sstream>
#include <vector>
#include "check.hpp"

static std::vector<std::ios_base::event> events;
static void cb(std::ios_base::event ev, std::ios_base&, int idx) {
  if (idx == 7) events.push_back(ev);
}

int main() {
  std::ostringstream a, b, tie_target;
  int idx = std::ios_base::xalloc();
  a.flags(std::ios_base::hex | std::ios_base::showbase);
  a.width(9);
  a.precision(2);
  a.fill('#');
  a.tie(&tie_target);
  a.iword(idx) = 42;
  a.setstate(std::ios_base::eofbit);

  b.register_callback(cb, 7);
  auto* old_buf = b.rdbuf();
  b.copyfmt(a);
  CHECK(b.flags() == (std::ios_base::hex | std::ios_base::showbase));
  CHECK(b.width() == 9 && b.precision() == 2 && b.fill() == '#');
  CHECK(b.tie() == &tie_target);
  CHECK(b.iword(idx) == 42);
  CHECK(b.rdbuf() == old_buf);
  CHECK(b.good());  // rdstate unchanged
  CHECK(events.size() == 1 && events[0] == std::ios_base::erase_event);  // b's callback erased

  // a callback registered on the source is copied and called with copyfmt_event
  std::ostringstream c, d;
  events.clear();
  c.register_callback(cb, 7);
  d.copyfmt(c);
  CHECK(events.size() == 1 && events[0] == std::ios_base::copyfmt_event);

  // self copy: no effect
  b.copyfmt(b);
  CHECK(b.width() == 9);

  // exceptions copied last: rdstate of the target matching throws
  std::ostringstream src, dst;
  src.exceptions(std::ios_base::failbit);
  dst.setstate(std::ios_base::failbit);
  bool threw = false;
  try {
    dst.copyfmt(src);
  } catch (const std::ios_base::failure&) {
    threw = true;
  }
  CHECK(threw);
  CHECK(dst.exceptions() == std::ios_base::failbit);
  return 0;
}
