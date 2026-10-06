// [fmtflags.manip]/1-29, [adjustfield.manip]/1-7, [basefield.manip]/1-7, [floatfield.manip]/1-10:
// each manipulator is a designated addressable function (/1: its address may be taken) of type
// ios_base&(ios_base&) that calls str.setf(flag), str.unsetf(flag) or str.setf(flag, field) and
// returns str. So exactly that flag (or field) changes and nothing else: the other format flags,
// width(), precision() and the stream state are untouched, for narrow and wide streams alike.
// hexfloat sets fixed | scientific in floatfield (Note 1: hex is not a floating-point format)
// and defaultfloat clears floatfield.
#include <ios>
#include <sstream>
#include <type_traits>
#include "check.hpp"

using std::ios_base;
using Manip = ios_base& (*)(ios_base&);
using F = ios_base::fmtflags;

struct Case {
  Manip m;
  F set;   // bits that must be set afterwards
  F clear; // bits that must be clear afterwards
  F field; // the bits the call may change (the flag, or the whole field)
};

const Case cases[] = {
    {std::boolalpha, ios_base::boolalpha, F(), ios_base::boolalpha},
    {std::noboolalpha, F(), ios_base::boolalpha, ios_base::boolalpha},
    {std::showbase, ios_base::showbase, F(), ios_base::showbase},
    {std::noshowbase, F(), ios_base::showbase, ios_base::showbase},
    {std::showpoint, ios_base::showpoint, F(), ios_base::showpoint},
    {std::noshowpoint, F(), ios_base::showpoint, ios_base::showpoint},
    {std::showpos, ios_base::showpos, F(), ios_base::showpos},
    {std::noshowpos, F(), ios_base::showpos, ios_base::showpos},
    {std::skipws, ios_base::skipws, F(), ios_base::skipws},
    {std::noskipws, F(), ios_base::skipws, ios_base::skipws},
    {std::uppercase, ios_base::uppercase, F(), ios_base::uppercase},
    {std::nouppercase, F(), ios_base::uppercase, ios_base::uppercase},
    {std::unitbuf, ios_base::unitbuf, F(), ios_base::unitbuf},
    {std::nounitbuf, F(), ios_base::unitbuf, ios_base::unitbuf},
    {std::internal, ios_base::internal, ios_base::left | ios_base::right, ios_base::adjustfield},
    {std::left, ios_base::left, ios_base::internal | ios_base::right, ios_base::adjustfield},
    {std::right, ios_base::right, ios_base::internal | ios_base::left, ios_base::adjustfield},
    {std::dec, ios_base::dec, ios_base::hex | ios_base::oct, ios_base::basefield},
    {std::hex, ios_base::hex, ios_base::dec | ios_base::oct, ios_base::basefield},
    {std::oct, ios_base::oct, ios_base::dec | ios_base::hex, ios_base::basefield},
    {std::fixed, ios_base::fixed, ios_base::scientific, ios_base::floatfield},
    {std::scientific, ios_base::scientific, ios_base::fixed, ios_base::floatfield},
    {std::hexfloat, ios_base::fixed | ios_base::scientific, F(), ios_base::floatfield},
    {std::defaultfloat, F(), ios_base::floatfield, ios_base::floatfield},
};

template <class Stream>
void check(Stream& s, F start) {
  for (const Case& c : cases) {
    s.flags(start);
    s.width(7);
    s.precision(3);
    s.setstate(ios_base::eofbit);
    ios_base& r = c.m(s);
    CHECK(&r == static_cast<ios_base*>(&s));
    F now = s.flags();
    CHECK((now & c.set) == c.set);
    CHECK((now & c.clear) == F());
    CHECK((now & ~c.field) == (start & ~c.field)); // nothing outside the field changed
    CHECK(s.width() == 7 && s.precision() == 3 && s.rdstate() == ios_base::eofbit);
    s.clear();
  }
}

int main() {
  static_assert(std::is_same_v<decltype(&std::hexfloat), Manip>);
  std::stringstream n;
  std::wstringstream w;
  const F none = F();
  const F all = ios_base::boolalpha | ios_base::showbase | ios_base::showpoint | ios_base::showpos |
                ios_base::skipws | ios_base::uppercase | ios_base::unitbuf | ios_base::adjustfield |
                ios_base::basefield | ios_base::floatfield;
  check(n, none);
  check(n, all);
  check(w, ios_base::hex | ios_base::left | ios_base::scientific | ios_base::uppercase);
  check(w, ios_base::dec | ios_base::fixed | ios_base::boolalpha);

  // Through operator<< (a pointer to the function) and through the manipulator itself.
  std::ostringstream o;
  o << std::hex << std::showbase << std::uppercase << 255 << ' ' << std::dec << std::noshowbase << 255;
  CHECK(o.str() == "0XFF 255");
  std::ostringstream f;
  f << std::hexfloat << 1.0 << ' ' << std::defaultfloat << 0.5;
  CHECK((f.flags() & ios_base::floatfield) == F() && f.str().substr(f.str().size() - 4) == " 0.5");
  CHECK(f.str().find('p') != std::string::npos); // hexfloat output has a binary exponent
  std::ostringstream h;
  h << std::hex << 2.5; // Note 1: hex does not affect floating-point output
  CHECK(h.str() == "2.5");
  Manip p = &std::boolalpha;
  std::ostringstream b;
  b << p << true << std::noboolalpha << ' ' << true;
  CHECK(b.str() == "true 1");
  return 0;
}
