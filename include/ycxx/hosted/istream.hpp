// libycxx hosted: basic_istream, basic_iostream and ws ([input.streams]), and the extractors
// other headers declare for their types (basic_string, getline, bitset).
//
// Every input function follows [istream.formatted.reqmts] / [istream.unformatted]: it collects
// a local error state, runs the extraction under guarded_io (an exception sets badbit in the
// stream state without throwing failure, and is rethrown if badbit is in exceptions()), and
// calls setstate with the local state at the end.
#pragma once

#include <ycxx/hosted/ostream.hpp>

namespace std {

template <class charT, class traits>
class basic_istream : virtual public basic_ios<charT, traits> {
public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;

  explicit basic_istream(basic_streambuf<charT, traits>* sb) : gcount_(0) { this->init(sb); }
  ~basic_istream() override {}

  class sentry;

  // [istream.extractors]
  basic_istream& operator>>(basic_istream& (*pf)(basic_istream&)) { return pf(*this); }
  basic_istream& operator>>(basic_ios<charT, traits>& (*pf)(basic_ios<charT, traits>&)) {
    pf(*this);
    return *this;
  }
  basic_istream& operator>>(ios_base& (*pf)(ios_base&)) {
    pf(*this);
    return *this;
  }

  // [istream.formatted.arithmetic]
  basic_istream& operator>>(bool& n) { return get_number(n); }
  basic_istream& operator>>(short& n) { return get_narrowed(n); }
  basic_istream& operator>>(unsigned short& n) { return get_number(n); }
  basic_istream& operator>>(int& n) { return get_narrowed(n); }
  basic_istream& operator>>(unsigned int& n) { return get_number(n); }
  basic_istream& operator>>(long& n) { return get_number(n); }
  basic_istream& operator>>(unsigned long& n) { return get_number(n); }
  basic_istream& operator>>(long long& n) { return get_number(n); }
  basic_istream& operator>>(unsigned long long& n) { return get_number(n); }
  basic_istream& operator>>(float& f) { return get_number(f); }
  basic_istream& operator>>(double& f) { return get_number(f); }
  basic_istream& operator>>(long double& f) { return get_number(f); }
  template <class F>
    requires ycxx::detail::is_extended_floating_point<F> && ycxx::detail::fp_rank_le<F, long double>
  basic_istream& operator>>(F& f);
  basic_istream& operator>>(void*& p) { return get_number(p); }
  basic_istream& operator>>(basic_streambuf<char_type, traits>* sb);

  // [istream.unformatted]
  streamsize gcount() const { return gcount_; }
  int_type get();
  basic_istream& get(char_type& c);
  basic_istream& get(char_type* s, streamsize n) { return get(s, n, this->widen('\n')); }
  basic_istream& get(char_type* s, streamsize n, char_type delim);
  basic_istream& get(basic_streambuf<char_type, traits>& sb) { return get(sb, this->widen('\n')); }
  basic_istream& get(basic_streambuf<char_type, traits>& sb, char_type delim);
  basic_istream& getline(char_type* s, streamsize n) { return getline(s, n, this->widen('\n')); }
  basic_istream& getline(char_type* s, streamsize n, char_type delim);
  basic_istream& ignore(streamsize n = 1, int_type delim = traits::eof());
  // A template (exactly char_type is deduced), so that ignore(n, -1L) still picks the int_type
  // overload instead of being ambiguous.
  template <class C>
    requires is_same_v<C, char_type> && is_same_v<char_type, char>
  basic_istream& ignore(streamsize n, C delim) {
    return ignore(n, traits::to_int_type(delim));
  }
  int_type peek();
  basic_istream& read(char_type* s, streamsize n);
  streamsize readsome(char_type* s, streamsize n);
  basic_istream& putback(char_type c);
  basic_istream& unget();
  int sync();
  pos_type tellg();
  basic_istream& seekg(pos_type pos);
  basic_istream& seekg(off_type off, ios_base::seekdir dir);

protected:
  basic_istream(const basic_istream&) = delete;
  basic_istream(basic_istream&& rhs) : gcount_(rhs.gcount_) {
    this->move(rhs);
    rhs.gcount_ = 0;
  }
  basic_istream& operator=(const basic_istream&) = delete;
  basic_istream& operator=(basic_istream&& rhs) {
    swap(rhs);
    return *this;
  }
  void swap(basic_istream& rhs) {
    basic_ios<charT, traits>::swap(rhs);
    const streamsize g = gcount_;
    gcount_ = rhs.gcount_;
    rhs.gcount_ = g;
  }

private:
  template <class V>
  basic_istream& get_number(V& v);
  template <class V>
  basic_istream& get_narrowed(V& v);
  // the end of an unformatted input function: the count, then setstate
  void finish(ios_base::iostate err, streamsize count) {
    gcount_ = count;
    if (err)
      this->setstate(err);
  }

  streamsize gcount_ = 0;
};

// [istream.sentry]
template <class charT, class traits>
class basic_istream<charT, traits>::sentry {
  bool ok_;

public:
  explicit sentry(basic_istream& is, bool noskipws = false) : ok_(false) {
    if (!is.good()) {
      is.setstate(ios_base::failbit);
      return;
    }
    if (is.tie() != nullptr)
      is.tie()->flush();
    if (!noskipws && (is.flags() & ios_base::skipws)) {
      ios_base::iostate err = ios_base::goodbit;
      ycxx::detail::guarded_io(is, [&] {
        const ctype<charT>& ct = use_facet<ctype<charT>>(is.getloc());
        basic_streambuf<charT, traits>* sb = is.rdbuf();
        for (int_type c = sb->sgetc();; c = sb->snextc()) {
          if (traits::eq_int_type(c, traits::eof())) {
            err |= ios_base::failbit | ios_base::eofbit;
            break;
          }
          if (!ct.is(ctype_base::space, traits::to_char_type(c)))
            break;
        }
      });
      if (err)
        is.setstate(err);
    }
    ok_ = is.good();
  }
  ~sentry() {}
  explicit operator bool() const { return ok_; }
  sentry(const sentry&) = delete;
  sentry& operator=(const sentry&) = delete;
};

template <class charT, class traits>
template <class V>
basic_istream<charT, traits>& basic_istream<charT, traits>::get_number(V& v) {
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this}) {
    ycxx::detail::guarded_io(*this, [&] {
      using It = istreambuf_iterator<charT, traits>;
      use_facet<num_get<charT, It>>(ycxx::detail::ios_access::locale_of(*this)).get(It(*this), It(), *this, err, v);
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

// [istream.formatted.arithmetic]/2-3: short and int through long, clamped.
template <class charT, class traits>
template <class V>
basic_istream<charT, traits>& basic_istream<charT, traits>::get_narrowed(V& v) {
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this}) {
    ycxx::detail::guarded_io(*this, [&] {
      using It = istreambuf_iterator<charT, traits>;
      long lval = 0;
      use_facet<num_get<charT, It>>(ycxx::detail::ios_access::locale_of(*this)).get(It(*this), It(), *this, err, lval);
      if (lval < numeric_limits<V>::min()) {
        err |= ios_base::failbit;
        v = numeric_limits<V>::min();
      } else if (numeric_limits<V>::max() < lval) {
        err |= ios_base::failbit;
        v = numeric_limits<V>::max();
      } else {
        v = static_cast<V>(lval);
      }
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

// [istream.formatted.arithmetic]/4-6
template <class charT, class traits>
template <class F>
  requires ycxx::detail::is_extended_floating_point<F> && ycxx::detail::fp_rank_le<F, long double>
basic_istream<charT, traits>& basic_istream<charT, traits>::operator>>(F& val) {
  using FP = conditional_t<ycxx::detail::fp_rank_le<F, float>, float,
                           conditional_t<ycxx::detail::fp_rank_le<F, double>, double, long double>>;
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this}) {
    ycxx::detail::guarded_io(*this, [&] {
      using It = istreambuf_iterator<charT, traits>;
      FP fval = 0;
      use_facet<num_get<charT, It>>(ycxx::detail::ios_access::locale_of(*this)).get(It(*this), It(), *this, err, fval);
      if (fval < -static_cast<FP>(numeric_limits<F>::max())) {
        err |= ios_base::failbit;
        val = -numeric_limits<F>::max();
      } else if (static_cast<FP>(numeric_limits<F>::max()) < fval) {
        err |= ios_base::failbit;
        val = numeric_limits<F>::max();
      } else {
        val = static_cast<F>(fval);
      }
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

// [istream.extractors]/14-15
template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::operator>>(basic_streambuf<char_type, traits>* sb) {
  ios_base::iostate err = ios_base::goodbit;
  streamsize n = 0;
  if (sb == nullptr) {
    gcount_ = 0;
    this->setstate(ios_base::failbit);
    return *this;
  }
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      basic_streambuf<charT, traits>* in = this->rdbuf();
      for (int_type c = in->sgetc();; c = in->snextc()) {
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        // an exception from the output sequence is caught (14.3)
        bool inserted = false;
        if constexpr (ycxx::detail::cfg::exceptions) {
          try {
            inserted = !traits::eq_int_type(sb->sputc(traits::to_char_type(c)), traits::eof());
          } catch (...) {
          }
        } else {
          inserted = !traits::eq_int_type(sb->sputc(traits::to_char_type(c)), traits::eof());
        }
        if (!inserted)
          break;
        ++n;
      }
    });
  }
  if (n == 0)
    err |= ios_base::failbit;
  finish(err, n);
  return *this;
}

template <class charT, class traits>
typename basic_istream<charT, traits>::int_type basic_istream<charT, traits>::get() {
  ios_base::iostate err = ios_base::goodbit;
  int_type c = traits::eof();
  streamsize n = 0;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      c = this->rdbuf()->sbumpc();
      if (traits::eq_int_type(c, traits::eof()))
        err |= ios_base::eofbit | ios_base::failbit;
      else
        n = 1;
    });
  }
  finish(err, n);
  return c;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::get(char_type& c) {
  const int_type r = get();
  if (!traits::eq_int_type(r, traits::eof()))
    c = traits::to_char_type(r);
  return *this;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::get(char_type* s, streamsize n, char_type delim) {
  ios_base::iostate err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      basic_streambuf<charT, traits>* sb = this->rdbuf();
      for (int_type c = sb->sgetc(); count + 1 < n; c = sb->snextc()) {
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        if (traits::eq(traits::to_char_type(c), delim))
          break;
        s[count++] = traits::to_char_type(c);
      }
    });
  }
  if (n > 0)
    s[count] = charT();
  if (count == 0)
    err |= ios_base::failbit;
  finish(err, count);
  return *this;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::get(basic_streambuf<char_type, traits>& sb,
                                                                char_type delim) {
  ios_base::iostate err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      basic_streambuf<charT, traits>* in = this->rdbuf();
      for (int_type c = in->sgetc();; c = in->snextc()) {
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        if (traits::eq(traits::to_char_type(c), delim))
          break;
        // an exception from sb is caught but not rethrown (13.4)
        bool inserted = false;
        if constexpr (ycxx::detail::cfg::exceptions) {
          try {
            inserted = !traits::eq_int_type(sb.sputc(traits::to_char_type(c)), traits::eof());
          } catch (...) {
          }
        } else {
          inserted = !traits::eq_int_type(sb.sputc(traits::to_char_type(c)), traits::eof());
        }
        if (!inserted)
          break;
        ++count;
      }
    });
  }
  if (count == 0)
    err |= ios_base::failbit;
  finish(err, count);
  return *this;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::getline(char_type* s, streamsize n, char_type delim) {
  ios_base::iostate err = ios_base::goodbit;
  streamsize count = 0, stored = 0;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      basic_streambuf<charT, traits>* sb = this->rdbuf();
      for (int_type c = sb->sgetc();; c = sb->snextc()) {
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        if (traits::eq(traits::to_char_type(c), delim)) {
          ++count; // extracted, not stored
          sb->sbumpc();
          break;
        }
        if (n < 1 || stored + 1 >= n) {
          err |= ios_base::failbit;
          break;
        }
        s[stored++] = traits::to_char_type(c);
        ++count;
      }
    });
  }
  if (n > 0)
    s[stored] = charT();
  if (count == 0)
    err |= ios_base::failbit;
  finish(err, count);
  return *this;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::ignore(streamsize n, int_type delim) {
  ios_base::iostate err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      basic_streambuf<charT, traits>* sb = this->rdbuf();
      const bool unlimited = n == numeric_limits<streamsize>::max();
      while (unlimited || count < n) {
        const int_type c = sb->sbumpc();
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        if (count != numeric_limits<streamsize>::max())
          ++count;
        if (traits::eq_int_type(c, delim))
          break;
      }
    });
  }
  finish(err, count);
  return *this;
}

template <class charT, class traits>
typename basic_istream<charT, traits>::int_type basic_istream<charT, traits>::peek() {
  ios_base::iostate err = ios_base::goodbit;
  int_type c = traits::eof();
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      c = this->rdbuf()->sgetc();
      if (traits::eq_int_type(c, traits::eof()))
        err |= ios_base::eofbit;
    });
  }
  finish(err, 0);
  return c;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::read(char_type* s, streamsize n) {
  ios_base::iostate err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      count = this->rdbuf()->sgetn(s, n);
      if (count != n)
        err |= ios_base::failbit | ios_base::eofbit;
    });
  } else {
    err |= ios_base::failbit;
  }
  finish(err, count);
  return *this;
}

template <class charT, class traits>
streamsize basic_istream<charT, traits>::readsome(char_type* s, streamsize n) {
  ios_base::iostate err = ios_base::goodbit;
  streamsize count = 0;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      const streamsize avail = this->rdbuf()->in_avail();
      if (avail == -1)
        err |= ios_base::eofbit;
      else if (avail > 0)
        count = this->rdbuf()->sgetn(s, avail < n ? avail : n);
    });
  } else {
    err |= ios_base::failbit;
  }
  finish(err, count);
  return count;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::putback(char_type c) {
  this->clear(this->rdstate() & ~ios_base::eofbit);
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      if (this->rdbuf() == nullptr || traits::eq_int_type(this->rdbuf()->sputbackc(c), traits::eof()))
        err |= ios_base::badbit;
    });
  } else {
    err |= ios_base::failbit;
  }
  finish(err, 0);
  return *this;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::unget() {
  this->clear(this->rdstate() & ~ios_base::eofbit);
  ios_base::iostate err = ios_base::goodbit;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      if (this->rdbuf() == nullptr || traits::eq_int_type(this->rdbuf()->sungetc(), traits::eof()))
        err |= ios_base::badbit;
    });
  } else {
    err |= ios_base::failbit;
  }
  finish(err, 0);
  return *this;
}

template <class charT, class traits>
int basic_istream<charT, traits>::sync() {
  ios_base::iostate err = ios_base::goodbit;
  int r = 0;
  if (sentry ok{*this, true}) {
    ycxx::detail::guarded_io(*this, [&] {
      if (this->rdbuf() == nullptr) {
        r = -1;
      } else if (this->rdbuf()->pubsync() == -1) {
        err |= ios_base::badbit;
        r = -1;
      }
    });
  } else {
    r = -1;
  }
  if (err)
    this->setstate(err);
  return r;
}

template <class charT, class traits>
typename basic_istream<charT, traits>::pos_type basic_istream<charT, traits>::tellg() {
  pos_type r = pos_type(off_type(-1));
  sentry ok{*this, true};
  if (!this->fail())
    ycxx::detail::guarded_io(*this, [&] { r = this->rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in); });
  return r;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::seekg(pos_type pos) {
  this->clear(this->rdstate() & ~ios_base::eofbit);
  ios_base::iostate err = ios_base::goodbit;
  sentry ok{*this, true};
  if (!this->fail()) {
    ycxx::detail::guarded_io(*this, [&] {
      if (this->rdbuf()->pubseekpos(pos, ios_base::in) == pos_type(off_type(-1)))
        err |= ios_base::failbit;
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

template <class charT, class traits>
basic_istream<charT, traits>& basic_istream<charT, traits>::seekg(off_type off, ios_base::seekdir dir) {
  this->clear(this->rdstate() & ~ios_base::eofbit);
  ios_base::iostate err = ios_base::goodbit;
  sentry ok{*this, true};
  if (!this->fail()) {
    ycxx::detail::guarded_io(*this, [&] {
      if (this->rdbuf()->pubseekoff(off, dir, ios_base::in) == pos_type(off_type(-1)))
        err |= ios_base::failbit;
    });
  }
  if (err)
    this->setstate(err);
  return *this;
}

// [istream.extractors]/7-13
template <class charT, class traits, size_t N>
basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& in, charT (&s)[N]) {
  ios_base::iostate err = ios_base::goodbit;
  size_t count = 0;
  if (typename basic_istream<charT, traits>::sentry ok{in}) {
    ycxx::detail::guarded_io(in, [&] {
      const streamsize w = in.width();
      const size_t n = w > 0 && static_cast<size_t>(w) < N ? static_cast<size_t>(w) : N;
      const ctype<charT>& ct = use_facet<ctype<charT>>(in.getloc());
      basic_streambuf<charT, traits>* sb = in.rdbuf();
      for (typename traits::int_type c = sb->sgetc(); count + 1 < n; c = sb->snextc()) {
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        if (ct.is(ctype_base::space, traits::to_char_type(c)))
          break;
        s[count++] = traits::to_char_type(c);
      }
      s[count] = charT();
      in.width(0);
    });
  }
  if (count == 0)
    err |= ios_base::failbit;
  if (err)
    in.setstate(err);
  return in;
}
template <class traits, size_t N>
[[deprecated("signed char / unsigned char stream extraction is deprecated ([depr.istream.extractors]); use char")]]
basic_istream<char, traits>& operator>>(basic_istream<char, traits>& in, unsigned char (&s)[N]) {
  return in >> reinterpret_cast<char(&)[N]>(s);
}
template <class traits, size_t N>
[[deprecated("signed char / unsigned char stream extraction is deprecated ([depr.istream.extractors]); use char")]]
basic_istream<char, traits>& operator>>(basic_istream<char, traits>& in, signed char (&s)[N]) {
  return in >> reinterpret_cast<char(&)[N]>(s);
}

template <class charT, class traits>
basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& in, charT& c) {
  ios_base::iostate err = ios_base::goodbit;
  if (typename basic_istream<charT, traits>::sentry ok{in}) {
    ycxx::detail::guarded_io(in, [&] {
      const typename traits::int_type r = in.rdbuf()->sbumpc();
      if (traits::eq_int_type(r, traits::eof()))
        err |= ios_base::eofbit | ios_base::failbit;
      else
        c = traits::to_char_type(r);
    });
  }
  if (err)
    in.setstate(err);
  return in;
}
template <class traits>
[[deprecated("signed char / unsigned char stream extraction is deprecated ([depr.istream.extractors]); use char")]]
basic_istream<char, traits>& operator>>(basic_istream<char, traits>& in, unsigned char& c) {
  return in >> reinterpret_cast<char&>(c);
}
template <class traits>
[[deprecated("signed char / unsigned char stream extraction is deprecated ([depr.istream.extractors]); use char")]]
basic_istream<char, traits>& operator>>(basic_istream<char, traits>& in, signed char& c) {
  return in >> reinterpret_cast<char&>(c);
}

// [istream.manip]
template <class charT, class traits>
basic_istream<charT, traits>& ws(basic_istream<charT, traits>& is) {
  ios_base::iostate err = ios_base::goodbit;
  if (typename basic_istream<charT, traits>::sentry ok{is, true}) {
    ycxx::detail::guarded_io(is, [&] {
      const ctype<charT>& ct = use_facet<ctype<charT>>(is.getloc());
      basic_streambuf<charT, traits>* sb = is.rdbuf();
      for (typename traits::int_type c = sb->sgetc();; c = sb->snextc()) {
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        if (!ct.is(ctype_base::space, traits::to_char_type(c)))
          break;
      }
    });
  }
  if (err)
    is.setstate(err);
  return is;
}

// [istream.rvalue]
template <class Istream, class T>
  requires derived_from<Istream, ios_base> && (!is_same_v<remove_cv_t<Istream>, ios_base>) &&
           requires(Istream& is, T&& x) { is >> static_cast<T&&>(x); }
Istream&& operator>>(Istream&& is, T&& x) {
  is >> static_cast<T&&>(x);
  return static_cast<Istream&&>(is);
}

// [iostreamclass]
template <class charT, class traits>
class basic_iostream : public basic_istream<charT, traits>, public basic_ostream<charT, traits> {
public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;

  // The basic_ostream part does not initialize the shared basic_ios a second time.
  explicit basic_iostream(basic_streambuf<charT, traits>* sb) : basic_istream<charT, traits>(sb) {}
  ~basic_iostream() override {}

protected:
  basic_iostream(const basic_iostream&) = delete;
  basic_iostream(basic_iostream&& rhs) : basic_istream<charT, traits>(static_cast<basic_istream<charT, traits>&&>(rhs)) {}
  basic_iostream& operator=(const basic_iostream&) = delete;
  basic_iostream& operator=(basic_iostream&& rhs) {
    swap(rhs);
    return *this;
  }
  void swap(basic_iostream& rhs) { basic_istream<charT, traits>::swap(rhs); }
};

// [string.io]
template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, basic_string<charT, traits, Allocator>& str) {
  ios_base::iostate err = ios_base::goodbit;
  bool any = false;
  if (typename basic_istream<charT, traits>::sentry ok{is}) {
    ycxx::detail::guarded_io(is, [&] {
      str.erase();
      const streamsize w = is.width();
      using size_type = typename basic_string<charT, traits, Allocator>::size_type;
      const size_type n = w > 0 ? static_cast<size_type>(w) : str.max_size();
      const ctype<charT>& ct = use_facet<ctype<charT>>(is.getloc());
      basic_streambuf<charT, traits>* sb = is.rdbuf();
      size_type count = 0;
      for (typename traits::int_type c = sb->sgetc(); count < n; c = sb->snextc()) {
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        if (ct.is(ctype_base::space, traits::to_char_type(c)))
          break;
        str.push_back(traits::to_char_type(c));
        ++count;
      }
      any = count != 0;
      is.width(0);
    });
  }
  if (!any)
    err |= ios_base::failbit;
  if (err)
    is.setstate(err);
  return is;
}

template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& getline(basic_istream<charT, traits>& is, basic_string<charT, traits, Allocator>& str,
                                      charT delim) {
  ios_base::iostate err = ios_base::goodbit;
  bool any = false;
  if (typename basic_istream<charT, traits>::sentry ok{is, true}) {
    ycxx::detail::guarded_io(is, [&] {
      str.erase();
      basic_streambuf<charT, traits>* sb = is.rdbuf();
      for (typename traits::int_type c = sb->sgetc();; c = sb->snextc()) {
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        if (traits::eq(traits::to_char_type(c), delim)) {
          sb->sbumpc();
          any = true;
          break;
        }
        if (str.size() == str.max_size()) {
          err |= ios_base::failbit;
          break;
        }
        str.push_back(traits::to_char_type(c));
        any = true;
      }
    });
  }
  if (!any)
    err |= ios_base::failbit;
  if (err)
    is.setstate(err);
  return is;
}
template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& getline(basic_istream<charT, traits>&& is, basic_string<charT, traits, Allocator>& str,
                                      charT delim) {
  return std::getline(is, str, delim);
}
template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& getline(basic_istream<charT, traits>& is, basic_string<charT, traits, Allocator>& str) {
  return std::getline(is, str, is.widen('\n'));
}
template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& getline(basic_istream<charT, traits>&& is, basic_string<charT, traits, Allocator>& str) {
  return std::getline(is, str, is.widen('\n'));
}

// [bitset.operators]
template <class charT, class traits, size_t N>
basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, bitset<N>& x) {
  ios_base::iostate err = ios_base::goodbit;
  basic_string<charT, traits> str;
  const charT zero = is.widen('0'), one = is.widen('1');
  if (typename basic_istream<charT, traits>::sentry ok{is}) {
    ycxx::detail::guarded_io(is, [&] {
      basic_streambuf<charT, traits>* sb = is.rdbuf();
      for (typename traits::int_type c = sb->sgetc(); str.size() < N; c = sb->snextc()) {
        if (traits::eq_int_type(c, traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        const charT ch = traits::to_char_type(c);
        if (!traits::eq(ch, zero) && !traits::eq(ch, one))
          break;
        str.push_back(ch);
      }
    });
    if (N > 0 && str.empty())
      err |= ios_base::failbit;
    else
      x = bitset<N>(str, 0, basic_string<charT, traits>::npos, zero, one);
  }
  if (err)
    is.setstate(err);
  return is;
}

// [complex.ops]: a series of simpler extractions: u, (u) or (u,v). x changes only when a whole
// number was read.
template <class T>
class complex;
template <class T, class charT, class traits>
basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, complex<T>& x) {
  T re{}, im{};
  charT ch{};
  if (!(is >> ch))
    return is;
  if (!traits::eq(ch, is.widen('('))) {
    is.putback(ch);
    if (is >> re)
      x = complex<T>(re, im);
    return is;
  }
  if (!(is >> re >> ch))
    return is;
  if (traits::eq(ch, is.widen(','))) {
    if (!(is >> im >> ch))
      return is;
  }
  if (traits::eq(ch, is.widen(')')))
    x = complex<T>(re, im);
  else
    is.setstate(ios_base::failbit);
  return is;
}

} // namespace std
