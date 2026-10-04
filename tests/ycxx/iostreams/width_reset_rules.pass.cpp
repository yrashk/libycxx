// Which operations use and reset width():
// [facet.num.put.virtuals] Stage 3: "str.width(0) is called." (all arithmetic and pointer
// inserters, [ostream.inserters.arithmetic]); [ostream.inserters.character]: "Calls
// os.width(0)" / "Calls width(0)"; [string.view.io]: "then calls os.width(0)".
// [ostream.unformatted] put / write and [ostream.inserters] operator<<(basic_streambuf*)
// ("Behaves as an unformatted output function") neither pad nor reset width.
// [istream.extractors]: operator>>(charT(&)[N]) stores at most min(size_t(width()), N) - 1
// characters when width() > 0 and "operator>> then calls width(0)"; operator>>(charT&) does not
// use width; [istream.formatted.arithmetic] / [facet.num.get.virtuals] do not use or reset
// width; [string.io] operator>>(basic_string) extracts at most width() characters when
// width() > 0 and calls is.width(0).
// [ostream.inserters.arithmetic]/1: short and int with oct or hex are converted through
// unsigned short / unsigned int: "static_cast<long>(static_cast<unsigned short>(val))".
#include <sstream>
#include <string>
#include <iomanip>
#include "check.hpp"

int main() {
  {
    std::ostringstream os;
    os.fill('.');
    os << std::setw(4) << 7 << '|' << std::setw(3) << 'c' << '|' << std::setw(4) << "ab" << '|'
       << std::setw(5) << std::string("xy") << '|' << std::setw(5) << 1.5 << '|';
    CHECK(os.str() == "...7|..c|..ab|...xy|..1.5|");
    CHECK(os.width() == 0);

    os.str("");
    os.width(5);
    os.put('a');
    os.write("bc", 2);
    CHECK(os.width() == 5);
    std::stringbuf src("SB");
    os << &src;  // unformatted: no padding, width kept
    CHECK(os.width() == 5);
    os << 1;
    CHECK(os.width() == 0);
    CHECK(os.str() == "abcSB....1");

    os.str("");
    int x = 0;
    os << std::setw(1) << static_cast<const void*>(&x);
    CHECK(os.width() == 0);
  }
  {
    std::ostringstream os;
    os << std::hex << static_cast<short>(-1) << ' ' << -1 << ' ' << -1L << ' ' << std::oct
       << static_cast<short>(-1);
    std::string allf(sizeof(long) * 2, 'f');
    CHECK(os.str() == "ffff ffffffff " + allf + " 177777");
  }
  {
    std::istringstream is("12345 abcdefgh ijklmnop q");
    is.width(3);
    int n = 0;
    is >> n;  // arithmetic extraction ignores width
    CHECK(n == 12345 && is.width() == 3);
    std::string s;
    is >> s;
    CHECK(s == "abc" && is.width() == 0);
    is >> s;
    CHECK(s == "defgh");
    char buf[4];
    is.width(10);
    is >> buf;  // min(width, N) - 1 == 3 characters
    CHECK(std::string(buf) == "ijk" && is.width() == 0);
    is.width(2);
    is >> buf;
    CHECK(std::string(buf) == "l" && is.width() == 0);
    char c = 0;
    is.width(7);
    is >> c;
    CHECK(c == 'm' && is.width() == 7);
    is >> s;
    CHECK(s == "nop" && is.width() == 0);
  }
  return 0;
}
