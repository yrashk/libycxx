// Exception-injection sweep over the formatted and unformatted input and output functions of
// basic_istream/basic_ostream: the k-th call of an unbuffered streambuf's virtual function
// (underflow, uflow, xsgetn, showmanyc, pbackfail, overflow, xsputn, sync, seekoff, seekpos)
// throws, for every k until the operation completes, with exceptions() set to goodbit, badbit
// and failbit.
//   [istream.formatted.reqmts]/1, [istream.unformatted]/1: "If an exception is thrown during
//     input then ios_base::badbit is set in the local error state, *this's error state is set
//     to the local error state, and the exception is rethrown if (exceptions() & badbit) != 0."
//   [ostream.formatted.reqmts]/1, [ostream.unformatted]/1: "If an exception is thrown during
//     output, then ios_base::badbit is set in *this's error state. If (exceptions() & badbit)
//     != 0 then the exception is rethrown."
//   So: badbit is always set; with badbit in exceptions() the ORIGINAL exception propagates
//   (not an ios_base::failure); without it nothing propagates (an ios_base::failure is
//   acceptable only if a bit in exceptions() other than badbit is set in rdstate()).
//   [ostream.sentry]/4: ~sentry with unitbuf: "If that function [pubsync] returns -1 or exits
//     via an exception, sets badbit in os.rdstate() without propagating an exception."
//   tellg/seekg/sync ([istream.unformatted]/39-46: "Behaves as an unformatted input function")
//   are covered; ostream's seekp/tellp are not unformatted output functions ([ostream.seeks]).
// skipws is cleared and the input starts with a non-space so that no input happens inside
// the sentry constructor ([istream.sentry]/2 does not say what an exception there does).
// Every run also checks that operator new blocks are balanced.
#include <iomanip>
#include <ios>
#include <istream>
#include <ostream>
#include <streambuf>
#include <string>
#include "exc_new.hpp"

using namespace exh;

enum : unsigned { U = 1, UF = 2, XG = 4, SM = 8, PB = 16, OV = 32, XP = 64, SY = 128, SK = 256, ALL = 511 };

struct Buf : std::streambuf {
  const char* in = "";
  std::size_t pos = 0, len = 0;
  char out[256];
  std::size_t olen = 0;
  unsigned which = ALL;
  void inj(unsigned w) {
    if (which & w) point(virt);
  }
  int_type underflow() override {
    inj(U);
    return pos < len ? traits_type::to_int_type(in[pos]) : traits_type::eof();
  }
  int_type uflow() override {
    inj(UF);
    return pos < len ? traits_type::to_int_type(in[pos++]) : traits_type::eof();
  }
  std::streamsize xsgetn(char* s, std::streamsize n) override {
    inj(XG);
    return std::streambuf::xsgetn(s, n);
  }
  std::streamsize showmanyc() override {
    inj(SM);
    return std::streamsize(len - pos);
  }
  int_type pbackfail(int_type c) override {
    inj(PB);
    if (pos == 0) return traits_type::eof();
    if (!traits_type::eq_int_type(c, traits_type::eof()) && in[pos - 1] != traits_type::to_char_type(c))
      return traits_type::eof();
    --pos;
    return traits_type::to_int_type(in[pos]);
  }
  int_type overflow(int_type c) override {
    inj(OV);
    if (traits_type::eq_int_type(c, traits_type::eof())) return traits_type::not_eof(c);
    if (olen < sizeof out) out[olen++] = traits_type::to_char_type(c);
    return c;
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    inj(XP);
    return std::streambuf::xsputn(s, n);
  }
  int sync() override {
    inj(SY);
    return 0;
  }
  pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode) override {
    inj(SK);
    if (dir == std::ios_base::cur && off == 0) return pos_type(off_type(pos));
    return pos_type(off_type(-1));
  }
  pos_type seekpos(pos_type p, std::ios_base::openmode) override {
    inj(SK);
    pos = std::size_t(off_type(p));
    return p;
  }
};

static const char input[] = "123 4.5e1 word\nsecond line here\nthird";

// Runs op on a fresh stream for k = 1, 2, ... until op no longer reaches the k-th call.
template <class Stream, class Op>
void io(const char* name, unsigned which, Op op) {
  for (std::ios_base::iostate mask : {std::ios_base::goodbit, std::ios_base::badbit, std::ios_base::failbit}) {
    st.scenario = name;
    st.kind = virt;
    bool done = false;
    for (long k = 1; k < 400 && !done; ++k) {
      st.k = k;
      long nl0 = new_live;
      {
        Buf b;
        b.in = input;
        b.len = sizeof input - 1;
        b.which = which;
        Stream s(&b);
        s.unsetf(std::ios_base::skipws);
        s.exceptions(mask);
        bool injected = false, failure = false, other = false;
        disarm();
        st.fired = false;
        st.left[virt] = k;
        try {
          op(s);
        } catch (const Injected&) {
          injected = true;
        } catch (const std::ios_base::failure&) {
          failure = true;
        } catch (...) {
          other = true;
        }
        disarm();
        EXH_EXPECT(!other, "an unexpected exception type escaped");
        if (st.fired) {
          EXH_EXPECT(s.rdstate() & std::ios_base::badbit, "an exception was thrown during input/output but badbit is not set");
          if (mask & std::ios_base::badbit) {
            EXH_EXPECT(injected, "badbit is in exceptions(): the original exception must be rethrown");
            EXH_EXPECT(!failure, "badbit is in exceptions(): ios_base::failure thrown instead of rethrowing the original exception");
          } else {
            EXH_EXPECT(!injected, "badbit is not in exceptions(): the exception must not propagate");
            if (failure)
              EXH_EXPECT(s.rdstate() & s.exceptions(), "ios_base::failure thrown although no bit of exceptions() is set");
          }
        } else {
          done = true;
          EXH_EXPECT(!injected, "");
        }
      }
      if (new_live != nl0) {
        report("operator new blocks not freed", __LINE__);
        new_live = nl0;
      }
    }
    EXH_EXPECT(done, "the operation never completed");
  }
  st.kind = -1;
}

int main() {
  using IS = std::istream;
  using OS = std::ostream;
  // ---- formatted input
  io<IS>("istream >> int", ALL, [](IS& s) {
    int x;
    s >> x;
  });
  io<IS>("istream >> long long, >> char, >> double", ALL, [](IS& s) {
    long long x;
    char c;
    double d;
    s >> x >> c >> d;
  });
  io<IS>("istream >> bool (boolalpha)", ALL, [](IS& s) {
    bool b;
    s >> std::boolalpha >> b;
  });
  io<IS>("istream >> void*", ALL, [](IS& s) {
    void* p;
    s >> p;
  });
  io<IS>("istream >> char", ALL, [](IS& s) {
    char c;
    s >> c >> c >> c;
  });
  io<IS>("istream >> string", ALL, [](IS& s) {
    std::string w;
    s >> w;
  });
  io<IS>("istream >> char[]", ALL, [](IS& s) {
    char buf[16];
    s >> std::setw(10) >> buf;
  });
  // ---- unformatted input
  io<IS>("istream::get()", ALL, [](IS& s) {
    s.get();
    s.get();
  });
  io<IS>("istream::get(char&)", ALL, [](IS& s) {
    char c;
    s.get(c).get(c);
  });
  io<IS>("istream::get(char*, n)", ALL, [](IS& s) {
    char buf[32];
    s.get(buf, 32);
  });
  io<IS>("istream::get(streambuf&)", ALL & ~(OV | XP), [](IS& s) {
    Buf sink;
    sink.which = 0;
    s.get(sink);
  });
  io<IS>("istream::getline(char*, n)", ALL, [](IS& s) {
    char buf[32];
    s.getline(buf, 32);
  });
  io<IS>("std::getline(istream&, string&)", ALL, [](IS& s) {
    std::string line;
    std::getline(s, line);
  });
  io<IS>("istream::ignore(20, ' ')", ALL, [](IS& s) { s.ignore(20, ' '); });
  io<IS>("istream::peek()", ALL, [](IS& s) { s.peek(); });
  io<IS>("istream::read(buf, 12)", ALL, [](IS& s) {
    char buf[16];
    s.read(buf, 12);
  });
  io<IS>("istream::readsome(buf, 12)", ALL, [](IS& s) {
    char buf[16];
    s.readsome(buf, 12);
  });
  io<IS>("istream::putback", ALL, [](IS& s) {
    s.get();
    s.putback('1'); // no putback position: pbackfail
  });
  io<IS>("istream::unget", ALL, [](IS& s) {
    s.get();
    s.unget();
  });
  io<IS>("istream::sync", ALL, [](IS& s) { s.sync(); });
  io<IS>("istream::tellg", ALL, [](IS& s) { s.tellg(); });
  io<IS>("istream::seekg(pos)", ALL, [](IS& s) { s.seekg(std::streampos(3)); });
  io<IS>("istream::seekg(off, dir)", ALL, [](IS& s) { s.seekg(0, std::ios_base::cur); });
  io<IS>("istream >> ws", ALL, [](IS& s) {
    s.ignore(3);
    s >> std::ws;
  });

  // ---- formatted output
  io<OS>("ostream << int", ALL, [](OS& s) { s << 12345; });
  io<OS>("ostream << double", ALL, [](OS& s) { s << 3.25; });
  io<OS>("ostream << bool, << void*", ALL, [](OS& s) { s << std::boolalpha << true << static_cast<const void*>(&s); });
  io<OS>("ostream << const char*", ALL, [](OS& s) { s << "hello, world"; });
  io<OS>("ostream << setw(20) << string", ALL, [](OS& s) { s << std::setw(20) << std::string("padded"); });
  io<OS>("ostream << char", ALL, [](OS& s) { s << 'a' << 'b'; });
  io<OS>("ostream << string_view", ALL, [](OS& s) { s << std::string_view("view"); });
  io<OS>("ostream << endl", ALL, [](OS& s) { s << std::endl; });
  // ---- unformatted output
  io<OS>("ostream::put", ALL, [](OS& s) { s.put('x').put('y'); });
  io<OS>("ostream::write", ALL, [](OS& s) { s.write("abcdef", 6); });
  io<OS>("ostream::flush", ALL, [](OS& s) {
    s.put('x');
    s.flush();
  });

  // ---- ~sentry with unitbuf: pubsync throwing sets badbit and never propagates
  for (std::ios_base::iostate mask : {std::ios_base::goodbit, std::ios_base::badbit}) {
    st.scenario = "~sentry: unitbuf pubsync throws";
    st.kind = virt;
    st.k = 1;
    Buf b;
    b.which = SY;
    OS s(&b);
    s.setf(std::ios_base::unitbuf);
    s.exceptions(mask);
    bool escaped = false;
    disarm();
    st.left[virt] = 1;
    try {
      s << 42;
    } catch (...) {
      escaped = true;
    }
    disarm();
    EXH_EXPECT(!escaped, "[ostream.sentry]/4: an exception from pubsync in ~sentry propagated");
    EXH_EXPECT(s.rdstate() & std::ios_base::badbit, "[ostream.sentry]/4: badbit not set");
    st.kind = -1;
  }
  return finish();
}
