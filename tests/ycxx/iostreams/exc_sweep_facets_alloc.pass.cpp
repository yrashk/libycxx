// Exceptions thrown by locale facets and by stream buffers' allocators during formatted and
// unformatted I/O, with exceptions() set to goodbit, badbit and failbit; the k-th call of the
// facet's virtual (or of the allocator's allocate, or operator new) throws, for every k.
//   [istream.formatted.reqmts]/1, [istream.unformatted]/1, [ostream.formatted.reqmts]/1,
//   [ostream.unformatted]/1: an exception thrown during input/output sets badbit and is
//   rethrown iff (exceptions() & badbit) != 0 (the original exception, not ios_base::failure).
//   [istream.arithmetic]/1-2, [ostream.inserters.arithmetic]/1: the arithmetic extractors and
//     inserters call num_get::get / num_put::put of the stream's locale: a facet that throws is
//     an exception during input/output. [ext.manip]/3, /6: get_money/put_money behave as
//     formatted input/output functions calling the money facets. [ext.manip]/8, /10, by
//     contrast, specify in >> get_time(tmb, fmt) and out << put_time(tmb, fmt) as calls of a
//     function f that calls the time facet directly (no sentry, no try/catch): the facet's
//     exception propagates whatever exceptions() is, and the state is unchanged.
//   [stringbuf.virtuals], [syncstream.syncbuf]: the buffers allocate through their allocator;
//     an allocation failure either propagates out of overflow (an exception during output) or
//     makes overflow fail (setstate(badbit)): in both cases badbit is set, and with badbit not
//     in exceptions() nothing propagates. Every allocated block is deallocated exactly once.
//   [syncstream.syncbuf.cons]/8: "If an exception is thrown from emit(), the destructor
//     catches and ignores that exception." [syncstream.osyncstream.members]/1: emit() "Behaves
//     as an unformatted output function".
//   operator>>(istream&, string&) and getline growing the string: an allocation failure is an
//   exception during input.
#include <iomanip>
#include <ios>
#include <istream>
#include <locale>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <syncstream>
#include "exc_new.hpp"

using namespace exh;
using std::ios_base;

struct NumGet : std::num_get<char> {
  using B = std::num_get<char>;
  iter_type do_get(iter_type b, iter_type e, ios_base& s, ios_base::iostate& err, long& v) const override {
    point(virt);
    return B::do_get(b, e, s, err, v);
  }
  iter_type do_get(iter_type b, iter_type e, ios_base& s, ios_base::iostate& err, unsigned long& v) const override {
    point(virt);
    return B::do_get(b, e, s, err, v);
  }
  iter_type do_get(iter_type b, iter_type e, ios_base& s, ios_base::iostate& err, double& v) const override {
    point(virt);
    return B::do_get(b, e, s, err, v);
  }
  iter_type do_get(iter_type b, iter_type e, ios_base& s, ios_base::iostate& err, bool& v) const override {
    point(virt);
    return B::do_get(b, e, s, err, v);
  }
  iter_type do_get(iter_type b, iter_type e, ios_base& s, ios_base::iostate& err, void*& v) const override {
    point(virt);
    return B::do_get(b, e, s, err, v);
  }
};
struct NumPut : std::num_put<char> {
  using B = std::num_put<char>;
  iter_type do_put(iter_type o, ios_base& s, char f, long v) const override {
    point(virt);
    return B::do_put(o, s, f, v);
  }
  iter_type do_put(iter_type o, ios_base& s, char f, unsigned long v) const override {
    point(virt);
    return B::do_put(o, s, f, v);
  }
  iter_type do_put(iter_type o, ios_base& s, char f, double v) const override {
    point(virt);
    return B::do_put(o, s, f, v);
  }
  iter_type do_put(iter_type o, ios_base& s, char f, bool v) const override {
    point(virt);
    return B::do_put(o, s, f, v);
  }
  iter_type do_put(iter_type o, ios_base& s, char f, const void* v) const override {
    point(virt);
    return B::do_put(o, s, f, v);
  }
};
struct MoneyGet : std::money_get<char> {
  iter_type do_get(iter_type b, iter_type e, bool intl, ios_base& s, ios_base::iostate& err, long double& v) const override {
    point(virt);
    return std::money_get<char>::do_get(b, e, intl, s, err, v);
  }
};
struct MoneyPut : std::money_put<char> {
  iter_type do_put(iter_type o, bool intl, ios_base& s, char f, long double v) const override {
    point(virt);
    return std::money_put<char>::do_put(o, intl, s, f, v);
  }
};
struct TimeGet : std::time_get<char> {
  iter_type do_get(iter_type b, iter_type e, ios_base& s, ios_base::iostate& err, std::tm* t, char f, char m) const override {
    point(virt);
    return std::time_get<char>::do_get(b, e, s, err, t, f, m);
  }
};
struct TimePut : std::time_put<char> {
  iter_type do_put(iter_type o, ios_base& s, char f, const std::tm* t, char fm, char m) const override {
    point(virt);
    return std::time_put<char>::do_put(o, s, f, t, fm, m);
  }
};

static std::locale throwing_locale() {
  std::locale l(std::locale::classic(), new NumGet);
  l = std::locale(l, new NumPut);
  l = std::locale(l, new MoneyGet);
  l = std::locale(l, new MoneyPut);
  l = std::locale(l, new TimeGet);
  return std::locale(l, new TimePut);
}

// The common check. fired: an injected exception was thrown inside op.
// accept_failure_with_badbit: the injected failure may have been turned into a failed buffer
// operation (then setstate(badbit) throws ios_base::failure when badbit is in exceptions()).
// plain: the expression is specified as a plain call of the facet (get_time/put_time,
// [ext.manip]/8, /10), not as a formatted I/O function: the exception propagates whatever
// exceptions() is, and the stream state is not changed.
template <class Setup, class Op>
void io(const char* name, Kind kind, Setup setup, Op op, bool accept_failure_with_badbit = false, bool plain = false) {
  for (ios_base::iostate mask : {ios_base::goodbit, ios_base::badbit, ios_base::failbit}) {
    st.scenario = name;
    st.kind = kind;
    bool done = false;
    for (long k = 1; k < 2000 && !done; ++k) {
      st.k = k;
      long nl0 = new_live;
      int nb0 = nblocks;
      {
        auto env = setup();
        auto& s = env.stream();
        s.exceptions(mask);
        bool injected = false, failure = false, other = false;
        disarm();
        st.fired = false;
        st.left[kind] = k;
        try {
          op(s);
        } catch (const Injected&) {
          injected = true;
        } catch (const alloc_failure&) {
          injected = true;
        } catch (const ios_base::failure&) {
          failure = true;
        } catch (...) {
          other = true;
        }
        disarm();
        EXH_EXPECT(!other, "an unexpected exception type escaped");
        if (st.fired && plain) {
          EXH_EXPECT(injected, "get_time/put_time: the facet's exception must propagate (no sentry, no catch)");
          EXH_EXPECT(s.rdstate() == ios_base::goodbit, "get_time/put_time: the stream state changed");
        } else if (st.fired) {
          EXH_EXPECT(s.rdstate() & ios_base::badbit, "an exception was thrown during input/output but badbit is not set");
          if (mask & ios_base::badbit) {
            if (accept_failure_with_badbit)
              EXH_EXPECT(injected || failure, "badbit is in exceptions(): nothing was thrown");
            else
              EXH_EXPECT(injected && !failure, "badbit is in exceptions(): the original exception must be rethrown");
          } else {
            EXH_EXPECT(!injected, "badbit is not in exceptions(): the exception must not propagate");
            if (failure) EXH_EXPECT(s.rdstate() & s.exceptions(), "ios_base::failure thrown although no bit of exceptions() is set");
          }
        } else {
          done = true;
        }
      }
      if (new_live != nl0) {
        report("operator new blocks not freed", __LINE__);
        new_live = nl0;
      }
      if (nblocks != nb0) {
        report("allocator blocks not freed", __LINE__);
        nblocks = nb0;
      }
    }
    EXH_EXPECT(done, "the operation never completed");
  }
  st.kind = -1;
}

struct InEnv {
  std::istringstream is;
  explicit InEnv(const char* text) : is(text) { is.imbue(throwing_locale()); }
  std::istream& stream() { return is; }
};
struct OutEnv {
  std::ostringstream os;
  OutEnv() { os.imbue(throwing_locale()); }
  std::ostream& stream() { return os; }
};

using AS = std::basic_ostringstream<char, std::char_traits<char>, alloc<char>>;
using AIS = std::basic_istringstream<char, std::char_traits<char>, alloc<char>>;
using ASync = std::basic_osyncstream<char, std::char_traits<char>, alloc<char>>;

struct Sink : std::streambuf {
  char data[4096];
  std::size_t n = 0;
  bool throw_on_write = false;
  int_type overflow(int_type c) override {
    if (throw_on_write) point(virt);
    if (!traits_type::eq_int_type(c, traits_type::eof()) && n < sizeof data) data[n++] = char(c);
    return traits_type::not_eof(c);
  }
  std::streamsize xsputn(const char* s, std::streamsize k) override {
    if (throw_on_write) point(virt);
    for (std::streamsize i = 0; i < k; ++i)
      if (n < sizeof data) data[n++] = s[i];
    return k;
  }
};

int main() {
  // Construct the locale machinery once outside any sweep.
  { InEnv warm("1"); OutEnv warm2; }

  auto in = [](const char* t) { return [t] { return InEnv(t); }; };
  auto out = [] { return OutEnv(); };
  io("istream >> int (num_get throws)", virt, in("123 456"), [](std::istream& s) {
    int a, b;
    s >> a >> b;
  });
  io("istream >> unsigned, >> double, >> bool, >> void*", virt, in("7 2.5 1 0x10"), [](std::istream& s) {
    unsigned u;
    double d;
    bool b;
    void* p;
    s >> u >> d >> b >> p;
  });
  io("istream >> get_money", virt, in("1234"), [](std::istream& s) {
    long double m;
    s >> std::get_money(m);
  });
  io("istream >> get_time", virt, in("12:34:56"), [](std::istream& s) {
    std::tm t{};
    s >> std::get_time(&t, "%H:%M:%S");
  }, false, true);
  io("ostream << int, << long (num_put throws)", virt, out, [](std::ostream& s) { s << 1 << ' ' << 2L; });
  io("ostream << double, << bool, << const void*", virt, out,
     [](std::ostream& s) { s << 2.5 << true << static_cast<const void*>(&s); });
  io("ostream << put_money", virt, out, [](std::ostream& s) { s << std::put_money(1234.0L); });
  io("ostream << put_time", virt, out, [](std::ostream& s) {
    std::tm t{};
    t.tm_hour = 12;
    s << std::put_time(&t, "%H:%M");
  }, false, true);

  // operator new failures while extracting into a std::string
  io("istream >> string (operator new fails)", gnew, in("averyveryveryverylongwordthatexceedsanysmallbuffer x"),
     [](std::istream& s) {
       std::string w;
       s >> w;
     });
  io("getline(istream&, string&) (operator new fails)", gnew,
     in("a line that is long enough to need several reallocations of the string's buffer\nnext"),
     [](std::istream& s) {
       std::string w;
       std::getline(s, w);
     });

  // basic_ostringstream / basic_istringstream with exh::alloc
  struct AOut {
    AS os;
    std::ostream& stream() { return os; }
  };
  io("basic_ostringstream<alloc> << long text (allocate fails)", allocation, [] { return AOut(); },
     [](std::ostream& s) {
       for (int i = 0; i < 20; ++i) s << "0123456789abcdefghijklmnopqrstuvwxyz";
     },
     true);
  io("basic_ostringstream<alloc> << int, put, write (allocate fails)", allocation, [] { return AOut(); },
     [](std::ostream& s) {
       for (int i = 0; i < 40; ++i) s << 1234567 << ' ';
       s.put('x');
       s.write("abcdefghijklmnopqrstuvwxyz", 26);
     },
     true);
  // str(s) / str() with a failing allocator: no particular guarantee; nothing leaks.
  for (long k = 1; k < 50; ++k) {
    st.scenario = "basic_stringbuf<alloc>::str(s), str()";
    st.kind = allocation;
    st.k = k;
    int nb0 = nblocks;
    bool threw;
    {
      using AStr = std::basic_string<char, std::char_traits<char>, alloc<char>>;
      AStr text("some text long enough to be stored out of line, certainly more than SSO");
      std::basic_stringbuf<char, std::char_traits<char>, alloc<char>> sb;
      threw = attempt([&] {
        sb.str(text);
        AStr copy = sb.str();
        sb.sputn("more", 4);
        AStr copy2 = sb.str();
      });
    }
    if (nblocks != nb0) {
      report("allocator blocks not freed", __LINE__);
      nblocks = nb0;
    }
    if (!threw) break;
  }

  // basic_osyncstream with exh::alloc: allocation failures while buffering
  for (ios_base::iostate mask : {ios_base::goodbit, ios_base::badbit}) {
    for (long k = 1; k < 200; ++k) {
      st.scenario = "basic_osyncstream<alloc> << text (allocate fails)";
      st.kind = allocation;
      st.k = k;
      int nb0 = nblocks;
      bool fired;
      {
        Sink sink;
        ASync os(&sink);
        os.exceptions(mask);
        bool escaped = false;
        disarm();
        st.fired = false;
        st.left[allocation] = k;
        try {
          for (int i = 0; i < 30; ++i) os << "0123456789abcdefghijklmnopqrstuvwxyz";
        } catch (...) {
          escaped = true;
        }
        disarm();
        fired = st.fired;
        if (fired) {
          EXH_EXPECT(os.rdstate() & ios_base::badbit, "badbit not set after an allocation failure");
          if (!(mask & ios_base::badbit)) EXH_EXPECT(!escaped, "an exception propagated with badbit not in exceptions()");
        }
      }
      if (nblocks != nb0) {
        report("allocator blocks not freed", __LINE__);
        nblocks = nb0;
      }
      if (!fired) break;
    }
  }
  // emit() when the wrapped buffer throws: an unformatted output function.
  for (ios_base::iostate mask : {ios_base::goodbit, ios_base::badbit}) {
    st.scenario = "basic_osyncstream::emit() with a throwing wrapped buffer";
    st.kind = virt;
    st.k = 1;
    Sink sink;
    {
      std::osyncstream os(&sink);
      os << "hello";
      os.exceptions(mask);
      sink.throw_on_write = true;
      bool injected = false, failure = false;
      disarm();
      st.left[virt] = 1;
      try {
        os.emit();
      } catch (const Injected&) {
        injected = true;
      } catch (const ios_base::failure&) {
        failure = true;
      }
      disarm();
      EXH_EXPECT(os.rdstate() & ios_base::badbit, "emit(): badbit not set");
      if (mask & ios_base::badbit)
        EXH_EXPECT(injected || failure, "emit(): badbit in exceptions() but nothing propagated");
      else
        EXH_EXPECT(!injected && !failure, "emit(): an exception propagated with badbit not in exceptions()");
      sink.throw_on_write = false;
    }
  }
  // ~basic_syncbuf: emit() throwing is caught and ignored ([syncstream.syncbuf.cons]/8).
  {
    st.scenario = "~basic_osyncstream with a throwing wrapped buffer";
    st.kind = virt;
    Sink sink;
    sink.throw_on_write = true;
    {
      std::osyncstream os(&sink);
      os << "pending text";
      st.left[virt] = 1;
    } // a propagating exception would call terminate (destructors are noexcept)
    disarm();
    st.kind = -1;
  }
  return finish();
}
