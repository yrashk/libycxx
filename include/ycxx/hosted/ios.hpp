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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
struct __ios_access;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

using streamsize = ptrdiff_t;

// [fpos]
template <class __stateT>
class fpos {
public:
  constexpr fpos(streamoff __off = 0) noexcept(is_nothrow_default_constructible_v<__stateT>) : __st_(), __off_(__off) {}
  constexpr operator streamoff() const noexcept { return __off_; }

  __stateT state() const { return __st_; }
  void state(__stateT s) { __st_ = s; }

  friend bool operator==(const fpos& p, const fpos& __q) noexcept { return p.__off_ == __q.__off_; }
  // `p == __o` for an integer o: without this overload (a template, so that it is an exact match), the comparison would be ambiguous between
  // converting o to fpos and converting p to streamoff.
  template <class _Ip>
    requires is_integral_v<_Ip>
  friend bool operator==(const fpos& p, _Ip __o) noexcept {
    return p.__off_ == __o;
  }
  friend streamoff operator-(const fpos& p, const fpos& __q) noexcept { return p.__off_ - __q.__off_; }
  fpos& operator+=(streamoff __o) noexcept {
    __off_ += __o;
    return *this;
  }
  fpos& operator-=(streamoff __o) noexcept {
    __off_ -= __o;
    return *this;
  }
  fpos operator+(streamoff __o) const {
    fpos r = *this;
    r += __o;
    return r;
  }
  fpos operator-(streamoff __o) const {
    fpos r = *this;
    r -= __o;
    return r;
  }
  friend fpos operator+(streamoff __o, const fpos& p) { return p + __o; }

private:
  __stateT __st_;
  streamoff __off_;
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

template <class __charT, class __traits>
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
  fmtflags flags() const { return __flags_; }
  fmtflags flags(fmtflags __fmtfl) {
    const fmtflags __old = __flags_;
    __flags_ = __fmtfl;
    return __old;
  }
  fmtflags setf(fmtflags __fmtfl) {
    const fmtflags __old = __flags_;
    __flags_ |= __fmtfl;
    return __old;
  }
  fmtflags setf(fmtflags __fmtfl, fmtflags mask) {
    const fmtflags __old = __flags_;
    __flags_ = (__flags_ & ~mask) | (__fmtfl & mask);
    return __old;
  }
  void unsetf(fmtflags mask) { __flags_ &= ~mask; }
  streamsize precision() const { return __prec_; }
  streamsize precision(streamsize __prec) {
    const streamsize __old = __prec_;
    __prec_ = __prec;
    return __old;
  }
  streamsize width() const { return __width_; }
  // Stores only a changed value: every formatted inserter ends with width(0), and concurrent
  // formatted output on a synchronized standard stream must not race
  // ([iostream.objects.overview]/7), which plain stores of the same zero would.
  streamsize width(streamsize __wide) {
    const streamsize __old = __width_;
    if (__old != __wide)
      __width_ = __wide;
    return __old;
  }

  // [ios.base.locales]
  locale imbue(const locale& __loc);
  locale getloc() const { return __loc_; }

  // [ios.base.storage]
  static int xalloc();
  long& iword(int __idx);
  void*& pword(int __idx);

  virtual ~ios_base();

  // [ios.base.callback]
  enum event { erase_event, imbue_event, copyfmt_event };
  using event_callback = void (*)(event, ios_base&, int __idx);
  void register_callback(event_callback __fn, int __idx);

  ios_base(const ios_base&) = delete;
  ios_base& operator=(const ios_base&) = delete;

  static bool sync_with_stdio(bool sync = true);

protected:
  ios_base() : __loc_(locale::classic()) {}

private:
  template <class __charT, class __traits>
  friend class basic_ios;
  friend struct __ycxx::__detail::__ios_access;

  struct __y_callback {
    event_callback __fn;
    int __idx;
  };
  struct __storage; // iword/pword arrays and callbacks (src/hosted/ios.cpp)

  // The ios_base part of basic_ios::init, copyfmt, move and swap.
  void __init_base(bool __has_buf);
  void __call_callbacks(event __ev) noexcept;
  void __copy_base(const ios_base& __rhs); // flags, width, precision, locale, arrays, callbacks
  void __move_base(ios_base& __rhs) noexcept;
  void __swap_base(ios_base& __rhs) noexcept;
  // setstate(badbit) for a failed iword/pword (may throw failure).
  void __storage_failed();

  // The stream state is read and written with relaxed atomic operations, and bits are added
  // with an atomic OR only when they change it: input functions on a synchronized standard
  // stream object may be called concurrently ([iostream.objects.overview]/7), and each of them
  // reads the state (the sentry's good()) and sets bits at end of file. A relaxed load or store
  // is an ordinary load or store on the supported targets; the read-modify-write happens only on
  // a transition (DECISIONS §7).
  iostate __load_state() const noexcept { return __atomic_load_n(&__state_, __ATOMIC_RELAXED); }
  void __store_state(iostate s) noexcept { __atomic_store_n(&__state_, s, __ATOMIC_RELAXED); }
  // Adds the bits of s; returns the new state.
  iostate __add_state(iostate s) noexcept {
    const iostate __old = __load_state();
    if ((__old | s) == __old)
      return __old;
    return __atomic_or_fetch(&__state_, s, __ATOMIC_RELAXED);
  }

  fmtflags __flags_ = fmtflags(skipws | dec);
  iostate __state_ = goodbit;
  iostate __except_ = goodbit;
  streamsize __prec_ = 6;
  streamsize __width_ = 0;
  locale __loc_;
  __storage* __store_ = nullptr;
};

// [ios.failure]
class ios_base::failure : public system_error {
public:
  explicit failure(const string& __msg, const error_code& ec = io_errc::stream);
  explicit failure(const char* __msg, const error_code& ec = io_errc::stream);
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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __ios_access {
  // Sets badbit without throwing failure (the exception rule of the I/O functions).
  static void __set_badbit_quietly(std::ios_base& s) noexcept { s.__add_state(std::ios_base::badbit); }
  static void __set_failbit_quietly(std::ios_base& s) noexcept { s.__add_state(std::ios_base::failbit); }
  // The stream's locale itself (getloc() returns a copy, which costs two reference-count updates).
  static const std::locale& __locale_of(const std::ios_base& s) noexcept { return s.__loc_; }
};

// Throws ios_base::failure(what) (hosted runtime); without exceptions, the error handler.
[[noreturn]] void __throw_ios_failure(const char* what);
[[noreturn]] [[__gnu__::__cold__]] inline void __raise_ios_failure(const char* what) {
  if constexpr (__cfg::exceptions)
    ::__ycxx::__detail::__throw_ios_failure(what);
  else
    ::ycxx_error_handler(ycxx_error_system_error, what);
}

// The exception rule of the formatted and unformatted I/O functions ([istream.formatted.reqmts]/1,
// [ostream.formatted.reqmts]/1, [istream.unformatted]/1, ...): an exception thrown by `__body` sets
// badbit without throwing failure, and is rethrown if badbit is in exceptions(). Exceptions
// thrown by clear() are not caught: callers collect the state and call setstate afterwards.
// With `__pass` set by the time an exception escapes body, the exception has already been dealt
// with by body's own rule and is propagated unchanged.
template <class _Ios, class _Fp>
void __guarded_io(_Ios& s, _Fp&& __body, const bool& __pass) {
  if constexpr (__cfg::exceptions) {
    try {
      static_cast<_Fp&&>(__body)();
    } catch (...) {
      if (__pass)
        throw;
      ::__ycxx::__detail::__ios_access::__set_badbit_quietly(s);
      if (s.exceptions() & std::ios_base::badbit)
        throw;
    }
  } else {
    static_cast<_Fp&&>(__body)();
  }
}
template <class _Ios, class _Fp>
void __guarded_io(_Ios& s, _Fp&& __body) {
  constexpr bool __never = false;
  ::__ycxx::__detail::__guarded_io(s, static_cast<_Fp&&>(__body), __never);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [ios]
template <class __charT, class __traits>
class basic_ios : public ios_base {
public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;

  // [iostate.flags]
  explicit operator bool() const { return !fail(); }
  bool operator!() const { return fail(); }
  iostate rdstate() const { return __load_state(); }
  void clear(iostate state = goodbit) {
    if (__sb_ == nullptr)
      state |= badbit;
    __store_state(state);
    if (state & __except_)
      ::__ycxx::__detail::__raise_ios_failure("std::basic_ios::clear: the stream state matches exceptions()");
  }
  // clear(rdstate() | state), as one atomic OR that is skipped when no bit is new (see load_state).
  void setstate(iostate state) {
    if (__sb_ == nullptr)
      state |= badbit;
    if (__add_state(state) & __except_)
      ::__ycxx::__detail::__raise_ios_failure("std::basic_ios::clear: the stream state matches exceptions()");
  }
  bool good() const { return __load_state() == goodbit; }
  bool eof() const { return (__load_state() & eofbit) != 0; }
  bool fail() const { return (__load_state() & (failbit | badbit)) != 0; }
  bool bad() const { return (__load_state() & badbit) != 0; }
  iostate exceptions() const { return __except_; }
  void exceptions(iostate __y_except) {
    __except_ = __y_except;
    clear(rdstate());
  }

  // [basic.ios.cons]
  explicit basic_ios(basic_streambuf<__charT, __traits>* __sb) { init(__sb); }
  ~basic_ios() override {}
  basic_ios(const basic_ios&) = delete;
  basic_ios& operator=(const basic_ios&) = delete;

  // [basic.ios.members]
  basic_ostream<__charT, __traits>* tie() const { return __tie_; }
  basic_ostream<__charT, __traits>* tie(basic_ostream<__charT, __traits>* __tiestr) {
    basic_ostream<__charT, __traits>* __old = __tie_;
    __tie_ = __tiestr;
    return __old;
  }
  basic_streambuf<__charT, __traits>* rdbuf() const { return __sb_; }
  basic_streambuf<__charT, __traits>* rdbuf(basic_streambuf<__charT, __traits>* __sb) {
    basic_streambuf<__charT, __traits>* __old = __sb_;
    __sb_ = __sb;
    clear();
    return __old;
  }

  basic_ios& copyfmt(const basic_ios& __rhs) {
    if (this == __builtin_addressof(__rhs))
      return *this;
    __call_callbacks(erase_event);
    __copy_base(__rhs);
    __tie_ = __rhs.__tie_;
    __fill_ = __rhs.__fill_;
    __fill_set_ = __rhs.__fill_set_;
    __call_callbacks(copyfmt_event);
    exceptions(__rhs.exceptions());
    return *this;
  }

  char_type fill() const {
    if (!__fill_set_) {
      __fill_ = widen(' ');
      __fill_set_ = true;
    }
    return __fill_;
  }
  char_type fill(char_type __ch) {
    const char_type __old = fill();
    __fill_ = __ch;
    return __old;
  }

  locale imbue(const locale& __loc) {
    locale __old = ios_base::imbue(__loc);
    if (__sb_ != nullptr)
      __sb_->pubimbue(__loc);
    return __old;
  }

  char narrow(char_type c, char __dfault) const { return use_facet<ctype<char_type>>(getloc()).narrow(c, __dfault); }
  char_type widen(char c) const { return use_facet<ctype<char_type>>(getloc()).widen(c); }

protected:
  basic_ios() {}
  void init(basic_streambuf<__charT, __traits>* __sb) {
    __init_base(__sb != nullptr);
    __sb_ = __sb;
    __tie_ = nullptr;
    // [basic.ios.cons] Table: fill() is widen(' ') in the locale at this point; without a
    // ctype<charT> there, it is computed on first use
    __fill_set_ = has_facet<ctype<__charT>>(getloc());
    if (__fill_set_)
      __fill_ = widen(' ');
  }
  void move(basic_ios& __rhs) {
    __move_base(__rhs);
    __tie_ = __rhs.__tie_;
    __rhs.__tie_ = nullptr;
    __fill_ = __rhs.__fill_;
    __fill_set_ = __rhs.__fill_set_;
    __sb_ = nullptr;
  }
  void move(basic_ios&& __rhs) { move(__rhs); }
  void swap(basic_ios& __rhs) noexcept {
    __swap_base(__rhs);
    basic_ostream<__charT, __traits>* t = __tie_;
    __tie_ = __rhs.__tie_;
    __rhs.__tie_ = t;
    const char_type __f = __fill_;
    __fill_ = __rhs.__fill_;
    __rhs.__fill_ = __f;
    const bool __fs = __fill_set_;
    __fill_set_ = __rhs.__fill_set_;
    __rhs.__fill_set_ = __fs;
  }
  void set_rdbuf(basic_streambuf<__charT, __traits>* __sb) { __sb_ = __sb; }

private:
  basic_streambuf<__charT, __traits>* __sb_ = nullptr;
  basic_ostream<__charT, __traits>* __tie_ = nullptr;
  mutable char_type __fill_{};
  mutable bool __fill_set_ = false;
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
