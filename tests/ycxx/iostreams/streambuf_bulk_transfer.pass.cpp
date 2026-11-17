// [ostream.inserters]/7-9 (operator<<(basic_streambuf*)), [istream.extractors]/14-15
// (operator>>(basic_streambuf*)) and [istream.unformatted] ignore(n, delim): the characters
// buffered in a get area move as runs. The observable part: inserting into a buffer without a
// put area whose xsputn takes a run at once (an unbuffered file, the synchronized cout) costs one
// xsputn per run of the source's get area, not one overflow per character; extracting into a
// buffer with a put area costs no more overflows than its size requires. Edge cases checked:
// runs that cross get-area boundaries, a delimiter at the first, last and one-past-the-limit
// position of a get area, the limits n of ignore (0, 1, exact, unlimited), a sink that accepts
// part of a run or nothing (failbit only when nothing was inserted; the rest stays in the
// source), end of file (eofbit for the extractors), and a stream inserting its own buffer.
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include "bulk_streambufs.hpp"
#include "check.hpp"

static std::string pattern(std::size_t n) {
  std::string s(n, '\0');
  for (std::size_t i = 0; i < n; ++i)
    s[i] = static_cast<char>('a' + i % 26);
  return s;
}

static void insert_streambuf() {
  const std::string data = pattern(10000);
  { // one xsputn per get area of the source, no overflow
    chunked_source src(data, 64);
    counting_sink sink;
    std::ostream os(&sink);
    os << &src;
    CHECK(os.good());
    CHECK(sink.out == data);
    CHECK(sink.overflows == 0);                   // 10000 before
    CHECK(sink.xsputns <= (10000 + 63) / 64 + 1); // 0 before
    CHECK(src.uflows == 0);
  }
  { // a sink with a put area: the runs go through it; overflow only when it is full
    chunked_source src(data, 64);
    counting_sink sink(100, false);
    std::ostream os(&sink);
    os << &src;
    os.flush();
    CHECK(os.good() && sink.str() == data);
    CHECK(sink.overflows <= 10000 / 100 + 1);
  }
  { // a sink that takes part of a run: what it took is extracted, the rest stays
    chunked_source src(data, 64);
    counting_sink sink(0, true, 100);
    std::ostream os(&sink);
    os << &src;
    CHECK(!os.fail() && sink.out == data.substr(0, 100));
    CHECK(src.rest() == data.substr(100));
  }
  { // a sink that takes nothing: failbit
    chunked_source src(data, 64);
    counting_sink sink(0, true, 0);
    std::ostream os(&sink);
    os << &src;
    CHECK(os.fail() && !os.bad());
    CHECK(src.rest() == data);
  }
  { // an empty source: failbit
    chunked_source src("", 64);
    counting_sink sink;
    std::ostream os(&sink);
    os << &src;
    CHECK(os.fail() && !os.bad() && sink.out.empty());
  }
  { // a stringbuf source: its whole contents in one run
    std::stringbuf src(data);
    counting_sink sink;
    std::ostream os(&sink);
    os << &src;
    CHECK(os.good() && sink.out == data && sink.overflows == 0 && sink.xsputns == 1);
  }
  { // the stream's own buffer: one character at a time, as before
    std::stringstream ss("abc");
    ss << ss.rdbuf();
    CHECK(ss.str() == "abc");
  }
}

static void extract_streambuf() {
  const std::string data = pattern(10000);
  { // into a buffer without a put area: one xsputn per get area of the source
    chunked_source src(data, 64);
    std::istream is(&src);
    counting_sink sink;
    is >> &sink;
    CHECK(is.eof() && !is.fail() && is.gcount() == 10000);
    CHECK(sink.out == data);
    CHECK(sink.overflows == 0); // 10000 before
    CHECK(sink.xsputns <= (10000 + 63) / 64 + 1);
  }
  { // into a put area of 100: about one overflow per 100 characters, and runs bounded by its room
    chunked_source src(data, 64);
    std::istream is(&src);
    counting_sink sink(100, false);
    is >> &sink;
    CHECK(is.eof() && !is.fail() && is.gcount() == 10000);
    CHECK(sink.str() == data);
    CHECK(sink.overflows <= 10000 / 100 + 1);
  }
  { // a sink that stops after 150 (overflow returns eof): exactly 150 are extracted
    chunked_source src(data, 64);
    std::istream is(&src);
    counting_sink sink(50, false, 150);
    is >> &sink;
    CHECK(!is.fail() && !is.eof() && is.gcount() == 150);
    CHECK(sink.str() == data.substr(0, 150));
    CHECK(src.rest() == data.substr(150));
  }
  { // a bulk sink that takes part of a run
    chunked_source src(data, 64);
    std::istream is(&src);
    counting_sink sink(0, true, 100);
    is >> &sink;
    CHECK(!is.fail() && is.gcount() == 100 && sink.out == data.substr(0, 100));
    CHECK(src.rest() == data.substr(100));
  }
  { // nothing inserted: failbit
    chunked_source src(data, 64);
    std::istream is(&src);
    counting_sink sink(0, true, 0);
    is >> &sink;
    CHECK(is.fail() && is.gcount() == 0 && src.rest() == data);
  }
  { // an empty source: eofbit and failbit
    chunked_source src("", 64);
    std::istream is(&src);
    counting_sink sink;
    is >> &sink;
    CHECK(is.eof() && is.fail() && is.gcount() == 0);
  }
  { // the stream's own buffer
    std::stringstream ss("abc");
    ss >> ss.rdbuf();
    CHECK(ss.str() == "abc");
  }
}

// ignore over a source whose get areas hold `chunk` characters
static void check_ignore(std::size_t chunk, const std::string& data, std::streamsize n, int delim,
                         std::streamsize count, const std::string& rest, bool eof) {
  chunked_source src(data, chunk);
  std::istream is(&src);
  is.ignore(n, delim);
  CHECK(is.gcount() == count);
  CHECK(src.rest() == rest);
  CHECK(is.eof() == eof && !is.fail());
}

static void ignore() {
  const auto all = std::numeric_limits<std::streamsize>::max();
  std::string data = pattern(1000);
  data[63] = '\n';  // the last of the first get area of 64
  data[64] = '\n';  // the first of the second
  data[200] = '\n'; // inside the fourth
  const auto at = [&](std::size_t i) { return data.substr(i); };
  check_ignore(64, data, all, '\n', 64, at(64), false);
  check_ignore(64, data, 64, '\n', 64, at(64), false);
  check_ignore(64, data, 63, '\n', 63, at(63), false); // the limit just before the delimiter
  check_ignore(64, data.substr(64), all, '\n', 1, at(65), false);
  check_ignore(64, data.substr(65), all, '\n', 136, at(201), false);
  check_ignore(64, data.substr(65), 135, '\n', 135, at(200), false);
  check_ignore(64, data, 0, '\n', 0, data, false);
  check_ignore(64, data, 1, '\n', 1, at(1), false);
  check_ignore(64, data, 500, std::char_traits<char>::eof(), 500, at(500), false); // no delimiter
  check_ignore(64, data, all, std::char_traits<char>::eof(), 1000, "", true);
  check_ignore(64, data, 1000, std::char_traits<char>::eof(), 1000, "", false); // n reached first
  check_ignore(64, data, 1001, 'Z', 1000, "", true);
  check_ignore(1, data, all, '\n', 64, at(64), false);
  // a delimiter that is no character's int_type value never matches
  check_ignore(64, "ab\xff" "cd", all, -1, 5, "", true);
  check_ignore(64, "ab\xff" "cd", all, 0xff, 3, "cd", false);
  // ignore() and ignore(n) with the default delimiter
  {
    chunked_source src(data, 64);
    std::istream is(&src);
    is.ignore();
    CHECK(is.gcount() == 1 && src.rest() == at(1));
    is.ignore(300);
    CHECK(is.gcount() == 300 && src.rest() == at(301));
  }
  // the char_type overload
  {
    chunked_source src("x\xffy", 64);
    std::istream is(&src);
    is.ignore(10, '\xff');
    CHECK(is.gcount() == 2 && src.rest() == "y");
  }
}

int main() {
  insert_streambuf();
  extract_streambuf();
  ignore();
}
