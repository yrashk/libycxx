// [stringbuf.members]/3 (init-buf-ptrs, used by the constructors and str(s)): with out in the
// mode, pbase() points to buf.front(); "if ios_base::ate is set in mode, pptr() == pbase() +
// buf.size() is true, otherwise pptr() == pbase() is true". The mode app is not ate: a
// basic_stringbuf opened with out | app (without ate) starts writing at the beginning,
// overwriting the initial contents; with ate it starts at the end. [stringbuf.virtuals]/5,8:
// overflow appends when the put area is full, so writing past the old contents extends them.
// Also for str(s) on such a buffer, and for ostringstream / stringstream, whose constructors
// pass the mode on ([ostringstream.cons]/2: basic_stringbuf(s, which | ios_base::out)).
// COUNTERPART: libcxx:input.output/string.streams/stringbuf/stringbuf.virtuals/overflow.pass.cpp
#include <ios>
#include <sstream>
#include <string>
#include "check.hpp"

int main() {
  using std::ios_base;
  {
    std::stringbuf sb("abcdef", ios_base::out | ios_base::app);
    sb.sputc('X');
    sb.sputn("YZ", 2);
    CHECK(sb.str() == "XYZdef");
    sb.sputn("1234567", 7);  // past the end: overflow extends the sequence
    CHECK(sb.str() == "XYZ1234567");
  }
  {
    std::stringbuf sb("abc", ios_base::out | ios_base::app | ios_base::ate);
    sb.sputc('X');
    CHECK(sb.str() == "abcX");
  }
  {
    std::stringbuf sb(ios_base::out | ios_base::app);
    sb.str("hello");
    sb.sputc('J');
    CHECK(sb.str() == "Jello");
  }
  {
    std::ostringstream os("abc", ios_base::app);
    os << 'Q';
    CHECK(os.str() == "Qbc");
  }
  {
    std::ostringstream os("abc", ios_base::ate);
    os << 'Q';
    CHECK(os.str() == "abcQ");
  }
  {
    std::stringstream ss("abcd", ios_base::in | ios_base::out | ios_base::app);
    ss << "xy";
    CHECK(ss.str() == "xycd");
    std::string w;
    ss >> w;  // the input sequence still starts at the beginning
    CHECK(w == "xycd");
  }
}
