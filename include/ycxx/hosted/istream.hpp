// libycxx hosted: basic_istream, basic_iostream and ws ([input.streams]), and the extractors
// other headers declare for their types (basic_string, getline, bitset).
//
// Every input function follows [istream.formatted.reqmts] / [istream.unformatted]: it collects
// a local error state, runs the extraction under guarded_io (an exception sets badbit in the
// stream state without throwing failure, and is rethrown if badbit is in exceptions()), and
// calls setstate with the local state at the end.
#pragma once

#include <ycxx/hosted/ostream.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class __charT, class __traits>
class basic_istream : virtual public basic_ios<__charT, __traits> {
public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;

  explicit basic_istream(basic_streambuf<__charT, __traits>* __sb) : __gcount_(0) { this->init(__sb); }
  ~basic_istream() override {}

  class sentry;

  // [istream.extractors]
  basic_istream& operator>>(basic_istream& (*__pf)(basic_istream&)) { return __pf(*this); }
  basic_istream& operator>>(basic_ios<__charT, __traits>& (*__pf)(basic_ios<__charT, __traits>&)) {
    __pf(*this);
    return *this;
  }
  basic_istream& operator>>(ios_base& (*__pf)(ios_base&)) {
    __pf(*this);
    return *this;
  }

  // [istream.formatted.arithmetic]
  basic_istream& operator>>(bool& n) { return __get_number(n); }
  basic_istream& operator>>(short& n) { return __get_narrowed(n); }
  basic_istream& operator>>(unsigned short& n) { return __get_number(n); }
  basic_istream& operator>>(int& n) { return __get_narrowed(n); }
  basic_istream& operator>>(unsigned int& n) { return __get_number(n); }
  basic_istream& operator>>(long& n) { return __get_number(n); }
  basic_istream& operator>>(unsigned long& n) { return __get_number(n); }
  basic_istream& operator>>(long long& n) { return __get_number(n); }
  basic_istream& operator>>(unsigned long long& n) { return __get_number(n); }
  basic_istream& operator>>(float& __f) { return __get_number(__f); }
  basic_istream& operator>>(double& __f) { return __get_number(__f); }
  basic_istream& operator>>(long double& __f) { return __get_number(__f); }
  template <class _Fp>
    requires __ycxx::__detail::__is_extended_floating_point<_Fp> && __ycxx::__detail::__fp_rank_le<_Fp, long double>
  basic_istream& operator>>(_Fp& __f);
  basic_istream& operator>>(void*& p) { return __get_number(p); }
  basic_istream& operator>>(basic_streambuf<char_type, __traits>* __sb);

  // [istream.unformatted]
  streamsize gcount() const { return __atomic_load_n(&__gcount_, __ATOMIC_RELAXED); }
  int_type get();
  basic_istream& get(char_type& c);
  basic_istream& get(char_type* s, streamsize n) { return get(s, n, this->widen('\n')); }
  basic_istream& get(char_type* s, streamsize n, char_type __delim);
  basic_istream& get(basic_streambuf<char_type, __traits>& __sb) { return get(__sb, this->widen('\n')); }
  basic_istream& get(basic_streambuf<char_type, __traits>& __sb, char_type __delim);
  basic_istream& getline(char_type* s, streamsize n) { return getline(s, n, this->widen('\n')); }
  basic_istream& getline(char_type* s, streamsize n, char_type __delim);
  basic_istream& ignore(streamsize n = 1, int_type __delim = __traits::eof());
  // A template (exactly char_type is deduced), so that ignore(n, -1L) still picks the int_type
  // overload instead of being ambiguous.
  template <class _Cp>
    requires is_same_v<_Cp, char_type> && is_same_v<char_type, char>
  basic_istream& ignore(streamsize n, _Cp __delim) {
    return ignore(n, __traits::to_int_type(__delim));
  }
  int_type peek();
  basic_istream& read(char_type* s, streamsize n);
  streamsize readsome(char_type* s, streamsize n);
  basic_istream& putback(char_type c);
  basic_istream& unget();
  int sync();
  pos_type tellg();
  basic_istream& seekg(pos_type __pos);
  basic_istream& seekg(off_type __off, ios_base::seekdir __dir);

protected:
  basic_istream(const basic_istream&) = delete;
  basic_istream(basic_istream&& __rhs) : __gcount_(__rhs.__gcount_) {
    this->move(__rhs);
    __rhs.__gcount_ = 0;
  }
  basic_istream& operator=(const basic_istream&) = delete;
  basic_istream& operator=(basic_istream&& __rhs) {
    swap(__rhs);
    return *this;
  }
  void swap(basic_istream& __rhs) {
    basic_ios<__charT, __traits>::swap(__rhs);
    const streamsize __g = __gcount_;
    __gcount_ = __rhs.__gcount_;
    __rhs.__gcount_ = __g;
  }

private:
  template <class _Vp>
  basic_istream& __get_number(_Vp& __v);
  template <class _Vp>
  basic_istream& __get_narrowed(_Vp& __v);
  // gcount_ is read and written with relaxed atomic operations (ordinary loads and stores on the
  // supported targets): every unformatted input function stores it, and those functions may run
  // concurrently on a synchronized standard stream object ([iostream.objects.overview]/7;
  // DECISIONS §7).
  void __set_gcount(streamsize n) noexcept { __atomic_store_n(&__gcount_, n, __ATOMIC_RELAXED); }
  // the end of an unformatted input function: the count, then setstate
  void finish(ios_base::iostate __err, streamsize count) {
    __set_gcount(count);
    if (__err)
      this->setstate(__err);
  }

  streamsize __gcount_ = 0;
};

// [istream.sentry]
template <class __charT, class __traits>
class basic_istream<__charT, __traits>::sentry {
  bool __ok_;

public:
  explicit sentry(basic_istream& is, bool noskipws = false) : __ok_(false) {
    if (!is.good()) {
      is.setstate(ios_base::failbit);
      return;
    }
    if (is.tie() != nullptr)
      is.tie()->flush();
    if (!noskipws && (is.flags() & ios_base::skipws)) {
      ios_base::iostate __err = ios_base::goodbit;
      __ycxx::__detail::__guarded_io(is, [&] {
        const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__ycxx::__detail::__ios_access::__locale_of(is));
        basic_streambuf<__charT, __traits>* __sb = is.rdbuf();
        for (int_type c = __sb->sgetc();; c = __sb->snextc()) {
          if (__traits::eq_int_type(c, __traits::eof())) {
            __err |= ios_base::failbit | ios_base::eofbit;
            break;
          }
          if (!__ct.is(ctype_base::space, __traits::to_char_type(c)))
            break;
        }
      });
      if (__err)
        is.setstate(__err);
    }
    __ok_ = is.good();
  }
  ~sentry() {}
  explicit operator bool() const { return __ok_; }
  sentry(const sentry&) = delete;
  sentry& operator=(const sentry&) = delete;
};

template <class __charT, class __traits>
template <class _Vp>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::__get_number(_Vp& __v) {
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      using _It = istreambuf_iterator<__charT, __traits>;
      use_facet<num_get<__charT, _It>>(__ycxx::__detail::__ios_access::__locale_of(*this)).get(_It(*this), _It(), *this, __err, __v);
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

// [istream.formatted.arithmetic]/2-3: short and int through long, clamped.
template <class __charT, class __traits>
template <class _Vp>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::__get_narrowed(_Vp& __v) {
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      using _It = istreambuf_iterator<__charT, __traits>;
      long __lval = 0;
      use_facet<num_get<__charT, _It>>(__ycxx::__detail::__ios_access::__locale_of(*this)).get(_It(*this), _It(), *this, __err, __lval);
      if (__lval < numeric_limits<_Vp>::min()) {
        __err |= ios_base::failbit;
        __v = numeric_limits<_Vp>::min();
      } else if (numeric_limits<_Vp>::max() < __lval) {
        __err |= ios_base::failbit;
        __v = numeric_limits<_Vp>::max();
      } else {
        __v = static_cast<_Vp>(__lval);
      }
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

// [istream.formatted.arithmetic]/4-6
template <class __charT, class __traits>
template <class _Fp>
  requires __ycxx::__detail::__is_extended_floating_point<_Fp> && __ycxx::__detail::__fp_rank_le<_Fp, long double>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::operator>>(_Fp& __val) {
  using _FP = conditional_t<__ycxx::__detail::__fp_rank_le<_Fp, float>, float,
                           conditional_t<__ycxx::__detail::__fp_rank_le<_Fp, double>, double, long double>>;
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      using _It = istreambuf_iterator<__charT, __traits>;
      _FP __fval = 0;
      use_facet<num_get<__charT, _It>>(__ycxx::__detail::__ios_access::__locale_of(*this)).get(_It(*this), _It(), *this, __err, __fval);
      if (__fval < -static_cast<_FP>(numeric_limits<_Fp>::max())) {
        __err |= ios_base::failbit;
        __val = -numeric_limits<_Fp>::max();
      } else if (static_cast<_FP>(numeric_limits<_Fp>::max()) < __fval) {
        __err |= ios_base::failbit;
        __val = numeric_limits<_Fp>::max();
      } else {
        __val = static_cast<_Fp>(__fval);
      }
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

// [istream.extractors]/14-15
template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::operator>>(basic_streambuf<char_type, __traits>* __sb) {
  ios_base::iostate __err = ios_base::goodbit;
  streamsize n = 0;
  if (__sb == nullptr) {
    __set_gcount(0);
    this->setstate(ios_base::failbit);
    return *this;
  }
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      basic_streambuf<__charT, __traits>* in = this->rdbuf();
      for (int_type c = in->sgetc();; c = in->snextc()) {
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        // an exception from the output sequence is caught (14.3)
        bool inserted = false;
        if constexpr (__ycxx::__detail::__cfg::exceptions) {
          try {
            inserted = !__traits::eq_int_type(__sb->sputc(__traits::to_char_type(c)), __traits::eof());
          } catch (...) {
          }
        } else {
          inserted = !__traits::eq_int_type(__sb->sputc(__traits::to_char_type(c)), __traits::eof());
        }
        if (!inserted)
          break;
        ++n;
      }
    });
  }
  if (n == 0)
    __err |= ios_base::failbit;
  finish(__err, n);
  return *this;
}

template <class __charT, class __traits>
typename basic_istream<__charT, __traits>::int_type basic_istream<__charT, __traits>::get() {
  ios_base::iostate __err = ios_base::goodbit;
  int_type c = __traits::eof();
  streamsize n = 0;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      c = this->rdbuf()->sbumpc();
      if (__traits::eq_int_type(c, __traits::eof()))
        __err |= ios_base::eofbit | ios_base::failbit;
      else
        n = 1;
    });
  }
  finish(__err, n);
  return c;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::get(char_type& c) {
  const int_type r = get();
  if (!__traits::eq_int_type(r, __traits::eof()))
    c = __traits::to_char_type(r);
  return *this;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::get(char_type* s, streamsize n, char_type __delim) {
  ios_base::iostate __err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      basic_streambuf<__charT, __traits>* __sb = this->rdbuf();
      for (int_type c = __sb->sgetc(); count + 1 < n; c = __sb->snextc()) {
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        if (__traits::eq(__traits::to_char_type(c), __delim))
          break;
        s[count++] = __traits::to_char_type(c);
      }
    });
  }
  if (n > 0)
    s[count] = __charT();
  if (count == 0)
    __err |= ios_base::failbit;
  finish(__err, count);
  return *this;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::get(basic_streambuf<char_type, __traits>& __sb,
                                                                char_type __delim) {
  ios_base::iostate __err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      basic_streambuf<__charT, __traits>* in = this->rdbuf();
      for (int_type c = in->sgetc();; c = in->snextc()) {
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        if (__traits::eq(__traits::to_char_type(c), __delim))
          break;
        // an exception from sb is caught but not rethrown (13.4)
        bool inserted = false;
        if constexpr (__ycxx::__detail::__cfg::exceptions) {
          try {
            inserted = !__traits::eq_int_type(__sb.sputc(__traits::to_char_type(c)), __traits::eof());
          } catch (...) {
          }
        } else {
          inserted = !__traits::eq_int_type(__sb.sputc(__traits::to_char_type(c)), __traits::eof());
        }
        if (!inserted)
          break;
        ++count;
      }
    });
  }
  if (count == 0)
    __err |= ios_base::failbit;
  finish(__err, count);
  return *this;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::getline(char_type* s, streamsize n, char_type __delim) {
  ios_base::iostate __err = ios_base::goodbit;
  streamsize count = 0, __stored = 0;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      basic_streambuf<__charT, __traits>* __sb = this->rdbuf();
      for (int_type c = __sb->sgetc();; c = __sb->snextc()) {
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        if (__traits::eq(__traits::to_char_type(c), __delim)) {
          ++count; // extracted, not stored
          __sb->sbumpc();
          break;
        }
        if (n < 1 || __stored + 1 >= n) {
          __err |= ios_base::failbit;
          break;
        }
        s[__stored++] = __traits::to_char_type(c);
        ++count;
      }
    });
  }
  if (n > 0)
    s[__stored] = __charT();
  if (count == 0)
    __err |= ios_base::failbit;
  finish(__err, count);
  return *this;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::ignore(streamsize n, int_type __delim) {
  ios_base::iostate __err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      basic_streambuf<__charT, __traits>* __sb = this->rdbuf();
      const bool __unlimited = n == numeric_limits<streamsize>::max();
      while (__unlimited || count < n) {
        const int_type c = __sb->sbumpc();
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        if (count != numeric_limits<streamsize>::max())
          ++count;
        if (__traits::eq_int_type(c, __delim))
          break;
      }
    });
  }
  finish(__err, count);
  return *this;
}

template <class __charT, class __traits>
typename basic_istream<__charT, __traits>::int_type basic_istream<__charT, __traits>::peek() {
  ios_base::iostate __err = ios_base::goodbit;
  int_type c = __traits::eof();
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      c = this->rdbuf()->sgetc();
      if (__traits::eq_int_type(c, __traits::eof()))
        __err |= ios_base::eofbit;
    });
  }
  finish(__err, 0);
  return c;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::read(char_type* s, streamsize n) {
  ios_base::iostate __err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      count = this->rdbuf()->sgetn(s, n);
      if (count != n)
        __err |= ios_base::failbit | ios_base::eofbit;
    });
  } else {
    __err |= ios_base::failbit;
  }
  finish(__err, count);
  return *this;
}

template <class __charT, class __traits>
streamsize basic_istream<__charT, __traits>::readsome(char_type* s, streamsize n) {
  ios_base::iostate __err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      const streamsize __avail = this->rdbuf()->in_avail();
      if (__avail == -1)
        __err |= ios_base::eofbit;
      else if (__avail > 0)
        count = this->rdbuf()->sgetn(s, __avail < n ? __avail : n);
    });
  } else {
    __err |= ios_base::failbit;
  }
  finish(__err, count);
  return count;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::putback(char_type c) {
  this->clear(this->rdstate() & ~ios_base::eofbit);
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (this->rdbuf() == nullptr || __traits::eq_int_type(this->rdbuf()->sputbackc(c), __traits::eof()))
        __err |= ios_base::badbit;
    });
  } else {
    __err |= ios_base::failbit;
  }
  finish(__err, 0);
  return *this;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::unget() {
  this->clear(this->rdstate() & ~ios_base::eofbit);
  ios_base::iostate __err = ios_base::goodbit;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (this->rdbuf() == nullptr || __traits::eq_int_type(this->rdbuf()->sungetc(), __traits::eof()))
        __err |= ios_base::badbit;
    });
  } else {
    __err |= ios_base::failbit;
  }
  finish(__err, 0);
  return *this;
}

template <class __charT, class __traits>
int basic_istream<__charT, __traits>::sync() {
  ios_base::iostate __err = ios_base::goodbit;
  int r = 0;
  if (sentry ok{*this, true}) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (this->rdbuf() == nullptr) {
        r = -1;
      } else if (this->rdbuf()->pubsync() == -1) {
        __err |= ios_base::badbit;
        r = -1;
      }
    });
  } else {
    r = -1;
  }
  if (__err)
    this->setstate(__err);
  return r;
}

template <class __charT, class __traits>
typename basic_istream<__charT, __traits>::pos_type basic_istream<__charT, __traits>::tellg() {
  pos_type r = pos_type(off_type(-1));
  sentry ok{*this, true};
  if (!this->fail())
    __ycxx::__detail::__guarded_io(*this, [&] { r = this->rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in); });
  return r;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::seekg(pos_type __pos) {
  this->clear(this->rdstate() & ~ios_base::eofbit);
  ios_base::iostate __err = ios_base::goodbit;
  sentry ok{*this, true};
  if (!this->fail()) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (this->rdbuf()->pubseekpos(__pos, ios_base::in) == pos_type(off_type(-1)))
        __err |= ios_base::failbit;
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& basic_istream<__charT, __traits>::seekg(off_type __off, ios_base::seekdir __dir) {
  this->clear(this->rdstate() & ~ios_base::eofbit);
  ios_base::iostate __err = ios_base::goodbit;
  sentry ok{*this, true};
  if (!this->fail()) {
    __ycxx::__detail::__guarded_io(*this, [&] {
      if (this->rdbuf()->pubseekoff(__off, __dir, ios_base::in) == pos_type(off_type(-1)))
        __err |= ios_base::failbit;
    });
  }
  if (__err)
    this->setstate(__err);
  return *this;
}

// [istream.extractors]/7-13
template <class __charT, class __traits, size_t _Np>
basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& in, __charT (&s)[_Np]) {
  ios_base::iostate __err = ios_base::goodbit;
  size_t count = 0;
  if (typename basic_istream<__charT, __traits>::sentry ok{in}) {
    __ycxx::__detail::__guarded_io(in, [&] {
      const streamsize __w = in.width();
      const size_t n = __w > 0 && static_cast<size_t>(__w) < _Np ? static_cast<size_t>(__w) : _Np;
      const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__ycxx::__detail::__ios_access::__locale_of(in));
      basic_streambuf<__charT, __traits>* __sb = in.rdbuf();
      for (typename __traits::int_type c = __sb->sgetc(); count + 1 < n; c = __sb->snextc()) {
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        if (__ct.is(ctype_base::space, __traits::to_char_type(c)))
          break;
        s[count++] = __traits::to_char_type(c);
      }
      s[count] = __charT();
      in.width(0);
    });
  }
  if (count == 0)
    __err |= ios_base::failbit;
  if (__err)
    in.setstate(__err);
  return in;
}
template <class __traits, size_t _Np>
[[deprecated("signed char / unsigned char stream extraction is deprecated ([depr.istream.extractors]); use char")]]
basic_istream<char, __traits>& operator>>(basic_istream<char, __traits>& in, unsigned char (&s)[_Np]) {
  return in >> reinterpret_cast<char(&)[_Np]>(s);
}
template <class __traits, size_t _Np>
[[deprecated("signed char / unsigned char stream extraction is deprecated ([depr.istream.extractors]); use char")]]
basic_istream<char, __traits>& operator>>(basic_istream<char, __traits>& in, signed char (&s)[_Np]) {
  return in >> reinterpret_cast<char(&)[_Np]>(s);
}

template <class __charT, class __traits>
basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& in, __charT& c) {
  ios_base::iostate __err = ios_base::goodbit;
  if (typename basic_istream<__charT, __traits>::sentry ok{in}) {
    __ycxx::__detail::__guarded_io(in, [&] {
      const typename __traits::int_type r = in.rdbuf()->sbumpc();
      if (__traits::eq_int_type(r, __traits::eof()))
        __err |= ios_base::eofbit | ios_base::failbit;
      else
        c = __traits::to_char_type(r);
    });
  }
  if (__err)
    in.setstate(__err);
  return in;
}
template <class __traits>
[[deprecated("signed char / unsigned char stream extraction is deprecated ([depr.istream.extractors]); use char")]]
basic_istream<char, __traits>& operator>>(basic_istream<char, __traits>& in, unsigned char& c) {
  return in >> reinterpret_cast<char&>(c);
}
template <class __traits>
[[deprecated("signed char / unsigned char stream extraction is deprecated ([depr.istream.extractors]); use char")]]
basic_istream<char, __traits>& operator>>(basic_istream<char, __traits>& in, signed char& c) {
  return in >> reinterpret_cast<char&>(c);
}

// [istream.manip]
template <class __charT, class __traits>
basic_istream<__charT, __traits>& ws(basic_istream<__charT, __traits>& is) {
  ios_base::iostate __err = ios_base::goodbit;
  if (typename basic_istream<__charT, __traits>::sentry ok{is, true}) {
    __ycxx::__detail::__guarded_io(is, [&] {
      const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__ycxx::__detail::__ios_access::__locale_of(is));
      basic_streambuf<__charT, __traits>* __sb = is.rdbuf();
      for (typename __traits::int_type c = __sb->sgetc();; c = __sb->snextc()) {
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        if (!__ct.is(ctype_base::space, __traits::to_char_type(c)))
          break;
      }
    });
  }
  if (__err)
    is.setstate(__err);
  return is;
}

// [istream.rvalue]
template <class _Istream, class _Tp>
  requires derived_from<_Istream, ios_base> && (!is_same_v<remove_cv_t<_Istream>, ios_base>) &&
           requires(_Istream& is, _Tp&& __x) { is >> static_cast<_Tp&&>(__x); }
_Istream&& operator>>(_Istream&& is, _Tp&& __x) {
  is >> static_cast<_Tp&&>(__x);
  return static_cast<_Istream&&>(is);
}

// [iostreamclass]
template <class __charT, class __traits>
class basic_iostream : public basic_istream<__charT, __traits>, public basic_ostream<__charT, __traits> {
public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;

  // The basic_ostream part does not initialize the shared basic_ios a second time.
  explicit basic_iostream(basic_streambuf<__charT, __traits>* __sb) : basic_istream<__charT, __traits>(__sb) {}
  ~basic_iostream() override {}

protected:
  basic_iostream(const basic_iostream&) = delete;
  basic_iostream(basic_iostream&& __rhs) : basic_istream<__charT, __traits>(static_cast<basic_istream<__charT, __traits>&&>(__rhs)) {}
  basic_iostream& operator=(const basic_iostream&) = delete;
  basic_iostream& operator=(basic_iostream&& __rhs) {
    swap(__rhs);
    return *this;
  }
  void swap(basic_iostream& __rhs) { basic_istream<__charT, __traits>::swap(__rhs); }
};

// [string.io]
template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, basic_string<__charT, __traits, _Allocator>& str) {
  ios_base::iostate __err = ios_base::goodbit;
  bool any = false;
  if (typename basic_istream<__charT, __traits>::sentry ok{is}) {
    __ycxx::__detail::__guarded_io(is, [&] {
      str.erase();
      const streamsize __w = is.width();
      using size_type = typename basic_string<__charT, __traits, _Allocator>::size_type;
      const size_type n = __w > 0 ? static_cast<size_type>(__w) : str.max_size();
      const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__ycxx::__detail::__ios_access::__locale_of(is));
      basic_streambuf<__charT, __traits>* __sb = is.rdbuf();
      size_type count = 0;
      for (typename __traits::int_type c = __sb->sgetc(); count < n; c = __sb->snextc()) {
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        if (__ct.is(ctype_base::space, __traits::to_char_type(c)))
          break;
        str.push_back(__traits::to_char_type(c));
        ++count;
      }
      any = count != 0;
      is.width(0);
    });
  }
  if (!any)
    __err |= ios_base::failbit;
  if (__err)
    is.setstate(__err);
  return is;
}

template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& getline(basic_istream<__charT, __traits>& is, basic_string<__charT, __traits, _Allocator>& str,
                                      __charT __delim) {
  ios_base::iostate __err = ios_base::goodbit;
  bool any = false;
  if (typename basic_istream<__charT, __traits>::sentry ok{is, true}) {
    __ycxx::__detail::__guarded_io(is, [&] {
      str.erase();
      basic_streambuf<__charT, __traits>* __sb = is.rdbuf();
      using __area = __ycxx::__detail::__streambuf_get_area;
      for (;;) {
        __charT* const __g = __area::__next(*__sb);
        __charT* const e = __area::__end(*__sb);
        bool __one = false; // one character at a time, near max_size()
        if (__g < e) {
          // A run of buffered characters: find the delimiter (traits::find, memchr for char)
          // and append what precedes it at once.
          const __charT* d = __traits::find(__g, static_cast<size_t>(e - __g), __delim);
          const size_t n = static_cast<size_t>((d ? d : e) - __g);
          if (n < str.max_size() - str.size()) {
            str.append(__g, n);
            any = any || n != 0;
            if (d) {
              __area::__advance_to(*__sb, __g + n + 1); // the delimiter is extracted, not stored
              any = true;
              break;
            }
            __area::__advance_to(*__sb, e);
            continue;
          }
          __one = true;
        }
        // No buffered character (or a string about to reach max_size()): one at a time through
        // the virtual functions; underflow may fill the get area, or the buffer may have none.
        typename __traits::int_type c = __sb->sgetc();
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        if (!__one && __area::__next(*__sb) < __area::__end(*__sb))
          continue; // the get area was filled: take the fast path
        if (__traits::eq(__traits::to_char_type(c), __delim)) {
          __sb->sbumpc();
          any = true;
          break;
        }
        if (str.size() == str.max_size()) {
          __err |= ios_base::failbit;
          break;
        }
        str.push_back(__traits::to_char_type(c));
        any = true;
        __sb->sbumpc();
      }
    });
  }
  if (!any)
    __err |= ios_base::failbit;
  if (__err)
    is.setstate(__err);
  return is;
}
template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& getline(basic_istream<__charT, __traits>&& is, basic_string<__charT, __traits, _Allocator>& str,
                                      __charT __delim) {
  return std::getline(is, str, __delim);
}
template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& getline(basic_istream<__charT, __traits>& is, basic_string<__charT, __traits, _Allocator>& str) {
  return std::getline(is, str, is.widen('\n'));
}
template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& getline(basic_istream<__charT, __traits>&& is, basic_string<__charT, __traits, _Allocator>& str) {
  return std::getline(is, str, is.widen('\n'));
}

// [bitset.operators]
template <class __charT, class __traits, size_t _Np>
basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, bitset<_Np>& __x) {
  ios_base::iostate __err = ios_base::goodbit;
  basic_string<__charT, __traits> str;
  const __charT zero = is.widen('0'), __one = is.widen('1');
  if (typename basic_istream<__charT, __traits>::sentry ok{is}) {
    __ycxx::__detail::__guarded_io(is, [&] {
      basic_streambuf<__charT, __traits>* __sb = is.rdbuf();
      for (typename __traits::int_type c = __sb->sgetc(); str.size() < _Np; c = __sb->snextc()) {
        if (__traits::eq_int_type(c, __traits::eof())) {
          __err |= ios_base::eofbit;
          break;
        }
        const __charT __ch = __traits::to_char_type(c);
        if (!__traits::eq(__ch, zero) && !__traits::eq(__ch, __one))
          break;
        str.push_back(__ch);
      }
    });
    if (_Np > 0 && str.empty())
      __err |= ios_base::failbit;
    else
      __x = bitset<_Np>(str, 0, basic_string<__charT, __traits>::npos, zero, __one);
  }
  if (__err)
    is.setstate(__err);
  return is;
}

// [complex.ops]: a series of simpler extractions: u, (u) or (u,v). x changes only when a whole
// number was read.
template <class _Tp>
class complex;
template <class _Tp, class __charT, class __traits>
basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, complex<_Tp>& __x) {
  _Tp __re{}, __im{};
  __charT __ch{};
  if (!(is >> __ch))
    return is;
  if (!__traits::eq(__ch, is.widen('('))) {
    is.putback(__ch);
    if (is >> __re)
      __x = complex<_Tp>(__re, __im);
    return is;
  }
  if (!(is >> __re >> __ch))
    return is;
  if (__traits::eq(__ch, is.widen(','))) {
    if (!(is >> __im >> __ch))
      return is;
  }
  if (__traits::eq(__ch, is.widen(')')))
    __x = complex<_Tp>(__re, __im);
  else
    is.setstate(ios_base::failbit);
  return is;
}

}} // namespace std
