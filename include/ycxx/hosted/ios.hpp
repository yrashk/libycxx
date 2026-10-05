// libycxx hosted: <ios> ([iostreams.base]): fpos, io_errc, ios_base, basic_ios and the
// manipulators.
//
// ios_base keeps the stream state (rdstate and the exception mask) itself, so its out-of-line
// members can report a failed iword/pword allocation as setstate(badbit) would
// ([ios.base.storage]/4). Its non-template members are defined in the hosted runtime
// (src/hosted/ios.cpp). The bitmask types fmtflags, iostate and openmode and the enumeration
// seekdir are enumerations nested in ios_base, with their bitmask operators as hidden friends.
// basic_ios::init sets fill() to widen(' ') when the locale has a ctype<charT>; otherwise fill()
// is computed on first use, so streams of character types without a ctype facet can be built.
#pragma once

#include <ycxx/core/system_error.hpp>
#include <ycxx/hosted/iosfwd.hpp>
#include <ycxx/hosted/locale_base.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
struct ios_access;
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

using streamsize = ptrdiff_t;

// [fpos]
template <class stateT>
class fpos {
public:
  constexpr fpos(streamoff off = 0) noexcept(is_nothrow_default_constructible_v<stateT>) : st_(), off_(off) {}
  constexpr operator streamoff() const noexcept { return off_; }

  stateT state() const { return st_; }
  void state(stateT s) { st_ = s; }

  friend bool operator==(const fpos& p, const fpos& q) noexcept { return p.off_ == q.off_; }
  // `p == o` for an integer o: without this overload (a template, so that it is an exact match), the comparison would be ambiguous between
  // converting o to fpos and converting p to streamoff.
  template <class I>
    requires is_integral_v<I>
  friend bool operator==(const fpos& p, I o) noexcept {
    return p.off_ == o;
  }
  friend streamoff operator-(const fpos& p, const fpos& q) noexcept { return p.off_ - q.off_; }
  fpos& operator+=(streamoff o) noexcept {
    off_ += o;
    return *this;
  }
  fpos& operator-=(streamoff o) noexcept {
    off_ -= o;
    return *this;
  }
  fpos operator+(streamoff o) const {
    fpos r = *this;
    r += o;
    return r;
  }
  fpos operator-(streamoff o) const {
    fpos r = *this;
    r -= o;
    return r;
  }
  friend fpos operator+(streamoff o, const fpos& p) { return p + o; }

private:
  stateT st_;
  streamoff off_;
};

// [error.reporting]
enum class io_errc { stream = 1 };
template <>
struct is_error_code_enum<io_errc> : true_type {};
const error_category& iostream_category() noexcept;
inline error_code make_error_code(io_errc e) noexcept { return error_code(static_cast<int>(e), iostream_category()); }
inline error_condition make_error_condition(io_errc e) noexcept {
  return error_condition(static_cast<int>(e), iostream_category());
}

template <class charT, class traits>
class basic_ios;

// [ios.base]
class ios_base {
public:
  class failure;

  enum fmtflags : unsigned {};
  static constexpr fmtflags boolalpha = fmtflags(1u << 0);
  static constexpr fmtflags dec = fmtflags(1u << 1);
  static constexpr fmtflags fixed = fmtflags(1u << 2);
  static constexpr fmtflags hex = fmtflags(1u << 3);
  static constexpr fmtflags internal = fmtflags(1u << 4);
  static constexpr fmtflags left = fmtflags(1u << 5);
  static constexpr fmtflags oct = fmtflags(1u << 6);
  static constexpr fmtflags right = fmtflags(1u << 7);
  static constexpr fmtflags scientific = fmtflags(1u << 8);
  static constexpr fmtflags showbase = fmtflags(1u << 9);
  static constexpr fmtflags showpoint = fmtflags(1u << 10);
  static constexpr fmtflags showpos = fmtflags(1u << 11);
  static constexpr fmtflags skipws = fmtflags(1u << 12);
  static constexpr fmtflags unitbuf = fmtflags(1u << 13);
  static constexpr fmtflags uppercase = fmtflags(1u << 14);
  static constexpr fmtflags adjustfield = fmtflags(left | right | internal);
  static constexpr fmtflags basefield = fmtflags(dec | oct | hex);
  static constexpr fmtflags floatfield = fmtflags(scientific | fixed);

  enum iostate : unsigned {};
  static constexpr iostate badbit = iostate(1u << 0);
  static constexpr iostate eofbit = iostate(1u << 1);
  static constexpr iostate failbit = iostate(1u << 2);
  static constexpr iostate goodbit = iostate(0);

  enum openmode : unsigned {};
  static constexpr openmode app = openmode(1u << 0);
  static constexpr openmode ate = openmode(1u << 1);
  static constexpr openmode binary = openmode(1u << 2);
  static constexpr openmode in = openmode(1u << 3);
  static constexpr openmode noreplace = openmode(1u << 4);
  static constexpr openmode out = openmode(1u << 5);
  static constexpr openmode trunc = openmode(1u << 6);

  enum seekdir : int {};
  static constexpr seekdir beg = seekdir(0);
  static constexpr seekdir cur = seekdir(1);
  static constexpr seekdir end = seekdir(2);

  class Init;

  // [bitmask.types]
  friend constexpr fmtflags operator&(fmtflags a, fmtflags b) noexcept { return fmtflags(unsigned(a) & unsigned(b)); }
  friend constexpr fmtflags operator|(fmtflags a, fmtflags b) noexcept { return fmtflags(unsigned(a) | unsigned(b)); }
  friend constexpr fmtflags operator^(fmtflags a, fmtflags b) noexcept { return fmtflags(unsigned(a) ^ unsigned(b)); }
  friend constexpr fmtflags operator~(fmtflags a) noexcept { return fmtflags(~unsigned(a)); }
  friend constexpr fmtflags& operator&=(fmtflags& a, fmtflags b) noexcept { return a = a & b; }
  friend constexpr fmtflags& operator|=(fmtflags& a, fmtflags b) noexcept { return a = a | b; }
  friend constexpr fmtflags& operator^=(fmtflags& a, fmtflags b) noexcept { return a = a ^ b; }
  friend constexpr iostate operator&(iostate a, iostate b) noexcept { return iostate(unsigned(a) & unsigned(b)); }
  friend constexpr iostate operator|(iostate a, iostate b) noexcept { return iostate(unsigned(a) | unsigned(b)); }
  friend constexpr iostate operator^(iostate a, iostate b) noexcept { return iostate(unsigned(a) ^ unsigned(b)); }
  friend constexpr iostate operator~(iostate a) noexcept { return iostate(~unsigned(a)); }
  friend constexpr iostate& operator&=(iostate& a, iostate b) noexcept { return a = a & b; }
  friend constexpr iostate& operator|=(iostate& a, iostate b) noexcept { return a = a | b; }
  friend constexpr iostate& operator^=(iostate& a, iostate b) noexcept { return a = a ^ b; }
  friend constexpr openmode operator&(openmode a, openmode b) noexcept { return openmode(unsigned(a) & unsigned(b)); }
  friend constexpr openmode operator|(openmode a, openmode b) noexcept { return openmode(unsigned(a) | unsigned(b)); }
  friend constexpr openmode operator^(openmode a, openmode b) noexcept { return openmode(unsigned(a) ^ unsigned(b)); }
  friend constexpr openmode operator~(openmode a) noexcept { return openmode(~unsigned(a)); }
  friend constexpr openmode& operator&=(openmode& a, openmode b) noexcept { return a = a & b; }
  friend constexpr openmode& operator|=(openmode& a, openmode b) noexcept { return a = a | b; }
  friend constexpr openmode& operator^=(openmode& a, openmode b) noexcept { return a = a ^ b; }

  // [fmtflags.state]
  fmtflags flags() const { return flags_; }
  fmtflags flags(fmtflags fmtfl) {
    const fmtflags old = flags_;
    flags_ = fmtfl;
    return old;
  }
  fmtflags setf(fmtflags fmtfl) {
    const fmtflags old = flags_;
    flags_ |= fmtfl;
    return old;
  }
  fmtflags setf(fmtflags fmtfl, fmtflags mask) {
    const fmtflags old = flags_;
    flags_ = (flags_ & ~mask) | (fmtfl & mask);
    return old;
  }
  void unsetf(fmtflags mask) { flags_ &= ~mask; }
  streamsize precision() const { return prec_; }
  streamsize precision(streamsize prec) {
    const streamsize old = prec_;
    prec_ = prec;
    return old;
  }
  streamsize width() const { return width_; }
  // Stores only a changed value: every formatted inserter ends with width(0), and concurrent
  // formatted output on a synchronized standard stream must not race
  // ([iostream.objects.overview]/7), which plain stores of the same zero would.
  streamsize width(streamsize wide) {
    const streamsize old = width_;
    if (old != wide)
      width_ = wide;
    return old;
  }

  // [ios.base.locales]
  locale imbue(const locale& loc);
  locale getloc() const { return loc_; }

  // [ios.base.storage]
  static int xalloc();
  long& iword(int idx);
  void*& pword(int idx);

  virtual ~ios_base();

  // [ios.base.callback]
  enum event { erase_event, imbue_event, copyfmt_event };
  using event_callback = void (*)(event, ios_base&, int idx);
  void register_callback(event_callback fn, int idx);

  ios_base(const ios_base&) = delete;
  ios_base& operator=(const ios_base&) = delete;

  static bool sync_with_stdio(bool sync = true);

protected:
  ios_base() : loc_(locale::classic()) {}

private:
  template <class charT, class traits>
  friend class basic_ios;
  friend struct ycxx::detail::ios_access;

  struct callback {
    event_callback fn;
    int idx;
  };
  struct storage; // iword/pword arrays and callbacks (src/hosted/ios.cpp)

  // The ios_base part of basic_ios::init, copyfmt, move and swap.
  void init_base(bool has_buf);
  void call_callbacks(event ev) noexcept;
  void copy_base(const ios_base& rhs); // flags, width, precision, locale, arrays, callbacks
  void move_base(ios_base& rhs) noexcept;
  void swap_base(ios_base& rhs) noexcept;
  // setstate(badbit) for a failed iword/pword (may throw failure).
  void storage_failed();

  fmtflags flags_ = fmtflags(skipws | dec);
  iostate state_ = goodbit;
  iostate except_ = goodbit;
  streamsize prec_ = 6;
  streamsize width_ = 0;
  locale loc_;
  storage* store_ = nullptr;
};

// [ios.failure]
class ios_base::failure : public system_error {
public:
  explicit failure(const string& msg, const error_code& ec = io_errc::stream);
  explicit failure(const char* msg, const error_code& ec = io_errc::stream);
  failure(const failure&) noexcept = default;
  failure& operator=(const failure&) noexcept = default;
  ~failure() override; // the key function, in the hosted runtime
};

// [ios.init]
class ios_base::Init {
public:
  Init();
  Init(const Init&) = default;
  ~Init();
  Init& operator=(const Init&) = default;
};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

struct ios_access {
  // Sets badbit without throwing failure (the exception rule of the I/O functions).
  static void set_badbit_quietly(std::ios_base& s) noexcept { s.state_ |= std::ios_base::badbit; }
  static void set_failbit_quietly(std::ios_base& s) noexcept { s.state_ |= std::ios_base::failbit; }
  // The stream's locale itself (getloc() returns a copy, which costs two reference-count updates).
  static const std::locale& locale_of(const std::ios_base& s) noexcept { return s.loc_; }
};

// Throws ios_base::failure(what) (hosted runtime); without exceptions, the error handler.
[[noreturn]] void throw_ios_failure(const char* what);
[[noreturn]] [[gnu::cold]] inline void raise_ios_failure(const char* what) {
  if constexpr (cfg::exceptions)
    ::ycxx::detail::throw_ios_failure(what);
  else
    ::ycxx_error_handler(ycxx_error_system_error, what);
}

// The exception rule of the formatted and unformatted I/O functions ([istream.formatted.reqmts]/1,
// [ostream.formatted.reqmts]/1, [istream.unformatted]/1, ...): an exception thrown by `body` sets
// badbit without throwing failure, and is rethrown if badbit is in exceptions(). Exceptions
// thrown by clear() are not caught: callers collect the state and call setstate afterwards.
// With `pass` set by the time an exception escapes body, the exception has already been dealt
// with by body's own rule and is propagated unchanged.
template <class Ios, class F>
void guarded_io(Ios& s, F&& body, const bool& pass) {
  if constexpr (cfg::exceptions) {
    try {
      static_cast<F&&>(body)();
    } catch (...) {
      if (pass)
        throw;
      ::ycxx::detail::ios_access::set_badbit_quietly(s);
      if (s.exceptions() & std::ios_base::badbit)
        throw;
    }
  } else {
    static_cast<F&&>(body)();
  }
}
template <class Ios, class F>
void guarded_io(Ios& s, F&& body) {
  constexpr bool never = false;
  ::ycxx::detail::guarded_io(s, static_cast<F&&>(body), never);
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [ios]
template <class charT, class traits>
class basic_ios : public ios_base {
public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;

  // [iostate.flags]
  explicit operator bool() const { return !fail(); }
  bool operator!() const { return fail(); }
  iostate rdstate() const { return state_; }
  void clear(iostate state = goodbit) {
    if (sb_ == nullptr)
      state |= badbit;
    state_ = state;
    if (state_ & except_)
      ::ycxx::detail::raise_ios_failure("std::basic_ios::clear: the stream state matches exceptions()");
  }
  void setstate(iostate state) { clear(rdstate() | state); }
  bool good() const { return state_ == goodbit; }
  bool eof() const { return (state_ & eofbit) != 0; }
  bool fail() const { return (state_ & (failbit | badbit)) != 0; }
  bool bad() const { return (state_ & badbit) != 0; }
  iostate exceptions() const { return except_; }
  void exceptions(iostate except) {
    except_ = except;
    clear(rdstate());
  }

  // [basic.ios.cons]
  explicit basic_ios(basic_streambuf<charT, traits>* sb) { init(sb); }
  ~basic_ios() override {}
  basic_ios(const basic_ios&) = delete;
  basic_ios& operator=(const basic_ios&) = delete;

  // [basic.ios.members]
  basic_ostream<charT, traits>* tie() const { return tie_; }
  basic_ostream<charT, traits>* tie(basic_ostream<charT, traits>* tiestr) {
    basic_ostream<charT, traits>* old = tie_;
    tie_ = tiestr;
    return old;
  }
  basic_streambuf<charT, traits>* rdbuf() const { return sb_; }
  basic_streambuf<charT, traits>* rdbuf(basic_streambuf<charT, traits>* sb) {
    basic_streambuf<charT, traits>* old = sb_;
    sb_ = sb;
    clear();
    return old;
  }

  basic_ios& copyfmt(const basic_ios& rhs) {
    if (this == __builtin_addressof(rhs))
      return *this;
    call_callbacks(erase_event);
    copy_base(rhs);
    tie_ = rhs.tie_;
    fill_ = rhs.fill_;
    fill_set_ = rhs.fill_set_;
    call_callbacks(copyfmt_event);
    exceptions(rhs.exceptions());
    return *this;
  }

  char_type fill() const {
    if (!fill_set_) {
      fill_ = widen(' ');
      fill_set_ = true;
    }
    return fill_;
  }
  char_type fill(char_type ch) {
    const char_type old = fill();
    fill_ = ch;
    return old;
  }

  locale imbue(const locale& loc) {
    locale old = ios_base::imbue(loc);
    if (sb_ != nullptr)
      sb_->pubimbue(loc);
    return old;
  }

  char narrow(char_type c, char dfault) const { return use_facet<ctype<char_type>>(getloc()).narrow(c, dfault); }
  char_type widen(char c) const { return use_facet<ctype<char_type>>(getloc()).widen(c); }

protected:
  basic_ios() {}
  void init(basic_streambuf<charT, traits>* sb) {
    init_base(sb != nullptr);
    sb_ = sb;
    tie_ = nullptr;
    // [basic.ios.cons] Table: fill() is widen(' ') in the locale at this point; without a
    // ctype<charT> there, it is computed on first use
    fill_set_ = has_facet<ctype<charT>>(getloc());
    if (fill_set_)
      fill_ = widen(' ');
  }
  void move(basic_ios& rhs) {
    move_base(rhs);
    tie_ = rhs.tie_;
    rhs.tie_ = nullptr;
    fill_ = rhs.fill_;
    fill_set_ = rhs.fill_set_;
    sb_ = nullptr;
  }
  void move(basic_ios&& rhs) { move(rhs); }
  void swap(basic_ios& rhs) noexcept {
    swap_base(rhs);
    basic_ostream<charT, traits>* t = tie_;
    tie_ = rhs.tie_;
    rhs.tie_ = t;
    const char_type f = fill_;
    fill_ = rhs.fill_;
    rhs.fill_ = f;
    const bool fs = fill_set_;
    fill_set_ = rhs.fill_set_;
    rhs.fill_set_ = fs;
  }
  void set_rdbuf(basic_streambuf<charT, traits>* sb) { sb_ = sb; }

private:
  basic_streambuf<charT, traits>* sb_ = nullptr;
  basic_ostream<charT, traits>* tie_ = nullptr;
  mutable char_type fill_{};
  mutable bool fill_set_ = false;
};

// [std.ios.manip]
inline ios_base& boolalpha(ios_base& str) {
  str.setf(ios_base::boolalpha);
  return str;
}
inline ios_base& noboolalpha(ios_base& str) {
  str.unsetf(ios_base::boolalpha);
  return str;
}
inline ios_base& showbase(ios_base& str) {
  str.setf(ios_base::showbase);
  return str;
}
inline ios_base& noshowbase(ios_base& str) {
  str.unsetf(ios_base::showbase);
  return str;
}
inline ios_base& showpoint(ios_base& str) {
  str.setf(ios_base::showpoint);
  return str;
}
inline ios_base& noshowpoint(ios_base& str) {
  str.unsetf(ios_base::showpoint);
  return str;
}
inline ios_base& showpos(ios_base& str) {
  str.setf(ios_base::showpos);
  return str;
}
inline ios_base& noshowpos(ios_base& str) {
  str.unsetf(ios_base::showpos);
  return str;
}
inline ios_base& skipws(ios_base& str) {
  str.setf(ios_base::skipws);
  return str;
}
inline ios_base& noskipws(ios_base& str) {
  str.unsetf(ios_base::skipws);
  return str;
}
inline ios_base& uppercase(ios_base& str) {
  str.setf(ios_base::uppercase);
  return str;
}
inline ios_base& nouppercase(ios_base& str) {
  str.unsetf(ios_base::uppercase);
  return str;
}
inline ios_base& unitbuf(ios_base& str) {
  str.setf(ios_base::unitbuf);
  return str;
}
inline ios_base& nounitbuf(ios_base& str) {
  str.unsetf(ios_base::unitbuf);
  return str;
}
// [adjustfield.manip]
inline ios_base& internal(ios_base& str) {
  str.setf(ios_base::internal, ios_base::adjustfield);
  return str;
}
inline ios_base& left(ios_base& str) {
  str.setf(ios_base::left, ios_base::adjustfield);
  return str;
}
inline ios_base& right(ios_base& str) {
  str.setf(ios_base::right, ios_base::adjustfield);
  return str;
}
// [basefield.manip]
inline ios_base& dec(ios_base& str) {
  str.setf(ios_base::dec, ios_base::basefield);
  return str;
}
inline ios_base& hex(ios_base& str) {
  str.setf(ios_base::hex, ios_base::basefield);
  return str;
}
inline ios_base& oct(ios_base& str) {
  str.setf(ios_base::oct, ios_base::basefield);
  return str;
}
// [floatfield.manip]
inline ios_base& fixed(ios_base& str) {
  str.setf(ios_base::fixed, ios_base::floatfield);
  return str;
}
inline ios_base& scientific(ios_base& str) {
  str.setf(ios_base::scientific, ios_base::floatfield);
  return str;
}
inline ios_base& hexfloat(ios_base& str) {
  str.setf(ios_base::fixed | ios_base::scientific, ios_base::floatfield);
  return str;
}
inline ios_base& defaultfloat(ios_base& str) {
  str.unsetf(ios_base::floatfield);
  return str;
}

} // namespace std
