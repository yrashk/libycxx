// [ostream.unformatted]: put(c) and write(s, n) insert without padding (width is not used or
// reset); flush() calls rdbuf()->pubsync() and sets badbit when it returns -1;
// [ostream.seeks]: tellp / seekp; [ostream.manip]: endl inserts '\n' and flushes; ends inserts
// a null character; flush. [ostream.sentry]: a tied stream is flushed before output.
// [ostream.formatted.reqmts]: a failing streambuf makes the output set badbit.
#include <sstream>
#include <ostream>
#include <string>
#include "check.hpp"

struct SyncCount : std::stringbuf {
  int syncs = 0;
  int result = 0;
  int sync() override { ++syncs; return result; }
};

struct Full : std::streambuf {
  int_type overflow(int_type) override { return traits_type::eof(); }
};

int main() {
  std::ostringstream os;
  os.width(10);
  os.put('a').write("bcd", 3);
  CHECK(os.str() == "abcd");
  CHECK(os.width() == 10);  // unformatted output does not use or reset it
  CHECK(os.tellp() == std::streampos(4));
  os.seekp(1);
  os.put('X');
  CHECK(os.str() == "aXcd");

  SyncCount sc;
  std::ostream o(&sc);
  o << "x" << std::endl;
  CHECK(sc.str() == "x\n" && sc.syncs == 1);
  o << std::ends;
  CHECK(sc.str() == std::string("x\n\0", 3));
  o << std::flush;
  CHECK(sc.syncs == 2);
  sc.result = -1;
  o.flush();
  CHECK(o.bad());

  // tie: the tied stream is flushed before output on o2
  SyncCount tied;
  std::ostream t(&tied);
  std::ostringstream o2;
  o2.tie(&t);
  o2 << 1;
  CHECK(tied.syncs == 1);

  Full full;
  std::ostream f(&full);
  f << "data";
  CHECK(f.bad());
  std::ostream f2(&full);
  f2.put('c');
  CHECK(f2.bad());
  return 0;
}
