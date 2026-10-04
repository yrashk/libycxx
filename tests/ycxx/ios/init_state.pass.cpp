// [basic.ios.cons] Table 142, basic_ios::init(sb): rdbuf() == sb, tie() == 0, rdstate() is
// goodbit if sb is not null, otherwise badbit; exceptions() goodbit; flags() skipws | dec;
// width() 0; precision() 6; fill() widen(' '). [fmtflags.state]: flags(f) / setf / unsetf /
// width(w) / precision(p) return the previous values.
#include <ios>
#include <ostream>
#include <sstream>
#include "check.hpp"

int main() {
  std::ostringstream os;
  CHECK(os.rdbuf() != nullptr);
  CHECK(os.tie() == nullptr);
  CHECK(os.rdstate() == std::ios_base::goodbit);
  CHECK(os.exceptions() == std::ios_base::goodbit);
  CHECK(os.flags() == (std::ios_base::skipws | std::ios_base::dec));
  CHECK(os.width() == 0);
  CHECK(os.precision() == 6);
  CHECK(os.fill() == ' ');

  std::ostream null(nullptr);
  CHECK(null.rdstate() == std::ios_base::badbit);
  CHECK(null.bad() && !null.good());
  std::wostringstream w;
  CHECK(w.fill() == L' ');

  // the setters return the previous value
  std::ios_base::fmtflags old = os.flags(std::ios_base::hex);
  CHECK(old == (std::ios_base::skipws | std::ios_base::dec));
  CHECK(os.flags() == std::ios_base::hex);
  CHECK(os.setf(std::ios_base::showbase) == std::ios_base::hex);
  CHECK(os.flags() == (std::ios_base::hex | std::ios_base::showbase));
  // setf(fl, mask): clears mask, then sets fl & mask
  os.setf(std::ios_base::oct | std::ios_base::left, std::ios_base::basefield);
  CHECK((os.flags() & std::ios_base::basefield) == std::ios_base::oct);
  CHECK(!(os.flags() & std::ios_base::left));
  os.unsetf(std::ios_base::showbase);
  CHECK(os.flags() == std::ios_base::oct);
  CHECK(os.width(7) == 0 && os.width() == 7);
  CHECK(os.precision(3) == 6 && os.precision() == 3);
  CHECK(os.fill('*') == ' ' && os.fill() == '*');

  // the masks
  using B = std::ios_base;
  CHECK(B::basefield == (B::dec | B::oct | B::hex));
  CHECK(B::adjustfield == (B::left | B::right | B::internal));
  CHECK(B::floatfield == (B::scientific | B::fixed));
  return 0;
}
