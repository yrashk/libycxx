// Exceptions during the bulk transfers of operator<<(basic_streambuf*) and
// operator>>(basic_streambuf*) (streambuf_bulk_transfer.pass.cpp):
// - [ostream.inserters]/8-9: an exception from the output (the stream's own buffer) sets badbit
//   and is rethrown when badbit is in exceptions(); one while getting a character from sb sets
//   failbit and is rethrown only when failbit is in exceptions(). The characters before it have
//   been inserted.
// - [istream.extractors]/14: an exception from the output sequence sb ends the extraction and is
//   caught; with a put area in sb, the characters extracted (gcount) are exactly those inserted
//   before the throwing overflow; failbit only if there were none.
// REQUIRES: exceptions
#include <istream>
#include <ostream>
#include <string>
#include "bulk_streambufs.hpp"
#include "check.hpp"

struct throwing_sink : counting_sink {
  int at;
  throwing_sink(std::size_t area, int t, bool bulk = true) : counting_sink(area, bulk), at(t) {}
  void before_overflow(int call) override {
    if (call == at)
      throw 8;
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    if (at < 0 && total() + static_cast<std::size_t>(n) > 130) { // a bulk sink that throws
      out.append(s, 130 - total());
      throw 9;
    }
    return counting_sink::xsputn(s, n);
  }
};

struct throwing_source : chunked_source {
  int at;
  throwing_source(std::string d, std::size_t c, int t) : chunked_source(static_cast<std::string&&>(d), c), at(t) {}
  void before_underflow(int call) override {
    if (call == at)
      throw 7;
  }
};

static std::string pattern(std::size_t n) {
  std::string s(n, '\0');
  for (std::size_t i = 0; i < n; ++i)
    s[i] = static_cast<char>('a' + i % 26);
  return s;
}

int main() {
  const std::string data = pattern(1000);
  // ---- operator<<(basic_streambuf*) ----
  { // the output throws: badbit, not rethrown
    chunked_source src(data, 64);
    throwing_sink sink(0, -1);
    std::ostream os(&sink);
    os << &src;
    CHECK(os.bad() && sink.out == data.substr(0, 130));
  }
  { // rethrown with badbit in exceptions()
    chunked_source src(data, 64);
    throwing_sink sink(0, -1);
    std::ostream os(&sink);
    os.exceptions(std::ios_base::badbit);
    bool caught = false;
    try {
      os << &src;
    } catch (int e) {
      caught = e == 9;
    }
    CHECK(caught && os.bad());
  }
  { // the source throws on its third underflow: failbit, the first two get areas inserted
    throwing_source src(data, 64, 2);
    counting_sink sink;
    std::ostream os(&sink);
    os << &src;
    CHECK(os.fail() && !os.bad() && sink.out == data.substr(0, 128));
  }
  { // rethrown with failbit in exceptions()
    throwing_source src(data, 64, 2);
    counting_sink sink;
    std::ostream os(&sink);
    os.exceptions(std::ios_base::failbit);
    bool caught = false;
    try {
      os << &src;
    } catch (int e) {
      caught = e == 7;
    }
    CHECK(caught && os.fail() && sink.out == data.substr(0, 128));
  }
  // ---- operator>>(basic_streambuf*) ----
  { // a put area of 10; the third overflow throws: 30 inserted and extracted
    chunked_source src(data, 64);
    std::istream is(&src);
    throwing_sink sink(10, 2);
    is >> &sink;
    CHECK(!is.fail() && !is.bad() && is.gcount() == 30);
    CHECK(sink.str() == data.substr(0, 30));
    CHECK(src.rest() == data.substr(30));
  }
  { // the first overflow throws: the 10 that filled the put area are extracted
    chunked_source src(data, 64);
    std::istream is(&src);
    throwing_sink sink(10, 0);
    is >> &sink;
    CHECK(!is.fail() && is.gcount() == 10 && src.rest() == data.substr(10));
  }
  { // no put area, and the first character's overflow throws: nothing extracted, failbit
    chunked_source src(data, 64);
    std::istream is(&src);
    throwing_sink sink(0, 0, false);
    is >> &sink;
    CHECK(is.fail() && !is.bad() && is.gcount() == 0 && src.rest() == data);
  }
  { // not rethrown, whatever exceptions() holds
    chunked_source src(data, 64);
    std::istream is(&src);
    throwing_sink sink(10, 2);
    is.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    is >> &sink;
    CHECK(!is.bad() && is.gcount() == 30);
  }
}
