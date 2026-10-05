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

namespace ycxx::adl_free {
// The base of basic_syncbuf that the emit_on_flush / noemit_on_flush / flush_emit manipulators
// see ([ostream.manip]/8 Note 1: the Allocator cannot be deduced). A stream buffer is found to
// be one through basic_streambuf's tag, so no RTTI is needed.
template <class charT, class traits>
class syncbuf_base;

// A stream buffer that appends everything written to a string (complex's inserter).
template <class charT, class traits>
class string_outbuf final : public std::basic_streambuf<charT, traits> {
public:
  std::basic_string<charT, traits> str;

protected:
  typename traits::int_type overflow(typename traits::int_type c) override {
    if (!traits::eq_int_type(c, traits::eof()))
      str.push_back(traits::to_char_type(c));
    return traits::not_eof(c);
  }
  std::streamsize xsputn(const charT* s, std::streamsize n) override {
    str.append(s, static_cast<std::size_t>(n));
    return n;
  }
};
} // namespace ycxx::adl_free

namespace ycxx::detail {
// The extended floating-point types ([basic.extended.fp]) and their conversion rank relative to
// a standard floating-point type (every value of F is a value of G).
template <class F>
concept is_extended_floating_point = std::is_floating_point_v<F> && !std::is_same_v<F, float> &&
                                     !std::is_same_v<F, double> && !std::is_same_v<F, long double>;
template <class F, class G>
inline constexpr bool fp_rank_le = std::numeric_limits<F>::digits <= std::numeric_limits<G>::digits &&
                                   std::numeric_limits<F>::max_exponent <= std::numeric_limits<G>::max_exponent &&
                                   std::numeric_limits<F>::min_exponent >= std::numeric_limits<G>::min_exponent;

// Writes n copies of c to sb; false if one could not be written.
template <class charT, class traits>
bool put_fill(std::basic_streambuf<charT, traits>* sb, charT c, std::streamsize n) {
  charT block[64];
  for (std::streamsize i = 0; i < 64 && i < n; ++i)
    block[i] = c;
  while (n > 0) {
    const std::streamsize k = n < 64 ? n : 64;
    if (sb->sputn(block, k) != k)
      return false;
    n -= k;
  }
  return true;
}
} // namespace ycxx::detail

namespace std {

template <class charT, class traits>
class basic_ostream : virtual public basic_ios<charT, traits> {
public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;

  explicit basic_ostream(basic_streambuf<char_type, traits>* sb) { this->init(sb); }
  ~basic_ostream() override {}

  class sentry;

  // [ostream.inserters]
  basic_ostream& operator<<(basic_ostream& (*pf)(basic_ostream&)) { return pf(*this); }
  basic_ostream& operator<<(basic_ios<charT, traits>& (*pf)(basic_ios<charT, traits>&)) {
    pf(*this);
    return *this;
  }
  basic_ostream& operator<<(ios_base& (*pf)(ios_base&)) {
    pf(*this);
    return *this;
  }

  // [ostream.inserters.arithmetic]
  basic_ostream& operator<<(bool n) { return put_number(n); }
  basic_ostream& operator<<(short n) {
    const ios_base::fmtflags base = this->flags() & ios_base::basefield;
    return put_number(base == ios_base::oct || base == ios_base::hex ? static_cast<long>(static_cast<unsigned short>(n))
                                                                     : static_cast<long>(n));
  }
  basic_ostream& operator<<(unsigned short n) { return put_number(static_cast<unsigned long>(n)); }
  basic_ostream& operator<<(int n) {
    const ios_base::fmtflags base = this->flags() & ios_base::basefield;
    return put_number(base == ios_base::oct || base == ios_base::hex ? static_cast<long>(static_cast<unsigned int>(n))
                                                                     : static_cast<long>(n));
  }
  basic_ostream& operator<<(unsigned int n) { return put_number(static_cast<unsigned long>(n)); }
  basic_ostream& operator<<(long n) { return put_number(n); }
  basic_ostream& operator<<(unsigned long n) { return put_number(n); }
  basic_ostream& operator<<(long long n) { return put_number(n); }
  basic_ostream& operator<<(unsigned long long n) { return put_number(n); }
  basic_ostream& operator<<(float f) { return put_number(static_cast<double>(f)); }
  basic_ostream& operator<<(double f) { return put_number(f); }
  basic_ostream& operator<<(long double f) { return put_number(f); }
  // [ostream.inserters.arithmetic]/5: the extended floating-point types of rank at most that
  // of long double.
  template <class F>
    requires ycxx::detail::is_extended_floating_point<F> && ycxx::detail::fp_rank_le<F, long double>
  basic_ostream& operator<<(F f) {
    if constexpr (ycxx::detail::fp_rank_le<F, double>)
      return put_number(static_cast<double>(f));
    else
      return put_number(static_cast<long double>(f));
  }
  basic_ostream& operator<<(const void* p) { return put_number(p); }
  basic_ostream& operator<<(const volatile void* p) { return *this << const_cast<const void*>(p); }
  basic_ostream& operator<<(nullptr_t) { return *this << "nullptr"; }
  basic_ostream& operator<<(basic_streambuf<char_type, traits>* sb);

  // [ostream.unformatted]
  basic_ostream& put(char_type c);
  basic_ostream& write(const char_type* s, streamsize n);
  basic_ostream& flush();

  // [ostream.seeks]
  pos_type tellp();
  basic_ostream& seekp(pos_type pos);
  basic_ostream& seekp(off_type off, ios_base::seekdir dir);

protected:
  basic_ostream(const basic_ostream&) = delete;
  basic_ostream(basic_ostream&& rhs) { this->move(rhs); }
  basic_ostream& operator=(const basic_ostream&) = delete;
  basic_ostream& operator=(basic_ostream&& rhs) {
    swap(rhs);
    return *this;
  }
  void swap(basic_ostream& rhs) { basic_ios<charT, traits>::swap(rhs); }
  // Used by basic_iostream, whose basic_istream part initializes the virtual base.
  basic_ostream() {}

private:
  template <class V>
  basic_ostream& put_number(V v);
};

// [ostream.sentry]
template <class charT, class traits>
class basic_ostream<charT, traits>::sentry {
  bool ok_;
  basic_ostream& os_;

public:
  explicit sentry(basic_ostream& os) : ok_(false), os_(os) {
    if (os.good()) {
      if (os.tie() != nullptr && os.tie() != __builtin_addressof(os))
        os.tie()->flush();
      ok_ = os.good();
    }
  }
  ~sentry() {
    if ((os_.flags() & ios_base::unitbuf) && std::uncaught_exceptions() == 0 && os_.good()) {
      if constexpr (ycxx::detail::cfg::exceptions) {
        try {
          if (os_.rdbuf()->pubsync() == -1)
            ycxx::detail::ios_access::set_badbit_quietly(os_);
        } catch (...) {
          ycxx::detail::ios_access::set_badbit_quietly(os_);
        }
      } else {
        if (os_.rdbuf()->pubsync() == -1)
          ycxx::detail::ios_access::set_badbit_quietly(os_);
      }
    }
  }
  explicit operator bool() const { return ok_; }
  sentry(const sentry&) = delete;
  sentry& operator=(const sentry&) = delete;
};

template <class charT, class traits>
template <class V>
basic_ostream<charT, traits>& basic_ostream<charT, traits>::put_number(V v) {
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this}) {
    ycxx::detail::guarded_io(*this, [&] {
      using It = ostreambuf_iterator<charT, traits>;
      if (use_facet<num_put<charT, It>>(ycxx::detail::ios_access::locale_of(*this)).put(It(*this), *this, this->fill(), v).failed())
        err |= ios_base::badbit;
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

template <class charT, class traits>
basic_ostream<charT, traits>& basic_ostream<charT, traits>::operator<<(basic_streambuf<char_type, traits>* sb) {
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this}) {
    if (sb == nullptr) {
      this->setstate(ios_base::badbit);
      return *this;
    }
    streamsize n = 0;
    bool from_source = false; // an exception from sb, rethrown because failbit is in exceptions()
    // [ostream.inserters]/9: an exception while getting a character from sb sets failbit and is
    // rethrown only if failbit is in exceptions()
    auto source = [&](auto get) -> int_type {
      if constexpr (ycxx::detail::cfg::exceptions) {
        try {
          return get();
        } catch (...) {
          err |= ios_base::failbit;
          if (this->exceptions() & ios_base::failbit) {
            ycxx::detail::ios_access::set_failbit_quietly(*this);
            from_source = true;
            throw;
          }
          return traits::eof();
        }
      } else {
        return get();
      }
    };
    ycxx::detail::guarded_io(
        *this,
        [&] {
          basic_streambuf<charT, traits>* out = this->rdbuf();
          for (;;) {
            const int_type c = source([&] { return sb->sgetc(); });
            if (traits::eq_int_type(c, traits::eof()))
              return;
            if (traits::eq_int_type(out->sputc(traits::to_char_type(c)), traits::eof()))
              return;
            ++n;
            if (traits::eq_int_type(source([&] { return sb->sbumpc(); }), traits::eof()) && (err & ios_base::failbit))
              return;
          }
        },
        from_source);
    if (n == 0)
      err |= ios_base::failbit;
  }
  if (err)
    this->setstate(err);
  return *this;
}

template <class charT, class traits>
basic_ostream<charT, traits>& basic_ostream<charT, traits>::put(char_type c) {
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this}) {
    ycxx::detail::guarded_io(*this, [&] {
      if (traits::eq_int_type(this->rdbuf()->sputc(c), traits::eof()))
        err |= ios_base::badbit;
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

template <class charT, class traits>
basic_ostream<charT, traits>& basic_ostream<charT, traits>::write(const char_type* s, streamsize n) {
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this}) {
    ycxx::detail::guarded_io(*this, [&] {
      if (this->rdbuf()->sputn(s, n) != n)
        err |= ios_base::badbit;
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

template <class charT, class traits>
basic_ostream<charT, traits>& basic_ostream<charT, traits>::flush() {
  if (this->rdbuf() == nullptr)
    return *this;
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this}) {
    ycxx::detail::guarded_io(*this, [&] {
      if (this->rdbuf()->pubsync() == -1)
        err |= ios_base::badbit;
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

template <class charT, class traits>
typename basic_ostream<charT, traits>::pos_type basic_ostream<charT, traits>::tellp() {
  pos_type r = pos_type(off_type(-1));
  sentry ok{*this};
  if (!this->fail())
    ycxx::detail::guarded_io(*this, [&] { r = this->rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out); });
  return r;
}

template <class charT, class traits>
basic_ostream<charT, traits>& basic_ostream<charT, traits>::seekp(pos_type pos) {
  ios_base::iostate err = ios_base::goodbit;
  sentry ok{*this};
  if (!this->fail()) {
    ycxx::detail::guarded_io(*this, [&] {
      if (this->rdbuf()->pubseekpos(pos, ios_base::out) == pos_type(off_type(-1)))
        err |= ios_base::failbit;
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

template <class charT, class traits>
basic_ostream<charT, traits>& basic_ostream<charT, traits>::seekp(off_type off, ios_base::seekdir dir) {
  ios_base::iostate err = ios_base::goodbit;
  sentry ok{*this};
  if (!this->fail()) {
    ycxx::detail::guarded_io(*this, [&] {
      if (this->rdbuf()->pubseekoff(off, dir, ios_base::out) == pos_type(off_type(-1)))
        err |= ios_base::failbit;
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

} // namespace std

namespace ycxx::detail {

// [ostream.formatted.reqmts]/3: inserts s[0..n) padded to width() with fill(), then width(0);
// the formatted-output protocol around it (sentry, exceptions).
template <class charT, class traits>
std::basic_ostream<charT, traits>& ostream_insert(std::basic_ostream<charT, traits>& os, const charT* s,
                                                   std::ptrdiff_t n) {
  std::ios_base::iostate err = std::ios_base::goodbit;
  if (typename std::basic_ostream<charT, traits>::sentry ok{os}) {
    ::ycxx::detail::guarded_io(os, [&] {
      std::basic_streambuf<charT, traits>* sb = os.rdbuf();
      const std::streamsize w = os.width();
      const std::streamsize pad = w > n ? w - n : 0;
      const bool left = (os.flags() & std::ios_base::adjustfield) == std::ios_base::left;
      if (!left && pad != 0 && !::ycxx::detail::put_fill(sb, os.fill(), pad))
        err |= std::ios_base::badbit;
      else if (sb->sputn(s, n) != n)
        err |= std::ios_base::badbit;
      else if (left && pad != 0 && !::ycxx::detail::put_fill(sb, os.fill(), pad))
        err |= std::ios_base::badbit;
      os.width(0);
    });
  }
  if (err)
    os.setstate(err);
  return os;
}

// The same for a char sequence inserted into a stream of another character type, each
// character widened ([ostream.inserters.character]/4).
template <class charT, class traits>
std::basic_ostream<charT, traits>& ostream_insert_widened(std::basic_ostream<charT, traits>& os, const char* s,
                                                          std::ptrdiff_t n) {
  ::ycxx::detail::small_buffer<charT, 128> wide(static_cast<std::size_t>(n));
  for (std::ptrdiff_t i = 0; i < n; ++i)
    wide.get()[i] = os.widen(s[i]);
  return ::ycxx::detail::ostream_insert(os, wide.get(), n);
}

} // namespace ycxx::detail

namespace std {

// [ostream.inserters.character]
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& out, charT c) {
  return ::ycxx::detail::ostream_insert(out, __builtin_addressof(c), 1);
}
template <class charT, class traits>
  requires(!is_same_v<charT, char>)
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& out, char c) {
  const charT w = out.widen(c);
  return ::ycxx::detail::ostream_insert(out, __builtin_addressof(w), 1);
}
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>& out, char c) {
  return ::ycxx::detail::ostream_insert(out, __builtin_addressof(c), 1);
}
template <class traits>
[[deprecated("signed char / unsigned char stream insertion is deprecated ([depr.ostream.inserters]); use char")]]
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>& out, signed char c) {
  return out << static_cast<char>(c);
}
template <class traits>
[[deprecated("signed char / unsigned char stream insertion is deprecated ([depr.ostream.inserters]); use char")]]
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>& out, unsigned char c) {
  return out << static_cast<char>(c);
}
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>&, wchar_t) = delete;
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>&, char8_t) = delete;
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>&, char16_t) = delete;
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>&, char32_t) = delete;
template <class traits>
basic_ostream<wchar_t, traits>& operator<<(basic_ostream<wchar_t, traits>&, char8_t) = delete;
template <class traits>
basic_ostream<wchar_t, traits>& operator<<(basic_ostream<wchar_t, traits>&, char16_t) = delete;
template <class traits>
basic_ostream<wchar_t, traits>& operator<<(basic_ostream<wchar_t, traits>&, char32_t) = delete;

template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& out, const charT* s) {
  ycxx::detail::precondition(s != nullptr, "std::operator<<(basic_ostream&, const charT*): null pointer");
  return ::ycxx::detail::ostream_insert(out, s, static_cast<streamsize>(traits::length(s)));
}
template <class charT, class traits>
  requires(!is_same_v<charT, char>)
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& out, const char* s) {
  ycxx::detail::precondition(s != nullptr, "std::operator<<(basic_ostream&, const char*): null pointer");
  return ::ycxx::detail::ostream_insert_widened(out, s, static_cast<streamsize>(char_traits<char>::length(s)));
}
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>& out, const char* s) {
  ycxx::detail::precondition(s != nullptr, "std::operator<<(basic_ostream&, const char*): null pointer");
  return ::ycxx::detail::ostream_insert(out, s, static_cast<streamsize>(traits::length(s)));
}
template <class traits>
[[deprecated("signed char / unsigned char stream insertion is deprecated ([depr.ostream.inserters]); use char")]]
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>& out, const signed char* s) {
  return out << reinterpret_cast<const char*>(s);
}
template <class traits>
[[deprecated("signed char / unsigned char stream insertion is deprecated ([depr.ostream.inserters]); use char")]]
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>& out, const unsigned char* s) {
  return out << reinterpret_cast<const char*>(s);
}
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>&, const wchar_t*) = delete;
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>&, const char8_t*) = delete;
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>&, const char16_t*) = delete;
template <class traits>
basic_ostream<char, traits>& operator<<(basic_ostream<char, traits>&, const char32_t*) = delete;
template <class traits>
basic_ostream<wchar_t, traits>& operator<<(basic_ostream<wchar_t, traits>&, const char8_t*) = delete;
template <class traits>
basic_ostream<wchar_t, traits>& operator<<(basic_ostream<wchar_t, traits>&, const char16_t*) = delete;
template <class traits>
basic_ostream<wchar_t, traits>& operator<<(basic_ostream<wchar_t, traits>&, const char32_t*) = delete;

// [ostream.manip]
template <class charT, class traits>
basic_ostream<charT, traits>& endl(basic_ostream<charT, traits>& os) {
  os.put(os.widen('\n'));
  os.flush();
  return os;
}
template <class charT, class traits>
basic_ostream<charT, traits>& ends(basic_ostream<charT, traits>& os) {
  os.put(charT());
  return os;
}
template <class charT, class traits>
basic_ostream<charT, traits>& flush(basic_ostream<charT, traits>& os) {
  os.flush();
  return os;
}
template <class charT, class traits>
basic_ostream<charT, traits>& emit_on_flush(basic_ostream<charT, traits>& os) {
  if (auto* buf = ycxx::adl_free::syncbuf_base<charT, traits>::of(os.rdbuf()))
    buf->set_emit_on_sync(true);
  return os;
}
template <class charT, class traits>
basic_ostream<charT, traits>& noemit_on_flush(basic_ostream<charT, traits>& os) {
  if (auto* buf = ycxx::adl_free::syncbuf_base<charT, traits>::of(os.rdbuf()))
    buf->set_emit_on_sync(false);
  return os;
}
template <class charT, class traits>
basic_ostream<charT, traits>& flush_emit(basic_ostream<charT, traits>& os) {
  os.flush();
  if (auto* buf = ycxx::adl_free::syncbuf_base<charT, traits>::of(os.rdbuf())) {
    ios_base::iostate err = ios_base::goodbit;
    if (typename basic_ostream<charT, traits>::sentry ok{os}) {
      ycxx::detail::guarded_io(os, [&] {
        if (!buf->emit())
          err |= ios_base::badbit;
      });
    }
    if (err)
      os.setstate(err);
  }
  return os;
}

// [ostream.rvalue]
template <class Ostream, class T>
  // "publicly and unambiguously derived from ios_base": ios_base itself is not
  requires derived_from<Ostream, ios_base> && (!is_same_v<remove_cv_t<Ostream>, ios_base>) &&
           requires(Ostream& os, const T& x) { os << x; }
Ostream&& operator<<(Ostream&& os, const T& x) {
  os << x;
  return static_cast<Ostream&&>(os);
}

// [string.view.io]
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, basic_string_view<charT, traits> str) {
  return ::ycxx::detail::ostream_insert(os, str.data(), static_cast<streamsize>(str.size()));
}

// [string.io]
template <class charT, class traits, class Allocator>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                         const basic_string<charT, traits, Allocator>& str) {
  return ::ycxx::detail::ostream_insert(os, str.data(), static_cast<streamsize>(str.size()));
}

// [bitset.operators]
template <size_t N>
class bitset;
template <class charT, class traits, size_t N>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const bitset<N>& x) {
  const ctype<charT>& ct = use_facet<ctype<charT>>(ycxx::detail::ios_access::locale_of(os));
  return os << x.template to_string<charT, traits, allocator<charT>>(ct.widen('0'), ct.widen('1'));
}

// [complex.ops]: formatted into a string first, with o's flags, precision and locale (as the
// draft's basic_ostringstream would), so that o's width applies to the whole number.
template <class T>
class complex;
template <class T, class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& o, const complex<T>& x) {
  ycxx::adl_free::string_outbuf<charT, traits> buf;
  basic_ostream<charT, traits> s(__builtin_addressof(buf));
  s.flags(o.flags());
  s.imbue(o.getloc());
  s.precision(o.precision());
  s << s.widen('(') << x.real() << s.widen(',') << x.imag() << s.widen(')');
  return o << buf.str;
}

} // namespace std

namespace ycxx::adl_free {

template <class charT, class traits>
class syncbuf_base : public std::basic_streambuf<charT, traits> {
public:
  // The syncbuf behind sb, or null.
  static syncbuf_base* of(std::basic_streambuf<charT, traits>* sb) noexcept {
    return sb != nullptr && ::ycxx::detail::streambuf_tag_access::is_syncbuf(*sb) ? static_cast<syncbuf_base*>(sb)
                                                                                  : nullptr;
  }
  void set_emit_on_sync(bool b) noexcept { emit_on_sync_ = b; }
  virtual bool emit() = 0;

protected:
  syncbuf_base() noexcept { ::ycxx::detail::streambuf_tag_access::set_syncbuf(*this); }
  syncbuf_base(const syncbuf_base& rhs) : std::basic_streambuf<charT, traits>(rhs), emit_on_sync_(rhs.emit_on_sync_) {
    ::ycxx::detail::streambuf_tag_access::set_syncbuf(*this);
  }
  syncbuf_base& operator=(const syncbuf_base&) = default;
  bool emit_on_sync_ = false;
};

} // namespace ycxx::adl_free
