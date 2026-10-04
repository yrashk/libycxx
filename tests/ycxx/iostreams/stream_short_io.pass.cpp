// [ostream.unformatted]/5 write(s, n): "Characters are inserted until either of the following
// occurs: n characters are inserted; inserting in the output sequence fails (in which case the
// function calls setstate(badbit))." [ostream.inserters.arithmetic]/1: "bool failed = use_facet
// <num_put<...>>(getloc()).put(*this, *this, fill(), val).failed();" ... "If failed is true
// then does setstate(badbit)"; [ostreambuf.iter.ops]: failed() is true if a previous sputc
// returned eof. [ostream.formatted.reqmts]/1: "If the generation fails, then the formatted
// output function does setstate(ios_base::failbit)". [ostream.sentry]: no output when !good().
// [istream.unformatted]: unget(): "If rdbuf() is null, or if rdbuf()->sungetc() returns
// traits::eof(), calls setstate(badbit)"; readsome(s, n): "If rdbuf()->in_avail() == -1, calls
// setstate(eofbit) and extracts no characters; If rdbuf()->in_avail() == 0, extracts no
// characters; If rdbuf()->in_avail() > 0, extracts min(rdbuf()->in_avail(), n)".
// [streambuf.virt.pback]/4: the default pbackfail returns traits::eof().
// [streambuf.pub.get] in_avail(): "If a read position is available, returns egptr() - gptr().
// Otherwise returns showmanyc()".
#include <streambuf>
#include <istream>
#include <ostream>
#include <string>
#include "check.hpp"

// Accepts `room` characters (no put area); both overflow and xsputn stop when it is full.
struct Limited : std::streambuf {
  std::string data;
  std::size_t room;
  explicit Limited(std::size_t r) : room(r) {}
  int_type overflow(int_type c) override {
    if (traits_type::eq_int_type(c, traits_type::eof())) return traits_type::not_eof(c);
    if (data.size() >= room) return traits_type::eof();
    data.push_back(traits_type::to_char_type(c));
    return c;
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    std::streamsize k = 0;
    while (k < n && data.size() < room) data.push_back(s[k++]);
    return k;
  }
};

// Serves the source one character per underflow; showmanyc reports -1 at the end.
struct OneAtATime : std::streambuf {
  std::string src;
  std::size_t pos = 0;
  char ch = 0;
  explicit OneAtATime(std::string s) : src(s) {}
  int_type underflow() override {
    if (pos == src.size()) return traits_type::eof();
    ch = src[pos++];
    setg(&ch, &ch, &ch + 1);
    return traits_type::to_int_type(ch);
  }
  std::streamsize showmanyc() override { return pos == src.size() ? -1 : 0; }
};

int main() {
  {
    Limited sb(5);
    std::ostream os(&sb);
    os.write("abcdefgh", 8);
    CHECK(os.bad() && sb.data == "abcde");
    os.clear();
    os.write("xyz", 3);
    CHECK(os.bad() && sb.data == "abcde");
  }
  {
    Limited sb(3);
    std::ostream os(&sb);
    os.width(8);
    os << 123456;
    CHECK(os.bad() && sb.data.size() == 3 && os.width() == 0);
    os.clear();
    os << 1;  // still full
    CHECK(os.bad());
  }
  {
    Limited sb(2);
    std::ostream os(&sb);
    os << "hello";
    CHECK(os.fail() && sb.data == "he");
    os << "x";  // sentry fails: nothing is attempted
    CHECK(sb.data == "he");
  }
  {
    Limited sb(4);
    std::ostream os(&sb);
    os.write("abcd", 4);  // exactly fits
    CHECK(os.good() && sb.data == "abcd");
    os.write("", 0);
    CHECK(os.good());
  }
  {
    OneAtATime sb("abcdef\ngh");
    std::istream is(&sb);
    CHECK(is.get() == 'a');
    CHECK(is.get() == 'b');
    CHECK(is.unget().good());   // 'b' is still behind gptr()
    CHECK(is.get() == 'b');
    CHECK(is.peek() == 'c');    // refill: 'b' is gone from the buffer
    CHECK(!is.unget().good() && is.bad());  // default pbackfail fails
    is.clear();
    CHECK(is.get() == 'c');
    char buf[8];
    CHECK(is.readsome(buf, 8) == 0 && is.good());  // nothing buffered, showmanyc() == 0
    CHECK(is.peek() == 'd');
    CHECK(is.readsome(buf, 8) == 1 && buf[0] == 'd');
    is.getline(buf, 4);  // "ef" then the delimiter
    CHECK(std::string(buf) == "ef" && is.gcount() == 3 && is.good());
    is.get(buf, 8, 'h');
    CHECK(std::string(buf) == "g" && is.peek() == 'h');
    CHECK(is.ignore(5).gcount() == 1 && is.eof());
    is.clear();
    CHECK(is.readsome(buf, 8) == 0 && is.eof());  // showmanyc() == -1
  }
  return 0;
}
