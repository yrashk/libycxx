// [ostream.sentry]/2-3: if os.good(), prepares for output; "If os.tie() is not a null pointer,
// calls os.tie()->flush()"; the sentry converts to os.good() after preparation (/5).
// /4: "If (os.flags() & ios_base::unitbuf) && !uncaught_exceptions() && os.good() is true,
// calls os.rdbuf()->pubsync(). If that function returns -1 or exits via an exception, sets
// badbit in os.rdstate() without propagating an exception."
// [ostream.formatted.reqmts]/1, [ostream.unformatted]/1: no output is attempted when the sentry
// is false; "If an exception is thrown during output, then ios_base::badbit is set in *this's
// error state. If (exceptions() & badbit) != 0 then the exception is rethrown."
// REQUIRES: exceptions
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <type_traits>
#include "check.hpp"

using traits = std::char_traits<char>;

struct Sink : std::streambuf {
  char buf[64];
  std::string flushed;
  int syncs = 0;
  int sync_result = 0;
  bool throw_in_sync = false;
  bool throw_in_overflow = false;
  Sink() { setp(buf, buf + 64); }
  std::string pending() const { return std::string(pbase(), pptr()); }
  int sync() override {
    ++syncs;
    if (throw_in_sync) throw 1;
    flushed.append(pbase(), pptr());
    setp(buf, buf + 64);
    return sync_result;
  }
  int_type overflow(int_type c) override {
    if (throw_in_overflow) throw 2;
    return traits::eof() == c ? traits::not_eof(c) : traits::eof();
  }
};

struct Thrower {};
std::ostream& operator<<(std::ostream& os, Thrower) {
  std::ostream::sentry s(os);
  throw 3;
}

int main() {

  {
    // tie: the tied stream is flushed before output
    Sink a, b;
    std::ostream oa(&a), ob(&b);
    ob << "first";
    CHECK(b.syncs == 0 && b.pending() == "first");
    oa.tie(&ob);
    oa << 1;
    CHECK(b.flushed == "first" && b.syncs == 1);
    ob << "second";
    oa.put('x');  // unformatted output constructs a sentry too
    CHECK(b.flushed == "firstsecond");
    {
      std::ostream::sentry s(oa);
      CHECK(static_cast<bool>(s));
    }
  }
  {
    // unitbuf: pubsync after each output operation
    Sink a;
    std::ostream o(&a);
    o << std::unitbuf;
    int before = a.syncs;
    o << "ab";
    CHECK(a.syncs == before + 1 && a.flushed == "ab");
    o << 12;
    CHECK(a.syncs == before + 2 && a.flushed == "ab12");
    o.put('c');
    CHECK(a.syncs == before + 3 && a.flushed == "ab12c");
    o.write("de", 2);
    CHECK(a.syncs == before + 4 && a.flushed == "ab12cde");
    {
      std::ostream::sentry s(o);
      CHECK(a.syncs == before + 4);
    }
    CHECK(a.syncs == before + 5);
    o << std::nounitbuf;
    o << "f";
    CHECK(a.flushed == "ab12cde" && a.pending() == "f");
  }
  {
    // unitbuf with a failing pubsync: badbit, no exception even when badbit is in the mask
    Sink a;
    std::ostream o(&a);
    o.setf(std::ios_base::unitbuf);
    a.sync_result = -1;
    o << "x";
    CHECK(o.bad());
    Sink b;
    std::ostream p(&b);
    p.setf(std::ios_base::unitbuf);
    b.throw_in_sync = true;
    bool escaped = false;
    try {
      p << "y";
    } catch (...) {
      escaped = true;
    }
    CHECK(!escaped && p.bad());
  }
  {
    // a stream that is not good writes nothing
    Sink a;
    std::ostream o(&a);
    o.setstate(std::ios_base::failbit);
    o << "abc" << 12;
    o.put('x');
    o.write("yz", 2);
    CHECK(a.pending().empty());
    {
      std::ostream::sentry s(o);
      CHECK(!s);
    }
    // nor does the sentry destructor sync with unitbuf (os.good() is false)
    o.setf(std::ios_base::unitbuf);
    o << "q";
    CHECK(a.syncs == 0);
  }
  {
    // an exception during output: badbit, rethrown only if badbit is in exceptions()
    Sink a;
    a.throw_in_overflow = true;
    std::ostream o(&a);
    o << std::string(64, 'x');  // fills the put area
    CHECK(o.good());
    o << 'y';  // overflow throws
    CHECK(o.bad());
    Sink b;
    b.throw_in_overflow = true;
    std::ostream p(&b);
    p.exceptions(std::ios_base::badbit);
    p << std::string(64, 'x');
    int caught = 0;
    try {
      p.put('y');
    } catch (int e) {
      caught = e;
    } catch (...) {
      caught = -1;
    }
    CHECK(caught == 2 && p.bad());  // the original exception, not ios_base::failure
  }
  {
    // the sentry destructor does not sync while an exception is propagating
    Sink a;
    std::ostream o(&a);
    o.setf(std::ios_base::unitbuf);
    int before = a.syncs;
    try {
      o << Thrower{};
    } catch (int) {
    }
    CHECK(a.syncs == before);
  }
  return 0;
}
