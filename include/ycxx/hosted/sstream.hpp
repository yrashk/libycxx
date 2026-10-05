// libycxx hosted: basic_stringbuf and the string streams ([string.streams]).
//
// basic_stringbuf keeps its characters in buf_. With ios_base::out in the mode, buf_ is kept
// resized to its whole capacity and the put area spans it, so writes do not reallocate until
// the capacity is used up ([stringbuf.members] Note 1); the high mark (the count of initialized
// characters) is tracked separately and is where str() and view() end. The get area always
// starts at buf_.data(); with ios_base::in it ends at the high mark, which overflow() and
// underflow() advance ([stringbuf.virtuals]/1, /8).
#pragma once

#include <ycxx/core/container_base.hpp>
#include <ycxx/hosted/istream.hpp>

namespace [[gnu::visibility("hidden")]] std {

template <class charT, class traits, class Allocator>
class basic_stringbuf : public basic_streambuf<charT, traits> {
  using string_type = basic_string<charT, traits, Allocator>;
  using sv_type = basic_string_view<charT, traits>;
  using base = basic_streambuf<charT, traits>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;
  using allocator_type = Allocator;

  // [stringbuf.cons]
  basic_stringbuf() : basic_stringbuf(ios_base::in | ios_base::out) {}
  explicit basic_stringbuf(ios_base::openmode which) : mode_(which) { init_buf_ptrs(); }
  explicit basic_stringbuf(const string_type& s, ios_base::openmode which = ios_base::in | ios_base::out)
      : mode_(which), buf_(s) {
    init_buf_ptrs();
  }
  explicit basic_stringbuf(const Allocator& a) : basic_stringbuf(ios_base::in | ios_base::out, a) {}
  basic_stringbuf(ios_base::openmode which, const Allocator& a) : mode_(which), buf_(a) { init_buf_ptrs(); }
  explicit basic_stringbuf(string_type&& s, ios_base::openmode which = ios_base::in | ios_base::out)
      : mode_(which), buf_(static_cast<string_type&&>(s)) {
    init_buf_ptrs();
  }
  template <class SAlloc>
  basic_stringbuf(const basic_string<charT, traits, SAlloc>& s, const Allocator& a)
      : basic_stringbuf(s, ios_base::in | ios_base::out, a) {}
  template <class SAlloc>
  basic_stringbuf(const basic_string<charT, traits, SAlloc>& s, ios_base::openmode which, const Allocator& a)
      : mode_(which), buf_(s.data(), s.size(), a) {
    init_buf_ptrs();
  }
  template <class SAlloc>
    requires(!is_same_v<SAlloc, Allocator>)
  explicit basic_stringbuf(const basic_string<charT, traits, SAlloc>& s,
                           ios_base::openmode which = ios_base::in | ios_base::out)
      : mode_(which), buf_(s.data(), s.size()) {
    init_buf_ptrs();
  }
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  explicit basic_stringbuf(const T& t, ios_base::openmode which = ios_base::in | ios_base::out)
      : basic_stringbuf(t, which, Allocator()) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  basic_stringbuf(const T& t, const Allocator& a) : basic_stringbuf(t, ios_base::in | ios_base::out, a) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  basic_stringbuf(const T& t, ios_base::openmode which, const Allocator& a)
      : mode_(which), buf_(sv_type(t), a) {
    init_buf_ptrs();
  }
  basic_stringbuf(const basic_stringbuf&) = delete;
  basic_stringbuf(basic_stringbuf&& rhs) : base(rhs), mode_(rhs.mode_) {
    const offsets o = rhs.save();
    buf_ = static_cast<string_type&&>(rhs.buf_);
    restore(o);
    rhs.reset();
  }
  basic_stringbuf(basic_stringbuf&& rhs, const Allocator& a) : base(rhs), mode_(rhs.mode_), buf_(a) {
    const offsets o = rhs.save();
    buf_ = string_type(static_cast<string_type&&>(rhs.buf_), a);
    restore(o);
    rhs.reset();
  }

  // [stringbuf.assign]
  basic_stringbuf& operator=(const basic_stringbuf&) = delete;
  basic_stringbuf& operator=(basic_stringbuf&& rhs) {
    if (this == __builtin_addressof(rhs))
      return *this;
    const offsets o = rhs.save();
    base::operator=(rhs);
    mode_ = rhs.mode_;
    buf_ = static_cast<string_type&&>(rhs.buf_);
    restore(o);
    rhs.reset();
    return *this;
  }
  void swap(basic_stringbuf& rhs) noexcept(allocator_traits<Allocator>::propagate_on_container_swap::value ||
                                           allocator_traits<Allocator>::is_always_equal::value) {
    const offsets mine = save(), theirs = rhs.save();
    base::swap(rhs);
    const ios_base::openmode m = mode_;
    mode_ = rhs.mode_;
    rhs.mode_ = m;
    buf_.swap(rhs.buf_);
    restore(theirs);
    rhs.restore(mine);
  }

  // [stringbuf.members]
  allocator_type get_allocator() const noexcept { return buf_.get_allocator(); }
  string_type str() const& { return string_type(view(), get_allocator()); }
  template <class SAlloc>
    requires ycxx::detail::qualifies_as_allocator<SAlloc>
  basic_string<charT, traits, SAlloc> str(const SAlloc& sa) const {
    return basic_string<charT, traits, SAlloc>(view(), sa);
  }
  string_type str() && {
    const sv_type v = view();
    const size_t start = v.empty() ? 0 : static_cast<size_t>(v.data() - buf_.data());
    if (start != 0)
      buf_.erase(0, start);
    buf_.resize(v.size());
    string_type r = static_cast<string_type&&>(buf_);
    buf_.clear();
    init_buf_ptrs();
    return r;
  }
  basic_string_view<charT, traits> view() const noexcept {
    if (mode_ & ios_base::out)
      return sv_type(this->pbase(), high_mark());
    if (mode_ & ios_base::in)
      return sv_type(this->eback(), static_cast<size_t>(this->egptr() - this->eback()));
    return sv_type();
  }
  void str(const string_type& s) {
    buf_ = s;
    init_buf_ptrs();
  }
  template <class SAlloc>
    requires(!is_same_v<SAlloc, Allocator>)
  void str(const basic_string<charT, traits, SAlloc>& s) {
    buf_.assign(s.data(), s.size());
    init_buf_ptrs();
  }
  void str(string_type&& s) {
    buf_ = static_cast<string_type&&>(s);
    init_buf_ptrs();
  }
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  void str(const T& t) {
    const sv_type sv = t;
    buf_ = sv;
    init_buf_ptrs();
  }

protected:
  // [stringbuf.virtuals]
  // Not in the draft's list of overriders: characters written through the put area (sputc
  // does not call a virtual function while it has room) are part of the input sequence too
  // ([stringbuf.virtuals]/1), so in_avail() counts them.
  streamsize showmanyc() override {
    if (!(mode_ & ios_base::in) || this->gptr() == nullptr)
      return 0;
    if (mode_ & ios_base::out)
      extend_get_area();
    return static_cast<streamsize>(this->egptr() - this->gptr());
  }
  int_type underflow() override {
    if (!(mode_ & ios_base::in) || this->gptr() == nullptr)
      return traits::eof();
    if (mode_ & ios_base::out)
      extend_get_area();
    if (this->gptr() < this->egptr())
      return traits::to_int_type(*this->gptr());
    return traits::eof();
  }
  int_type pbackfail(int_type c = traits::eof()) override {
    if (this->eback() == nullptr || !(this->eback() < this->gptr()))
      return traits::eof();
    if (traits::eq_int_type(c, traits::eof())) {
      this->gbump(-1);
      return traits::not_eof(c);
    }
    if (traits::eq(traits::to_char_type(c), this->gptr()[-1])) {
      this->gbump(-1);
      return c;
    }
    if (mode_ & ios_base::out) {
      this->gbump(-1);
      *this->gptr() = traits::to_char_type(c);
      return c;
    }
    return traits::eof();
  }
  int_type overflow(int_type c = traits::eof()) override {
    if (traits::eq_int_type(c, traits::eof()))
      return traits::not_eof(c);
    if (!(mode_ & ios_base::out))
      return traits::eof();
    if (this->pptr() == this->epptr()) {
      // [stringbuf.virtuals]/8: a larger array, holding the old one plus a write position
      if (buf_.size() >= buf_.max_size())
        return traits::eof();
      offsets o = save();
      size_t want = buf_.size() < 16 ? 32 : buf_.size() + buf_.size() / 2;
      if (want > buf_.max_size() || want < buf_.size())
        want = buf_.max_size();
      buf_.resize(want);
      o.put = true;
      o.get = (mode_ & ios_base::in) != 0;
      restore(o);
    }
    *this->pptr() = traits::to_char_type(c);
    this->pbump(1);
    if (mode_ & ios_base::in)
      extend_get_area();
    return c;
  }
  basic_streambuf<charT, traits>* setbuf(charT*, streamsize) override { return this; }
  pos_type seekoff(off_type off, ios_base::seekdir way,
                   ios_base::openmode which = ios_base::in | ios_base::out) override {
    const bool in = (which & ios_base::in) != 0, out = (which & ios_base::out) != 0;
    if (!in && !out)
      return pos_type(off_type(-1));
    if (in && out && way == ios_base::cur)
      return pos_type(off_type(-1));
    const off_type hm = static_cast<off_type>(mode_ & ios_base::out ? high_mark() : in_end());
    off_type newoff;
    if (way == ios_base::beg)
      newoff = 0;
    else if (way == ios_base::cur)
      newoff = in ? static_cast<off_type>(this->gptr() - this->eback()) : static_cast<off_type>(this->pptr() - this->pbase());
    else if (way == ios_base::end)
      newoff = hm;
    else
      return pos_type(off_type(-1));
    if ((in && this->gptr() == nullptr && newoff != 0) || (out && this->pptr() == nullptr && newoff != 0))
      return pos_type(off_type(-1));
    // positioning a sequence the mode does not have fails, unless it stays at 0
    if ((in && !(mode_ & ios_base::in)) || (out && !(mode_ & ios_base::out)))
      if (newoff + off != 0)
        return pos_type(off_type(-1));
    if (off < -newoff || off > hm - newoff)
      return pos_type(off_type(-1));
    newoff += off;
    if (in && this->gptr() != nullptr) {
      if (mode_ & ios_base::out)
        extend_get_area();
      this->setg(this->eback(), this->eback() + newoff, this->egptr());
    }
    if (out && this->pptr() != nullptr) {
      const off_type keep = static_cast<off_type>(high_mark());
      this->setp(this->pbase(), this->epptr());
      advance_put(static_cast<size_t>(newoff));
      hm_ = static_cast<size_t>(keep);
    }
    return pos_type(newoff);
  }
  pos_type seekpos(pos_type sp, ios_base::openmode which = ios_base::in | ios_base::out) override {
    const pos_type r = seekoff(off_type(sp), ios_base::beg, which);
    return r == pos_type(off_type(-1)) ? r : sp;
  }

private:
  // the pointers as offsets into buf_ (for moving and swapping, which can move the characters)
  struct offsets {
    bool get, put;
    size_t gnext, gend, pnext, hm;
  };
  offsets save() const noexcept {
    offsets o{};
    o.get = this->eback() != nullptr;
    o.put = this->pbase() != nullptr;
    if (o.get) {
      o.gnext = static_cast<size_t>(this->gptr() - this->eback());
      o.gend = static_cast<size_t>(this->egptr() - this->eback());
    }
    if (o.put)
      o.pnext = static_cast<size_t>(this->pptr() - this->pbase());
    o.hm = mode_ & ios_base::out ? high_mark() : 0;
    return o;
  }
  void restore(const offsets& o) noexcept {
    charT* p = buf_.data();
    if (o.get)
      this->setg(p, p + o.gnext, p + o.gend);
    else
      this->setg(nullptr, nullptr, nullptr);
    if (o.put) {
      this->setp(p, p + buf_.size());
      advance_put(o.pnext);
    } else {
      this->setp(nullptr, nullptr);
    }
    hm_ = o.hm;
  }
  // rhs after a move: empty, as if std::move(rhs).str() had been called
  void reset() {
    buf_.clear();
    init_buf_ptrs();
  }

  void advance_put(size_t n) noexcept {
    while (n > static_cast<size_t>(__INT_MAX__)) {
      this->pbump(__INT_MAX__);
      n -= static_cast<size_t>(__INT_MAX__);
    }
    this->pbump(static_cast<int>(n));
  }
  size_t high_mark() const noexcept {
    const size_t p = static_cast<size_t>(this->pptr() - this->pbase());
    return p > hm_ ? p : hm_;
  }
  size_t in_end() const noexcept { return static_cast<size_t>(this->egptr() - this->eback()); }
  // the get area ends at the high mark (shared buffer with ios_base::out)
  void extend_get_area() noexcept {
    hm_ = high_mark();
    this->setg(this->eback(), this->gptr(), this->pbase() + hm_);
  }

  // [stringbuf.members]/2-3
  void init_buf_ptrs() {
    hm_ = buf_.size();
    if (mode_ & ios_base::out) {
      buf_.resize(buf_.capacity() > hm_ ? buf_.capacity() : hm_);
      charT* p = buf_.data();
      this->setp(p, p + buf_.size());
      if (mode_ & ios_base::ate)
        advance_put(hm_);
    } else {
      this->setp(nullptr, nullptr);
    }
    charT* p = buf_.data();
    if (mode_ & ios_base::in)
      this->setg(p, p, p + hm_);
    else
      this->setg(nullptr, nullptr, nullptr);
  }

  ios_base::openmode mode_;
  string_type buf_;
  size_t hm_ = 0; // the high mark, as a count (see high_mark())
};

template <class charT, class traits, class Allocator>
void swap(basic_stringbuf<charT, traits, Allocator>& x, basic_stringbuf<charT, traits, Allocator>& y) noexcept(
    noexcept(x.swap(y))) {
  x.swap(y);
}

// [istringstream]
template <class charT, class traits, class Allocator>
class basic_istringstream : public basic_istream<charT, traits> {
  using string_type = basic_string<charT, traits, Allocator>;
  using buf_type = basic_stringbuf<charT, traits, Allocator>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;
  using allocator_type = Allocator;

  basic_istringstream() : basic_istringstream(ios_base::in) {}
  explicit basic_istringstream(ios_base::openmode which)
      : basic_istream<charT, traits>(__builtin_addressof(sb_)), sb_(which | ios_base::in) {}
  explicit basic_istringstream(const string_type& s, ios_base::openmode which = ios_base::in)
      : basic_istream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which | ios_base::in) {}
  basic_istringstream(ios_base::openmode which, const Allocator& a)
      : basic_istream<charT, traits>(__builtin_addressof(sb_)), sb_(which | ios_base::in, a) {}
  explicit basic_istringstream(string_type&& s, ios_base::openmode which = ios_base::in)
      : basic_istream<charT, traits>(__builtin_addressof(sb_)), sb_(static_cast<string_type&&>(s), which | ios_base::in) {}
  template <class SAlloc>
  basic_istringstream(const basic_string<charT, traits, SAlloc>& s, const Allocator& a)
      : basic_istringstream(s, ios_base::in, a) {}
  template <class SAlloc>
  basic_istringstream(const basic_string<charT, traits, SAlloc>& s, ios_base::openmode which, const Allocator& a)
      : basic_istream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which | ios_base::in, a) {}
  template <class SAlloc>
    requires(!is_same_v<SAlloc, Allocator>)
  explicit basic_istringstream(const basic_string<charT, traits, SAlloc>& s, ios_base::openmode which = ios_base::in)
      : basic_istream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which | ios_base::in) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  explicit basic_istringstream(const T& t, ios_base::openmode which = ios_base::in)
      : basic_istringstream(t, which, Allocator()) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  basic_istringstream(const T& t, const Allocator& a) : basic_istringstream(t, ios_base::in, a) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  basic_istringstream(const T& t, ios_base::openmode which, const Allocator& a)
      : basic_istream<charT, traits>(__builtin_addressof(sb_)), sb_(t, which | ios_base::in, a) {}
  basic_istringstream(const basic_istringstream&) = delete;
  basic_istringstream(basic_istringstream&& rhs)
      : basic_istream<charT, traits>(static_cast<basic_istream<charT, traits>&&>(rhs)),
        sb_(static_cast<buf_type&&>(rhs.sb_)) {
    basic_istream<charT, traits>::set_rdbuf(__builtin_addressof(sb_));
  }
  basic_istringstream& operator=(const basic_istringstream&) = delete;
  basic_istringstream& operator=(basic_istringstream&& rhs) {
    basic_istream<charT, traits>::operator=(static_cast<basic_istream<charT, traits>&&>(rhs));
    sb_ = static_cast<buf_type&&>(rhs.sb_);
    return *this;
  }
  void swap(basic_istringstream& rhs) {
    basic_istream<charT, traits>::swap(rhs);
    sb_.swap(rhs.sb_);
  }

  buf_type* rdbuf() const { return const_cast<buf_type*>(__builtin_addressof(sb_)); }
  string_type str() const& { return rdbuf()->str(); }
  template <class SAlloc>
    requires ycxx::detail::qualifies_as_allocator<SAlloc>
  basic_string<charT, traits, SAlloc> str(const SAlloc& sa) const {
    return rdbuf()->str(sa);
  }
  string_type str() && { return static_cast<buf_type&&>(*rdbuf()).str(); }
  basic_string_view<charT, traits> view() const noexcept { return rdbuf()->view(); }
  void str(const string_type& s) { rdbuf()->str(s); }
  template <class SAlloc>
    requires(!is_same_v<SAlloc, Allocator>)
  void str(const basic_string<charT, traits, SAlloc>& s) {
    rdbuf()->str(s);
  }
  void str(string_type&& s) { rdbuf()->str(static_cast<string_type&&>(s)); }
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  void str(const T& t) {
    rdbuf()->str(t);
  }

private:
  buf_type sb_;
};

template <class charT, class traits, class Allocator>
void swap(basic_istringstream<charT, traits, Allocator>& x, basic_istringstream<charT, traits, Allocator>& y) {
  x.swap(y);
}

// [ostringstream]
template <class charT, class traits, class Allocator>
class basic_ostringstream : public basic_ostream<charT, traits> {
  using string_type = basic_string<charT, traits, Allocator>;
  using buf_type = basic_stringbuf<charT, traits, Allocator>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;
  using allocator_type = Allocator;

  basic_ostringstream() : basic_ostringstream(ios_base::out) {}
  explicit basic_ostringstream(ios_base::openmode which)
      : basic_ostream<charT, traits>(__builtin_addressof(sb_)), sb_(which | ios_base::out) {}
  explicit basic_ostringstream(const string_type& s, ios_base::openmode which = ios_base::out)
      : basic_ostream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which | ios_base::out) {}
  basic_ostringstream(ios_base::openmode which, const Allocator& a)
      : basic_ostream<charT, traits>(__builtin_addressof(sb_)), sb_(which | ios_base::out, a) {}
  explicit basic_ostringstream(string_type&& s, ios_base::openmode which = ios_base::out)
      : basic_ostream<charT, traits>(__builtin_addressof(sb_)), sb_(static_cast<string_type&&>(s), which | ios_base::out) {}
  template <class SAlloc>
  basic_ostringstream(const basic_string<charT, traits, SAlloc>& s, const Allocator& a)
      : basic_ostringstream(s, ios_base::out, a) {}
  template <class SAlloc>
  basic_ostringstream(const basic_string<charT, traits, SAlloc>& s, ios_base::openmode which, const Allocator& a)
      : basic_ostream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which | ios_base::out, a) {}
  template <class SAlloc>
    requires(!is_same_v<SAlloc, Allocator>)
  explicit basic_ostringstream(const basic_string<charT, traits, SAlloc>& s, ios_base::openmode which = ios_base::out)
      : basic_ostream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which | ios_base::out) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  explicit basic_ostringstream(const T& t, ios_base::openmode which = ios_base::out)
      : basic_ostringstream(t, which, Allocator()) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  basic_ostringstream(const T& t, const Allocator& a) : basic_ostringstream(t, ios_base::out, a) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  basic_ostringstream(const T& t, ios_base::openmode which, const Allocator& a)
      : basic_ostream<charT, traits>(__builtin_addressof(sb_)), sb_(t, which | ios_base::out, a) {}
  basic_ostringstream(const basic_ostringstream&) = delete;
  basic_ostringstream(basic_ostringstream&& rhs)
      : basic_ostream<charT, traits>(static_cast<basic_ostream<charT, traits>&&>(rhs)),
        sb_(static_cast<buf_type&&>(rhs.sb_)) {
    basic_ostream<charT, traits>::set_rdbuf(__builtin_addressof(sb_));
  }
  basic_ostringstream& operator=(const basic_ostringstream&) = delete;
  basic_ostringstream& operator=(basic_ostringstream&& rhs) {
    basic_ostream<charT, traits>::operator=(static_cast<basic_ostream<charT, traits>&&>(rhs));
    sb_ = static_cast<buf_type&&>(rhs.sb_);
    return *this;
  }
  void swap(basic_ostringstream& rhs) {
    basic_ostream<charT, traits>::swap(rhs);
    sb_.swap(rhs.sb_);
  }

  buf_type* rdbuf() const { return const_cast<buf_type*>(__builtin_addressof(sb_)); }
  string_type str() const& { return rdbuf()->str(); }
  template <class SAlloc>
    requires ycxx::detail::qualifies_as_allocator<SAlloc>
  basic_string<charT, traits, SAlloc> str(const SAlloc& sa) const {
    return rdbuf()->str(sa);
  }
  string_type str() && { return static_cast<buf_type&&>(*rdbuf()).str(); }
  basic_string_view<charT, traits> view() const noexcept { return rdbuf()->view(); }
  void str(const string_type& s) { rdbuf()->str(s); }
  template <class SAlloc>
    requires(!is_same_v<SAlloc, Allocator>)
  void str(const basic_string<charT, traits, SAlloc>& s) {
    rdbuf()->str(s);
  }
  void str(string_type&& s) { rdbuf()->str(static_cast<string_type&&>(s)); }
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  void str(const T& t) {
    rdbuf()->str(t);
  }

private:
  buf_type sb_;
};

template <class charT, class traits, class Allocator>
void swap(basic_ostringstream<charT, traits, Allocator>& x, basic_ostringstream<charT, traits, Allocator>& y) {
  x.swap(y);
}

// [stringstream]
template <class charT, class traits, class Allocator>
class basic_stringstream : public basic_iostream<charT, traits> {
  using string_type = basic_string<charT, traits, Allocator>;
  using buf_type = basic_stringbuf<charT, traits, Allocator>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;
  using allocator_type = Allocator;

  basic_stringstream() : basic_stringstream(ios_base::out | ios_base::in) {}
  explicit basic_stringstream(ios_base::openmode which)
      : basic_iostream<charT, traits>(__builtin_addressof(sb_)), sb_(which) {}
  explicit basic_stringstream(const string_type& s, ios_base::openmode which = ios_base::out | ios_base::in)
      : basic_iostream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which) {}
  basic_stringstream(ios_base::openmode which, const Allocator& a)
      : basic_iostream<charT, traits>(__builtin_addressof(sb_)), sb_(which, a) {}
  explicit basic_stringstream(string_type&& s, ios_base::openmode which = ios_base::out | ios_base::in)
      : basic_iostream<charT, traits>(__builtin_addressof(sb_)), sb_(static_cast<string_type&&>(s), which) {}
  template <class SAlloc>
  basic_stringstream(const basic_string<charT, traits, SAlloc>& s, const Allocator& a)
      : basic_stringstream(s, ios_base::out | ios_base::in, a) {}
  template <class SAlloc>
  basic_stringstream(const basic_string<charT, traits, SAlloc>& s, ios_base::openmode which, const Allocator& a)
      : basic_iostream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which, a) {}
  template <class SAlloc>
    requires(!is_same_v<SAlloc, Allocator>)
  explicit basic_stringstream(const basic_string<charT, traits, SAlloc>& s,
                              ios_base::openmode which = ios_base::out | ios_base::in)
      : basic_iostream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  explicit basic_stringstream(const T& t, ios_base::openmode which = ios_base::out | ios_base::in)
      : basic_stringstream(t, which, Allocator()) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  basic_stringstream(const T& t, const Allocator& a) : basic_stringstream(t, ios_base::out | ios_base::in, a) {}
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  basic_stringstream(const T& t, ios_base::openmode which, const Allocator& a)
      : basic_iostream<charT, traits>(__builtin_addressof(sb_)), sb_(t, which, a) {}
  basic_stringstream(const basic_stringstream&) = delete;
  basic_stringstream(basic_stringstream&& rhs)
      : basic_iostream<charT, traits>(static_cast<basic_iostream<charT, traits>&&>(rhs)),
        sb_(static_cast<buf_type&&>(rhs.sb_)) {
    basic_istream<charT, traits>::set_rdbuf(__builtin_addressof(sb_));
  }
  basic_stringstream& operator=(const basic_stringstream&) = delete;
  basic_stringstream& operator=(basic_stringstream&& rhs) {
    basic_iostream<charT, traits>::operator=(static_cast<basic_iostream<charT, traits>&&>(rhs));
    sb_ = static_cast<buf_type&&>(rhs.sb_);
    return *this;
  }
  void swap(basic_stringstream& rhs) {
    basic_iostream<charT, traits>::swap(rhs);
    sb_.swap(rhs.sb_);
  }

  buf_type* rdbuf() const { return const_cast<buf_type*>(__builtin_addressof(sb_)); }
  string_type str() const& { return rdbuf()->str(); }
  template <class SAlloc>
    requires ycxx::detail::qualifies_as_allocator<SAlloc>
  basic_string<charT, traits, SAlloc> str(const SAlloc& sa) const {
    return rdbuf()->str(sa);
  }
  string_type str() && { return static_cast<buf_type&&>(*rdbuf()).str(); }
  basic_string_view<charT, traits> view() const noexcept { return rdbuf()->view(); }
  void str(const string_type& s) { rdbuf()->str(s); }
  template <class SAlloc>
    requires(!is_same_v<SAlloc, Allocator>)
  void str(const basic_string<charT, traits, SAlloc>& s) {
    rdbuf()->str(s);
  }
  void str(string_type&& s) { rdbuf()->str(static_cast<string_type&&>(s)); }
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  void str(const T& t) {
    rdbuf()->str(t);
  }

private:
  buf_type sb_;
};

template <class charT, class traits, class Allocator>
void swap(basic_stringstream<charT, traits, Allocator>& x, basic_stringstream<charT, traits, Allocator>& y) {
  x.swap(y);
}

} // namespace std
