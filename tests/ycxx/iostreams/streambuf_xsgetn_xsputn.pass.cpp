// [streambuf.virt.get]/4-5: the default xsgetn assigns up to n characters "as if by repeated
// calls to sbumpc()", stopping when n are assigned or sbumpc() would return eof(); returns the
// number assigned. /16: the default uflow() calls underflow() and on success returns
// to_int_type(*gptr()) and increments gptr(). [streambuf.virt.put]/1-2: the default xsputn
// writes "as if by repeated calls to sputc(c)" and stops when sputc(c) would return eof().
// A derived buffer that only overrides underflow() (serving the source two characters at a
// time) and overflow() (accepting a limited number of characters through a 3-character put
// area) is read and written across refills with sgetn / sputn and through istream/ostream.
#include <streambuf>
#include <istream>
#include <ostream>
#include <string>
#include <cstring>
#include "check.hpp"

using traits = std::char_traits<char>;

struct Chunked : std::streambuf {
  std::string src;
  std::size_t pos = 0;
  char window[2];
  std::string sink;
  std::size_t capacity;
  char pbuf[3];
  int underflows = 0;

  Chunked(std::string s, std::size_t cap) : src(std::move(s)), capacity(cap) { setp(pbuf, pbuf + 3); }

  void drain() {
    sink.append(pbase(), pptr());
    setp(pbuf, pbuf + 3);
  }

protected:
  int_type underflow() override {
    ++underflows;
    if (gptr() != nullptr && gptr() < egptr()) return traits::to_int_type(*gptr());
    if (pos == src.size()) return traits::eof();
    std::size_t n = src.size() - pos < 2 ? src.size() - pos : 2;
    std::memcpy(window, src.data() + pos, n);
    pos += n;
    setg(window, window, window + n);
    return traits::to_int_type(window[0]);
  }
  int_type overflow(int_type c) override {
    // consume the whole pending sequence if it still fits in the capacity, otherwise fail
    std::size_t pending = static_cast<std::size_t>(pptr() - pbase()) + (traits::eq_int_type(c, traits::eof()) ? 0 : 1);
    if (sink.size() + pending > capacity) return traits::eof();
    drain();
    if (!traits::eq_int_type(c, traits::eof())) sink.push_back(traits::to_char_type(c));
    return traits::not_eof(c);
  }
  int sync() override {
    if (sink.size() + (pptr() - pbase()) > capacity) return -1;
    drain();
    return 0;
  }
};

int main() {
  {
    Chunked b("hello world", 0);
    char buf[16] = {};
    CHECK(b.sgetn(buf, 3) == 3 && std::string(buf, 3) == "hel");
    CHECK(b.sgetn(buf, 5) == 5 && std::string(buf, 5) == "lo wo");
    CHECK(b.sgetn(buf, 16) == 3 && std::string(buf, 3) == "rld");  // stops at eof
    CHECK(b.sgetn(buf, 4) == 0);
    CHECK(b.sbumpc() == traits::eof());
  }
  {
    // the default uflow: underflow() then *gptr() and gptr()++
    Chunked b("xyz", 0);
    CHECK(b.sbumpc() == 'x');
    CHECK(b.sbumpc() == 'y');
    CHECK(b.sbumpc() == 'z');
    CHECK(b.sbumpc() == traits::eof());
  }
  {
    Chunked b("", 8);
    CHECK(b.sputn("abcdefg", 7) == 7);
    CHECK(b.pubsync() == 0);
    CHECK(b.sink == "abcdefg");
    // only one more character fits: sputn reports how many were written
    std::streamsize n = b.sputn("XYZW", 4);
    CHECK(n >= 1 && n <= 4);
    b.pubsync();  // may fail if the put area holds more than fits
    CHECK(b.sink.size() <= 8);
  }
  {
    // exact count: capacity 5, put area of 3: "abc" fill the area, 'd' triggers overflow
    // which drains "abc" and appends 'd' (4), 'e' goes into the area, 'f' ... 'g' fill it,
    // the next overflow would exceed 5 characters.
    Chunked b("", 5);
    CHECK(b.sputc('a') == 'a' && b.sputc('b') == 'b' && b.sputc('c') == 'c');
    CHECK(b.sputc('d') == 'd' && b.sink == "abcd");
    CHECK(b.sputc('e') == 'e');
    CHECK(b.pubsync() == 0 && b.sink == "abcde");
    CHECK(b.sputc('f') == 'f');  // stored in the put area
    CHECK(b.pubsync() == -1);
  }
  {
    // through the streams
    Chunked b("12 34\nnext", 100);
    std::istream is(&b);
    int x = 0, y = 0;
    is >> x >> y;
    CHECK(x == 12 && y == 34);
    std::string line;
    is.ignore();
    std::getline(is, line);
    CHECK(line == "next" && is.eof());
    std::ostream os(&b);
    os << "value=" << 42 << '!' << std::flush;
    CHECK(b.sink == "value=42!");
  }
  return 0;
}
