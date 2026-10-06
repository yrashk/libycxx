// libycxx hosted: basic_ostream ([output.streams]) and the inserters other headers declare for
// their types (basic_string, basic_string_view, bitset; ycxx/core/stream_io.hpp declares them).
//
// A character sequence whose output fails (sputn writes fewer characters than asked, or a fill
// character cannot be written) sets badbit, as num_put's failed() does
// ([ostream.inserters.arithmetic]/2) and write() does ([ostream.unformatted]/5).
// The ostream overloads of print / println / vprint_* are in ycxx/hosted/ostream_print.hpp.
#pragma once

#include <ycxx/core/exception.hpp>
#include <ycxx/core/string_view.hpp>
#include <ycxx/hosted/locale_num.hpp>
#include <ycxx/hosted/streambuf.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// The base of basic_syncbuf that the emit_on_flush / noemit_on_flush / flush_emit manipulators
// see ([ostream.manip]/8 Note 1: the Allocator cannot be deduced). A stream buffer is found to
// be one through basic_streambuf's tag, so no RTTI is needed.
template <class __charT, class __traits>
class __syncbuf_base;

// A stream buffer that appends everything written to a string (complex's inserter).
template <class __charT, class __traits>
class __string_outbuf final : public std::basic_streambuf<__charT, __traits> {
public:
  std::basic_string<__charT, __traits> str;

protected:
  typename __traits::int_type overflow(typename __traits::int_type c) override {
    if (!__traits::eq_int_type(c, __traits::eof()))
      str.push_back(__traits::to_char_type(c));
    return __traits::not_eof(c);
  }
  std::streamsize xsputn(const __charT* s, std::streamsize n) override {
    str.append(s, static_cast<std::size_t>(n));
    return n;
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// The extended floating-point types ([basic.extended.fp]) and their conversion rank relative to
// a standard floating-point type (every value of F is a value of G).
template <class _Fp>
concept __is_extended_floating_point = std::is_floating_point_v<_Fp> && !std::is_same_v<_Fp, float> &&
                                     !std::is_same_v<_Fp, double> && !std::is_same_v<_Fp, long double>;
template <class _Fp, class _Gp>
inline constexpr bool __fp_rank_le = std::numeric_limits<_Fp>::digits <= std::numeric_limits<_Gp>::digits &&
                                   std::numeric_limits<_Fp>::max_exponent <= std::numeric_limits<_Gp>::max_exponent &&
                                   std::numeric_limits<_Fp>::min_exponent >= std::numeric_limits<_Gp>::min_exponent;

// Writes n copies of c to sb; false if one could not be written.
template <class __charT, class __traits>
bool __put_fill(std::basic_streambuf<__charT, __traits>* __sb, __charT c, std::streamsize n) {
  __charT block[64];
  for (std::streamsize i = 0; i < 64 && i < n; ++i)
    block[i] = c;
  while (n > 0) {
    const std::streamsize k = n < 64 ? n : 64;
    if (__sb->sputn(block, k) != k)
      return false;
    n -= k;
  }
  return true;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class __charT, class __traits>
class basic_ostream : virtual public basic_ios<__charT, __traits> {
public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;

  explicit basic_ostream(basic_streambuf<char_type, __traits>* __sb) { this->init(__sb); }
  ~basic_ostream() override {}

  class sentry;

  // [ostream.inserters]
  basic_ostream& operator<<(basic_ostream& (*__pf)(basic_ostream&)) { return __pf(*this); }
  basic_ostream& operator<<(basic_ios<__charT, __traits>& (*__pf)(basic_ios<__charT, __traits>&)) {
    __pf(*this);
    return *this;
  }
  basic_ostream& operator<<(ios_base& (*__pf)(ios_base&)) {
    __pf(*this);
    return *this;
  }

  // [ostream.inserters.arithmetic]
  basic_ostream& operator<<(bool n) { return __put_number(n); }
  basic_ostream& operator<<(short n) {
    const ios_base::fmtflags base = this->flags() & ios_base::basefield;
    return __put_number(base == ios_base::oct || base == ios_base::hex ? static_cast<long>(static_cast<unsigned short>(n))
                                                                     : static_cast<long>(n));
  }
  basic_ostream& operator<<(unsigned short n) { return __put_number(static_cast<unsigned long>(n)); }
  basic_ostream& operator<<(int n) {
    const ios_base::fmtflags base = this->flags() & ios_base::basefield;
    return __put_number(base == ios_base::oct || base == ios_base::hex ? static_cast<long>(static_cast<unsigned int>(n))
                                                                     : static_cast<long>(n));
  }
  basic_ostream& operator<<(unsigned int n) { return __put_number(static_cast<unsigned long>(n)); }
  basic_ostream& operator<<(long n) { return __put_number(n); }
  basic_ostream& operator<<(unsigned long n) { return __put_number(n); }
  basic_ostream& operator<<(long long n) { return __put_number(n); }
  basic_ostream& operator<<(unsigned long long n) { return __put_number(n); }
  basic_ostream& operator<<(float __f) { return __put_number(static_cast<double>(__f)); }
  basic_ostream& operator<<(double __f) { return __put_number(__f); }
  basic_ostream& operator<<(long double __f) { return __put_number(__f); }
  // [ostream.inserters.arithmetic]/5: the extended floating-point types of rank at most that
  // of long double.
  template <class _Fp>
    requires __ycxx::__detail::__is_extended_floating_point<_Fp> && __ycxx::__detail::__fp_rank_le<_Fp, long double>
  basic_ostream& operator<<(_Fp __f) {
    if constexpr (__ycxx::__detail::__fp_rank_le<_Fp, double>)
      return __put_number(static_cast<double>(__f));
    else
      return __put_number(static_cast<long double>(__f));
  }
  basic_ostream& operator<<(const void* p) { return __put_number(p); }
  basic_ostream& operator<<(const volatile void* p) { return *this << const_cast<const void*>(p); }
  basic_ostream& operator<<(nullptr_t) { return *this << "nullptr"; }
  basic_ostream& operator<<(basic_streambuf<char_type, __traits>* __sb);

  // [ostream.unformatted]
  basic_ostream& put(char_type c);
  basic_ostream& write(const char_type* s, streamsize n);
  basic_ostream& flush();

  // [ostream.seeks]
  pos_type tellp();
  basic_ostream& seekp(pos_type __pos);
  basic_ostream& seekp(off_type __off, ios_base::seekdir __dir);

protected:
  basic_ostream(const basic_ostream&) = delete;
  basic_ostream(basic_ostream&& __rhs) { this->move(__rhs); }
  basic_ostream& operator=(const basic_ostream&) = delete;
  basic_ostream& operator=(basic_ostream&& __rhs) {
    swap(__rhs);
    return *this;
  }
  void swap(basic_ostream& __rhs) { basic_ios<__charT, __traits>::swap(__rhs); }
  // Used by basic_iostream, whose basic_istream part initializes the virtual base.
  basic_ostream() {}

private:
  template <class _Vp>
  basic_ostream& __put_number(_Vp __v);
};

// [ostream.sentry]
template <class __charT, class __traits>
class basic_ostream<__charT, __traits>::sentry {
  bool __ok_;
  basic_ostream& __os_;

public:
  explicit sentry(basic_ostream& __os) : __ok_(false), __os_(__os) {
    if (__os.good()) {
      if (__os.tie() != nullptr && __os.tie() != __builtin_addressof(__os))
        __os.tie()->flush();
      __ok_ = __os.good();
    }
  }
  ~sentry() {
    if ((__os_.flags() & ios_base::unitbuf) && std::uncaught_exceptions() == 0 && __os_.good()) {
      if constexpr (__ycxx::__detail::__cfg::exceptions) {
        try {
          if (__os_.rdbuf()->pubsync() == -1)
            __ycxx::__detail::__ios_access::__set_badbit_quietly(__os_);
        } catch (...) {
          __ycxx::__detail::__ios_access::__set_badbit_quietly(__os_);
        }
      } else {
        if (__os_.rdbuf()->pubsync() == -1)
          __ycxx::__detail::__ios_access::__set_badbit_quietly(__os_);
      }
    }
  }
  explicit operator bool() const { return __ok_; }
  sentry(const sentry&) = delete;
  sentry& operator=(const sentry&) = delete;
};

template <class __charT, class __traits>
template <class _Vp>
basic_ostream<__charT, __traits>& basic_ostream<__charT, __traits>::__put_number(_Vp __v) {
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      using _It = ostreambuf_iterator<__charT, __traits>;
      if (use_facet<num_put<__charT, _It>>(__ycxx::__detail::__ios_access::__locale_of(*this)).put(_It(*this), *this, this->fill(), __v).failed())
        __err |= ios_base::badbit;
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

template <class __charT, class __traits>
basic_ostream<__charT, __traits>& basic_ostream<__charT, __traits>::operator<<(basic_streambuf<char_type, __traits>* __sb) {
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this}) {
    if (__sb == nullptr) {
      this->setstate(ios_base::badbit);
      return *this;
    }
    streamsize n = 0;
    bool __from_source = false; // an exception from sb, rethrown because failbit is in exceptions()
    // [ostream.inserters]/9: an exception while getting a character from sb sets failbit and is
    // rethrown only if failbit is in exceptions()
    auto __source = [&](auto get) -> int_type {
      if constexpr (__ycxx::__detail::__cfg::exceptions) {
        try {
          return get();
        } catch (...) {
          __err |= ios_base::failbit;
          if (this->exceptions() & ios_base::failbit) {
            __ycxx::__detail::__ios_access::__set_failbit_quietly(*this);
            __from_source = true;
            throw;
          }
          return __traits::eof();
        }
      } else {
        return get();
      }
    };
    __ycxx::__detail::__guarded_io(
        *this,
        [&] {
          basic_streambuf<__charT, __traits>* out = this->rdbuf();
          for (;;) {
            const int_type c = __source([&] { return __sb->sgetc(); });
            if (__traits::eq_int_type(c, __traits::eof()))
              return;
            if (__traits::eq_int_type(out->sputc(__traits::to_char_type(c)), __traits::eof()))
              return;
            ++n;
            if (__traits::eq_int_type(__source([&] { return __sb->sbumpc(); }), __traits::eof()) && (__err & ios_base::failbit))
              return;
          }
        },
        __from_source);
    if (n == 0)
      __err |= ios_base::failbit;
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

template <class __charT, class __traits>
basic_ostream<__charT, __traits>& basic_ostream<__charT, __traits>::put(char_type c) {
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (__traits::eq_int_type(this->rdbuf()->sputc(c), __traits::eof()))
        __err |= ios_base::badbit;
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

template <class __charT, class __traits>
basic_ostream<__charT, __traits>& basic_ostream<__charT, __traits>::write(const char_type* s, streamsize n) {
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (this->rdbuf()->sputn(s, n) != n)
        __err |= ios_base::badbit;
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

template <class __charT, class __traits>
basic_ostream<__charT, __traits>& basic_ostream<__charT, __traits>::flush() {
  if (this->rdbuf() == nullptr)
    return *this;
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (this->rdbuf()->pubsync() == -1)
        __err |= ios_base::badbit;
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

template <class __charT, class __traits>
typename basic_ostream<__charT, __traits>::pos_type basic_ostream<__charT, __traits>::tellp() {
  pos_type r = pos_type(off_type(-1));
  sentry ok{*this};
  if (!this->fail())
    __ycxx::__detail::__guarded_io(*this, [&] { r = this->rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out); });
  return r;
}

template <class __charT, class __traits>
basic_ostream<__charT, __traits>& basic_ostream<__charT, __traits>::seekp(pos_type __pos) {
  ios_base::iostate __err = ios_base::goodbit;
  sentry ok{*this};
  if (!this->fail()) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (this->rdbuf()->pubseekpos(__pos, ios_base::out) == pos_type(off_type(-1)))
        __err |= ios_base::failbit;
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

template <class __charT, class __traits>
basic_ostream<__charT, __traits>& basic_ostream<__charT, __traits>::seekp(off_type __off, ios_base::seekdir __dir) {
  ios_base::iostate __err = ios_base::goodbit;
  sentry ok{*this};
  if (!this->fail()) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (this->rdbuf()->pubseekoff(__off, __dir, ios_base::out) == pos_type(off_type(-1)))
        __err |= ios_base::failbit;
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// [ostream.formatted.reqmts]/3: inserts s[0..n) padded to width() with fill(), then width(0);
// the formatted-output protocol around it (sentry, exceptions).
template <class __charT, class __traits>
std::basic_ostream<__charT, __traits>& __ostream_insert(std::basic_ostream<__charT, __traits>& __os, const __charT* s,
                                                   std::ptrdiff_t n) {
  std::ios_base::iostate __err = std::ios_base::goodbit;
  if (typename std::basic_ostream<__charT, __traits>::sentry ok{__os}) {
    ::__ycxx::__detail::__guarded_io(__os, [&] {
      std::basic_streambuf<__charT, __traits>* __sb = __os.rdbuf();
      const std::streamsize __w = __os.width();
      const std::streamsize __pad = __w > n ? __w - n : 0;
      const bool left = (__os.flags() & std::ios_base::adjustfield) == std::ios_base::left;
      if (!left && __pad != 0 && !::__ycxx::__detail::__put_fill(__sb, __os.fill(), __pad))
        __err |= std::ios_base::badbit;
      else if (__sb->sputn(s, n) != n)
        __err |= std::ios_base::badbit;
      else if (left && __pad != 0 && !::__ycxx::__detail::__put_fill(__sb, __os.fill(), __pad))
        __err |= std::ios_base::badbit;
      __os.width(0);
    });
  }
  if (__err)
    __os.setstate(__err);
  return __os;
}

// The same for a char sequence inserted into a stream of another character type, each
// character widened ([ostream.inserters.character]/4).
template <class __charT, class __traits>
std::basic_ostream<__charT, __traits>& __ostream_insert_widened(std::basic_ostream<__charT, __traits>& __os, const char* s,
                                                          std::ptrdiff_t n) {
  ::__ycxx::__detail::__small_buffer<__charT, 128> __wide(static_cast<std::size_t>(n));
  for (std::ptrdiff_t i = 0; i < n; ++i)
    __wide.get()[i] = __os.widen(s[i]);
  return ::__ycxx::__detail::__ostream_insert(__os, __wide.get(), n);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [ostream.inserters.character]
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& out, __charT c) {
  return ::__ycxx::__detail::__ostream_insert(out, __builtin_addressof(c), 1);
}
template <class __charT, class __traits>
  requires(!is_same_v<__charT, char>)
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& out, char c) {
  const __charT __w = out.widen(c);
  return ::__ycxx::__detail::__ostream_insert(out, __builtin_addressof(__w), 1);
}
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>& out, char c) {
  return ::__ycxx::__detail::__ostream_insert(out, __builtin_addressof(c), 1);
}
template <class __traits>
[[deprecated("signed char / unsigned char stream insertion is deprecated ([depr.ostream.inserters]); use char")]]
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>& out, signed char c) {
  return out << static_cast<char>(c);
}
template <class __traits>
[[deprecated("signed char / unsigned char stream insertion is deprecated ([depr.ostream.inserters]); use char")]]
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>& out, unsigned char c) {
  return out << static_cast<char>(c);
}
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>&, wchar_t) = delete;
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>&, char8_t) = delete;
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>&, char16_t) = delete;
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>&, char32_t) = delete;
template <class __traits>
basic_ostream<wchar_t, __traits>& operator<<(basic_ostream<wchar_t, __traits>&, char8_t) = delete;
template <class __traits>
basic_ostream<wchar_t, __traits>& operator<<(basic_ostream<wchar_t, __traits>&, char16_t) = delete;
template <class __traits>
basic_ostream<wchar_t, __traits>& operator<<(basic_ostream<wchar_t, __traits>&, char32_t) = delete;

template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& out, const __charT* s) {
  __ycxx::__detail::__precondition(s != nullptr, "std::operator<<(basic_ostream&, const charT*): null pointer");
  return ::__ycxx::__detail::__ostream_insert(out, s, static_cast<streamsize>(__traits::length(s)));
}
template <class __charT, class __traits>
  requires(!is_same_v<__charT, char>)
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& out, const char* s) {
  __ycxx::__detail::__precondition(s != nullptr, "std::operator<<(basic_ostream&, const char*): null pointer");
  return ::__ycxx::__detail::__ostream_insert_widened(out, s, static_cast<streamsize>(char_traits<char>::length(s)));
}
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>& out, const char* s) {
  __ycxx::__detail::__precondition(s != nullptr, "std::operator<<(basic_ostream&, const char*): null pointer");
  return ::__ycxx::__detail::__ostream_insert(out, s, static_cast<streamsize>(__traits::length(s)));
}
template <class __traits>
[[deprecated("signed char / unsigned char stream insertion is deprecated ([depr.ostream.inserters]); use char")]]
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>& out, const signed char* s) {
  return out << reinterpret_cast<const char*>(s);
}
template <class __traits>
[[deprecated("signed char / unsigned char stream insertion is deprecated ([depr.ostream.inserters]); use char")]]
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>& out, const unsigned char* s) {
  return out << reinterpret_cast<const char*>(s);
}
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>&, const wchar_t*) = delete;
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>&, const char8_t*) = delete;
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>&, const char16_t*) = delete;
template <class __traits>
basic_ostream<char, __traits>& operator<<(basic_ostream<char, __traits>&, const char32_t*) = delete;
template <class __traits>
basic_ostream<wchar_t, __traits>& operator<<(basic_ostream<wchar_t, __traits>&, const char8_t*) = delete;
template <class __traits>
basic_ostream<wchar_t, __traits>& operator<<(basic_ostream<wchar_t, __traits>&, const char16_t*) = delete;
template <class __traits>
basic_ostream<wchar_t, __traits>& operator<<(basic_ostream<wchar_t, __traits>&, const char32_t*) = delete;

// [ostream.manip]
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& endl(basic_ostream<__charT, __traits>& __os) {
  __os.put(__os.widen('\n'));
  __os.flush();
  return __os;
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& ends(basic_ostream<__charT, __traits>& __os) {
  __os.put(__charT());
  return __os;
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& flush(basic_ostream<__charT, __traits>& __os) {
  __os.flush();
  return __os;
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& emit_on_flush(basic_ostream<__charT, __traits>& __os) {
  if (auto* __buf = __ycxx::__adl_free::__syncbuf_base<__charT, __traits>::__of(__os.rdbuf()))
    __buf->set_emit_on_sync(true);
  return __os;
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& noemit_on_flush(basic_ostream<__charT, __traits>& __os) {
  if (auto* __buf = __ycxx::__adl_free::__syncbuf_base<__charT, __traits>::__of(__os.rdbuf()))
    __buf->set_emit_on_sync(false);
  return __os;
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& flush_emit(basic_ostream<__charT, __traits>& __os) {
  __os.flush();
  if (auto* __buf = __ycxx::__adl_free::__syncbuf_base<__charT, __traits>::__of(__os.rdbuf())) {
    ios_base::iostate __err = ios_base::goodbit;
    if (typename basic_ostream<__charT, __traits>::sentry ok{__os}) {
      __ycxx::__detail::__guarded_io(__os, [&] {
        if (!__buf->emit())
          __err |= ios_base::badbit;
      });
    }
    if (__err)
      __os.setstate(__err);
  }
  return __os;
}

// [ostream.rvalue]
template <class _Ostream, class _Tp>
  // "publicly and unambiguously derived from ios_base": ios_base itself is not
  requires derived_from<_Ostream, ios_base> && (!is_same_v<remove_cv_t<_Ostream>, ios_base>) &&
           requires(_Ostream& __os, const _Tp& __x) { __os << __x; }
_Ostream&& operator<<(_Ostream&& __os, const _Tp& __x) {
  __os << __x;
  return static_cast<_Ostream&&>(__os);
}

// [string.view.io]
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, basic_string_view<__charT, __traits> str) {
  return ::__ycxx::__detail::__ostream_insert(__os, str.data(), static_cast<streamsize>(str.size()));
}

// [string.io]
template <class __charT, class __traits, class _Allocator>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                         const basic_string<__charT, __traits, _Allocator>& str) {
  return ::__ycxx::__detail::__ostream_insert(__os, str.data(), static_cast<streamsize>(str.size()));
}

// [bitset.operators]
template <size_t _Np>
class bitset;
template <class __charT, class __traits, size_t _Np>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const bitset<_Np>& __x) {
  const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__ycxx::__detail::__ios_access::__locale_of(__os));
  return __os << __x.template to_string<__charT, __traits, allocator<__charT>>(__ct.widen('0'), __ct.widen('1'));
}

// [complex.ops]: formatted into a string first, with o's flags, precision and locale (as the
// draft's basic_ostringstream would), so that o's width applies to the whole number.
template <class _Tp>
class complex;
template <class _Tp, class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __o, const complex<_Tp>& __x) {
  __ycxx::__adl_free::__string_outbuf<__charT, __traits> __buf;
  basic_ostream<__charT, __traits> s(__builtin_addressof(__buf));
  s.flags(__o.flags());
  s.imbue(__o.getloc());
  s.precision(__o.precision());
  s << s.widen('(') << __x.real() << s.widen(',') << __x.imag() << s.widen(')');
  return __o << __buf.str;
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

template <class __charT, class __traits>
class __syncbuf_base : public std::basic_streambuf<__charT, __traits> {
public:
  // The syncbuf behind sb, or null.
  static __syncbuf_base* __of(std::basic_streambuf<__charT, __traits>* __sb) noexcept {
    return __sb != nullptr && ::__ycxx::__detail::__streambuf_tag_access::__is_syncbuf(*__sb) ? static_cast<__syncbuf_base*>(__sb)
                                                                                  : nullptr;
  }
  void set_emit_on_sync(bool b) noexcept { __emit_on_sync_ = b; }
  virtual bool emit() = 0;

protected:
  __syncbuf_base() noexcept { ::__ycxx::__detail::__streambuf_tag_access::__set_syncbuf(*this); }
  __syncbuf_base(const __syncbuf_base& __rhs) : std::basic_streambuf<__charT, __traits>(__rhs), __emit_on_sync_(__rhs.__emit_on_sync_) {
    ::__ycxx::__detail::__streambuf_tag_access::__set_syncbuf(*this);
  }
  __syncbuf_base& operator=(const __syncbuf_base&) = default;
  bool __emit_on_sync_ = false;
};

}} // namespace __ycxx::__adl_free
