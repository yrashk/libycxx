// Stream buffers for the bulk-transfer tests (iostreams/streambuf_bulk_transfer*.pass.cpp): a source that
// serves a string through a get area of `chunk` characters (refilled by underflow), and a sink
// with an optional put area that counts its overflow and xsputn calls and can refuse or throw.
#pragma once

#include <streambuf>
#include <string>

struct chunked_source : std::streambuf {
  std::string data;
  std::size_t pos = 0, chunk;
  char buf[256];
  int underflows = 0, uflows = 0;
  chunked_source(std::string d, std::size_t c) : data(static_cast<std::string&&>(d)), chunk(c) {}
  virtual void before_underflow(int /*call, 0-based*/) {} // a test's hook (it may throw)
  int_type underflow() override {
    before_underflow(underflows++);
    if (gptr() < egptr())
      return traits_type::to_int_type(*gptr());
    if (pos == data.size())
      return traits_type::eof();
    std::size_t n = data.size() - pos < chunk ? data.size() - pos : chunk;
    data.copy(buf, n, pos);
    pos += n;
    setg(buf, buf, buf + n);
    return traits_type::to_int_type(*gptr());
  }
  int_type uflow() override {
    ++uflows;
    return std::streambuf::uflow();
  }
  // the characters not yet extracted
  std::string rest() const { return std::string(gptr(), egptr()) + data.substr(pos); }
};

struct counting_sink : std::streambuf {
  std::string out;
  std::size_t area;      // put area size; 0: none (every character is an overflow)
  std::size_t limit;     // accepts at most this many characters in all
  bool bulk;             // xsputn appends a run at once (as an unbuffered file does)
  int overflows = 0, xsputns = 0;
  char buf[256];
  explicit counting_sink(std::size_t a = 0, bool b = true, std::size_t lim = std::string::npos)
      : area(a), limit(lim), bulk(b) {
    if (area)
      setp(buf, buf + area);
  }
  virtual void before_overflow(int /*call, 0-based*/) {} // a test's hook (it may throw)
  std::size_t total() const { return out.size() + static_cast<std::size_t>(pptr() - pbase()); }
  std::string str() const { return out + std::string(pbase(), pptr()); }
  int_type overflow(int_type c) override {
    before_overflow(overflows++);
    out.append(pbase(), pptr());
    if (area)
      setp(buf, buf + area);
    if (traits_type::eq_int_type(c, traits_type::eof()))
      return traits_type::not_eof(c);
    if (out.size() >= limit)
      return traits_type::eof();
    if (area)
      *pptr() = traits_type::to_char_type(c), pbump(1);
    else
      out += traits_type::to_char_type(c);
    return c;
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    ++xsputns;
    if (!bulk || area)
      return std::streambuf::xsputn(s, n);
    std::size_t k = static_cast<std::size_t>(n);
    if (k > limit - out.size())
      k = limit - out.size();
    out.append(s, k);
    return static_cast<std::streamsize>(k);
  }
};
