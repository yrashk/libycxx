// [facet.num.put.virtuals]/6: bool: if boolalpha is not set, as an integer (1 / 0, so showpos
// and the base flags apply); otherwise numpunct truename() / falsename() ("true" / "false" in
// the C locale), padded like the other conversions. void* is printed with %p.
// [ostream.inserters.arithmetic]: operator<<(const void*) / operator<<(void*). [ostream.inserters]:
// operator<<(nullptr_t) inserts an implementation-defined NTCTS (only checked to compile).
#include <sstream>
#include <ios>
#include <cstdio>
#include <string>
#include "check.hpp"

int main() {
  std::ostringstream os;
  os << true << ' ' << false << ' ' << std::boolalpha << true << ' ' << false;
  CHECK(os.str() == "1 0 true false");

  std::ostringstream w;
  w << std::boolalpha << std::left;
  w.width(7);
  w.fill('.');
  w << true;
  CHECK(w.str() == "true...");
  std::ostringstream w2;
  w2 << std::boolalpha;
  w2.width(6);
  w2 << false;
  CHECK(w2.str() == " false");
  std::ostringstream sp;
  sp << std::showpos << true;
  CHECK(sp.str() == "+1");

  int x = 0;
  const void* p = &x;
  char expect[64];
  std::snprintf(expect, sizeof expect, "%p", p);
  std::ostringstream ps;
  ps << p;
  CHECK(ps.str() == expect);
  std::ostringstream ps2;
  ps2 << static_cast<void*>(&x);
  CHECK(ps2.str() == expect);
  std::ostringstream np;
  np << nullptr;
  CHECK(np.good());
  return 0;
}
