// [syncstream.syncbuf.members]/4: emit() returns true only if "(4.1) wrapped == nullptr is
// false" (and the transfer succeeded); [syncstream.syncbuf.cons]: a default syncbuf has
// wrapped == nullptr. [syncstream.osyncstream.members]/1: osyncstream::emit() "Behaves as an
// unformatted output function. After constructing a sentry object, calls sb.emit(). If that
// call returns false, calls setstate(ios_base::badbit)."
#include <syncstream>
#include <streambuf>
#include "check.hpp"

int main() {
  std::syncbuf sb;
  CHECK(sb.get_wrapped() == nullptr);
  CHECK(!sb.emit());

  std::osyncstream none(static_cast<std::streambuf*>(nullptr));
  CHECK(none.get_wrapped() == nullptr);
  none << "x";
  CHECK(none.good());
  none.emit();
  CHECK(none.bad());
  return 0;
}
