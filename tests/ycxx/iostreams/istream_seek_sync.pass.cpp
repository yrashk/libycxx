// [istream.unformatted]/39: sync(): "if rdbuf() is a null pointer, returns -1. Otherwise, calls
// rdbuf()->pubsync() and, if that function returns -1 calls setstate(badbit) ..., and returns
// -1. Otherwise, returns zero"; it does not affect gcount(). /40-41: tellg(): "if fail() !=
// false, returns pos_type(-1)"; otherwise rdbuf()->pubseekoff(0, cur, in). /42-45: seekg
// "first clears eofbit", does not affect gcount(); "if fail() != true" calls pubseekpos(pos, in)
// / pubseekoff(off, dir, in); "In case of failure, the function calls setstate(failbit)".
#include <istream>
#include <sstream>
#include <streambuf>
#include "check.hpp"

struct Recorder : std::streambuf {
  int syncs = 0;
  int sync_result = 0;
  int seekoffs = 0, seekposs = 0;
  std::ios_base::openmode last_which{};
  std::ios_base::seekdir last_dir{};
  char data[4] = {'a', 'b', 'c', 'd'};
  Recorder() { setg(data, data, data + 4); }
  int sync() override { ++syncs; return sync_result; }
  pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override {
    ++seekoffs;
    last_dir = dir;
    last_which = which;
    return off == 99 ? pos_type(off_type(-1)) : pos_type(off + 1000);
  }
  pos_type seekpos(pos_type p, std::ios_base::openmode which) override {
    ++seekposs;
    last_which = which;
    return off_type(p) == 99 ? pos_type(off_type(-1)) : p;
  }
};

int main() {
  {
    Recorder r;
    std::istream is(&r);
    is.ignore(2);
    CHECK(is.gcount() == 2);
    CHECK(is.sync() == 0 && r.syncs == 1 && is.good());
    CHECK(is.gcount() == 2);  // not affected
    r.sync_result = -1;
    CHECK(is.sync() == -1 && is.bad());
  }
  {
    std::istream is(nullptr);
    CHECK(is.sync() == -1);
  }
  {
    Recorder r;
    std::istream is(&r);
    CHECK(is.tellg() == std::streampos(1000));
    CHECK(r.seekoffs == 1 && r.last_dir == std::ios_base::cur && r.last_which == std::ios_base::in);
    is.seekg(5, std::ios_base::beg);
    CHECK(r.seekoffs == 2 && r.last_dir == std::ios_base::beg && r.last_which == std::ios_base::in);
    CHECK(is.good());
    is.seekg(std::streampos(3));
    CHECK(r.seekposs == 1 && r.last_which == std::ios_base::in && is.good());
    // a failing seek sets failbit
    is.seekg(99, std::ios_base::cur);
    CHECK(is.fail() && !is.bad());
    // with fail() set: no call, tellg returns -1
    int before = r.seekoffs;
    CHECK(is.tellg() == std::streampos(std::streamoff(-1)));
    is.seekg(1, std::ios_base::beg);
    is.seekg(std::streampos(1));
    CHECK(r.seekoffs == before && r.seekposs == 1);
    is.clear();
    is.seekg(std::streampos(99));
    CHECK(is.fail());
  }
  {
    // seekg clears eofbit first, and leaves gcount() alone
    std::istringstream is("abc");
    char buf[8];
    is.read(buf, 8);
    CHECK(is.eof() && is.fail() && is.gcount() == 3);
    is.clear(std::ios_base::eofbit);
    is.seekg(1);
    CHECK(is.good() && is.gcount() == 3);
    CHECK(is.get() == 'b');
    is.ignore(10);
    CHECK(is.eof() && !is.fail());
    is.seekg(0, std::ios_base::beg);
    CHECK(is.good() && is.tellg() == std::streampos(0));
    CHECK(is.get() == 'a');
    // a seek before the beginning fails
    is.seekg(-5, std::ios_base::cur);
    CHECK(is.fail());
    CHECK(is.tellg() == std::streampos(std::streamoff(-1)));
    // tellg with only eofbit set: the sentry finds good() false and sets failbit
    // ([istream.sentry]/2), so fail() is true and tellg returns pos_type(-1)
    std::istringstream t("ab");
    t.ignore(5);
    CHECK(t.eof() && !t.fail());
    CHECK(t.tellg() == std::streampos(std::streamoff(-1)));
    CHECK(t.fail());
  }
  return 0;
}
