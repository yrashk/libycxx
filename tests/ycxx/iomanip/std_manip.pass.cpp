// [std.manip]: setw(n) sets width(n) for the next formatted operation (output resets it to 0);
// setfill(c), setprecision(n), setbase(base) (8 -> oct, 10 -> dec, 16 -> hex, anything else
// clears basefield), setiosflags(mask) / resetiosflags(mask); all usable with both input and
// output streams where meaningful. [basefield.manip], [floatfield.manip], [adjustfield.manip].
#include <iomanip>
#include <sstream>
#include <string>
#include "check.hpp"

int main() {
  std::ostringstream os;
  os << std::setw(5) << 42 << '|' << 42;
  CHECK(os.str() == "   42|42");

  std::ostringstream f;
  f << std::setfill('0') << std::setw(4) << 7 << ' ' << std::setw(3) << -1;
  CHECK(f.str() == "0007 0-1");

  std::ostringstream p;
  p << std::setprecision(3) << 3.14159 << ' ' << std::fixed << 2.0;
  CHECK(p.str() == "3.14 2.000");

  std::ostringstream b;
  b << std::setbase(16) << 255 << ' ' << std::setbase(8) << 8 << ' ' << std::setbase(10) << 10;
  CHECK(b.str() == "ff 10 10");
  b << std::setbase(3);
  CHECK((b.flags() & std::ios_base::basefield) == std::ios_base::fmtflags{});

  std::ostringstream fl;
  fl << std::setiosflags(std::ios_base::showbase | std::ios_base::hex) << 255;  // setf(mask): dec|hex
  fl << ' ' << std::resetiosflags(std::ios_base::basefield) << std::setiosflags(std::ios_base::hex) << 255;
  fl << ' ' << std::resetiosflags(std::ios_base::showbase) << 255;
  CHECK(fl.str() == "255 0xff ff");  // with both dec and hex set Table 97 selects %d

  std::ostringstream m;
  m << std::left << std::setw(4) << 1 << std::right << std::setw(4) << 2 << std::internal << std::setw(4) << -3;
  CHECK(m.str() == "1      2-  3");
  std::ostringstream sci;
  sci << std::scientific << std::setprecision(1) << 1500.0 << ' ' << std::hexfloat << 1.0 << ' '
      << std::defaultfloat << 1500.0;
  CHECK(sci.str() == "1.5e+03 0x1p+0 2e+03");
  std::ostringstream up;
  up << std::uppercase << std::hex << std::showbase << 255 << std::nouppercase << ' ' << 255
     << std::noshowbase << ' ' << std::showpos << std::dec << 1 << std::noshowpos << ' ' << 1
     << ' ' << std::showpoint << 1.0 << std::noshowpoint << ' ' << 1.0;
  CHECK(up.str() == "0XFF 0xff +1 1 1.00000 1");

  std::istringstream in("ff 12");
  int x, y;
  in >> std::setbase(16) >> x >> std::setbase(8) >> y;
  CHECK(x == 255 && y == 10);
  std::istringstream w("abcdef");
  std::string s;
  w >> std::setw(2) >> s;
  CHECK(s == "ab");
  return 0;
}
