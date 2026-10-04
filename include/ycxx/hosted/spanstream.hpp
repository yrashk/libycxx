// libycxx hosted: basic_spanbuf and the span streams ([span.streams]).
#pragma once

#include <ycxx/core/ranges_base.hpp>
#include <ycxx/core/span.hpp>
#include <ycxx/hosted/istream.hpp>

namespace std {

template <class charT, class traits>
class basic_spanbuf : public basic_streambuf<charT, traits> {
  using base = basic_streambuf<charT, traits>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;

  // [spanbuf.cons]
  basic_spanbuf() : basic_spanbuf(ios_base::in | ios_base::out) {}
  explicit basic_spanbuf(ios_base::openmode which) : basic_spanbuf(std::span<charT>(), which) {}
  explicit basic_spanbuf(std::span<charT> s, ios_base::openmode which = ios_base::in | ios_base::out)
      : mode_(which) {
    span(s);
  }
  basic_spanbuf(const basic_spanbuf&) = delete;
  basic_spanbuf(basic_spanbuf&& rhs) : base(static_cast<base&&>(rhs)), mode_(rhs.mode_), buf_(rhs.buf_) {}

  // [spanbuf.assign]
  basic_spanbuf& operator=(const basic_spanbuf&) = delete;
  basic_spanbuf& operator=(basic_spanbuf&& rhs) {
    basic_spanbuf tmp{static_cast<basic_spanbuf&&>(rhs)};
    this->swap(tmp);
    return *this;
  }
  void swap(basic_spanbuf& rhs) {
    base::swap(rhs);
    const ios_base::openmode m = mode_;
    mode_ = rhs.mode_;
    rhs.mode_ = m;
    const std::span<charT> b = buf_;
    buf_ = rhs.buf_;
    rhs.buf_ = b;
  }

  // [spanbuf.members]
  std::span<charT> span() const noexcept {
    if (mode_ & ios_base::out)
      return std::span<charT>(this->pbase(), this->pptr());
    return buf_;
  }
  void span(std::span<charT> s) noexcept {
    buf_ = s;
    charT* const p = s.data();
    if (mode_ & ios_base::out) {
      this->setp(p, p + s.size());
      if (mode_ & ios_base::ate)
        advance_put(s.size());
    } else {
      this->setp(nullptr, nullptr);
    }
    if (mode_ & ios_base::in)
      this->setg(p, p, p + s.size());
    else
      this->setg(nullptr, nullptr, nullptr);
  }

protected:
  // [spanbuf.virtuals]
  basic_streambuf<charT, traits>* setbuf(charT* s, streamsize n) override {
    this->span(std::span<charT>(s, static_cast<size_t>(n)));
    return this;
  }
  pos_type seekoff(off_type off, ios_base::seekdir way,
                   ios_base::openmode which = ios_base::in | ios_base::out) override {
    const bool in = (which & ios_base::in) != 0, out = (which & ios_base::out) != 0;
    const pos_type fail = pos_type(off_type(-1));
    if (!in && !out)
      return fail;
    if (in && out && way == ios_base::cur)
      return fail;
    // [spanbuf.virtuals]/4-5, for each sequence to be positioned
    off_type newoff_in = 0, newoff_out = 0;
    if (in && !position(way, off, this->gptr(), this->eback(), newoff_in))
      return fail;
    if (out && !position(way, off, this->pptr(), this->pbase(), newoff_out))
      return fail;
    if (in && this->gptr() != nullptr)
      this->setg(this->eback(), this->eback() + newoff_in, this->egptr());
    if (out && this->pptr() != nullptr) {
      this->setp(this->pbase(), this->epptr());
      advance_put(static_cast<size_t>(newoff_out));
    }
    return pos_type(out ? newoff_out : newoff_in);
  }
  pos_type seekpos(pos_type sp, ios_base::openmode which = ios_base::in | ios_base::out) override {
    return seekoff(off_type(sp), ios_base::beg, which);
  }

private:
  bool position(ios_base::seekdir way, off_type off, charT* next, charT* beg, off_type& newoff) const {
    off_type baseoff;
    if (way == ios_base::beg)
      baseoff = 0;
    else if (way == ios_base::cur)
      baseoff = static_cast<off_type>(next - beg);
    else if (way == ios_base::end)
      baseoff = (mode_ & ios_base::out) && !(mode_ & ios_base::in) ? static_cast<off_type>(this->pptr() - this->pbase())
                                                                   : static_cast<off_type>(buf_.size());
    else
      return false;
    off_type r;
    if (__builtin_add_overflow(baseoff, off, &r) || r < 0 || static_cast<size_t>(r) > buf_.size())
      return false;
    if (next == nullptr && r != 0)
      return false;
    newoff = r;
    return true;
  }
  void advance_put(size_t n) noexcept {
    while (n > static_cast<size_t>(__INT_MAX__)) {
      this->pbump(__INT_MAX__);
      n -= static_cast<size_t>(__INT_MAX__);
    }
    this->pbump(static_cast<int>(n));
  }

  ios_base::openmode mode_;
  std::span<charT> buf_;
};

template <class charT, class traits>
void swap(basic_spanbuf<charT, traits>& x, basic_spanbuf<charT, traits>& y) {
  x.swap(y);
}

// [ispanstream]
template <class charT, class traits>
class basic_ispanstream : public basic_istream<charT, traits> {
  using buf_type = basic_spanbuf<charT, traits>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;

  explicit basic_ispanstream(std::span<charT> s, ios_base::openmode which = ios_base::in)
      : basic_istream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which | ios_base::in) {}
  basic_ispanstream(const basic_ispanstream&) = delete;
  basic_ispanstream(basic_ispanstream&& rhs)
      : basic_istream<charT, traits>(static_cast<basic_istream<charT, traits>&&>(rhs)),
        sb_(static_cast<buf_type&&>(rhs.sb_)) {
    basic_istream<charT, traits>::set_rdbuf(__builtin_addressof(sb_));
  }
  template <class ROS>
    requires ranges::borrowed_range<ROS> && (!convertible_to<ROS, std::span<charT>>) &&
             convertible_to<ROS, std::span<const charT>>
  explicit basic_ispanstream(ROS&& s) : basic_ispanstream(as_mutable(static_cast<ROS&&>(s))) {}
  basic_ispanstream& operator=(const basic_ispanstream&) = delete;
  basic_ispanstream& operator=(basic_ispanstream&& rhs) {
    basic_istream<charT, traits>::operator=(static_cast<basic_istream<charT, traits>&&>(rhs));
    sb_ = static_cast<buf_type&&>(rhs.sb_);
    return *this;
  }
  void swap(basic_ispanstream& rhs) {
    basic_istream<charT, traits>::swap(rhs);
    sb_.swap(rhs.sb_);
  }

  buf_type* rdbuf() const noexcept { return const_cast<buf_type*>(__builtin_addressof(sb_)); }
  std::span<const charT> span() const noexcept { return rdbuf()->span(); }
  void span(std::span<charT> s) noexcept { rdbuf()->span(s); }
  template <class ROS>
    requires ranges::borrowed_range<ROS> && (!convertible_to<ROS, std::span<charT>>) &&
             convertible_to<ROS, std::span<const charT>>
  void span(ROS&& s) noexcept {
    this->span(as_mutable(static_cast<ROS&&>(s)));
  }

private:
  template <class ROS>
  static std::span<charT> as_mutable(ROS&& s) noexcept {
    const std::span<const charT> sp(static_cast<ROS&&>(s));
    return std::span<charT>(const_cast<charT*>(sp.data()), sp.size());
  }
  buf_type sb_;
};

template <class charT, class traits>
void swap(basic_ispanstream<charT, traits>& x, basic_ispanstream<charT, traits>& y) {
  x.swap(y);
}

// [ospanstream]
template <class charT, class traits>
class basic_ospanstream : public basic_ostream<charT, traits> {
  using buf_type = basic_spanbuf<charT, traits>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;

  explicit basic_ospanstream(std::span<charT> s, ios_base::openmode which = ios_base::out)
      : basic_ostream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which | ios_base::out) {}
  basic_ospanstream(const basic_ospanstream&) = delete;
  basic_ospanstream(basic_ospanstream&& rhs) noexcept
      : basic_ostream<charT, traits>(static_cast<basic_ostream<charT, traits>&&>(rhs)),
        sb_(static_cast<buf_type&&>(rhs.sb_)) {
    basic_ostream<charT, traits>::set_rdbuf(__builtin_addressof(sb_));
  }
  basic_ospanstream& operator=(const basic_ospanstream&) = delete;
  basic_ospanstream& operator=(basic_ospanstream&& rhs) {
    basic_ostream<charT, traits>::operator=(static_cast<basic_ostream<charT, traits>&&>(rhs));
    sb_ = static_cast<buf_type&&>(rhs.sb_);
    return *this;
  }
  void swap(basic_ospanstream& rhs) {
    basic_ostream<charT, traits>::swap(rhs);
    sb_.swap(rhs.sb_);
  }

  buf_type* rdbuf() const noexcept { return const_cast<buf_type*>(__builtin_addressof(sb_)); }
  std::span<charT> span() const noexcept { return rdbuf()->span(); }
  void span(std::span<charT> s) noexcept { rdbuf()->span(s); }

private:
  buf_type sb_;
};

template <class charT, class traits>
void swap(basic_ospanstream<charT, traits>& x, basic_ospanstream<charT, traits>& y) {
  x.swap(y);
}

// [spanstream]
template <class charT, class traits>
class basic_spanstream : public basic_iostream<charT, traits> {
  using buf_type = basic_spanbuf<charT, traits>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;

  explicit basic_spanstream(std::span<charT> s, ios_base::openmode which = ios_base::out | ios_base::in)
      : basic_iostream<charT, traits>(__builtin_addressof(sb_)), sb_(s, which) {}
  basic_spanstream(const basic_spanstream&) = delete;
  basic_spanstream(basic_spanstream&& rhs)
      : basic_iostream<charT, traits>(static_cast<basic_iostream<charT, traits>&&>(rhs)),
        sb_(static_cast<buf_type&&>(rhs.sb_)) {
    basic_istream<charT, traits>::set_rdbuf(__builtin_addressof(sb_));
  }
  basic_spanstream& operator=(const basic_spanstream&) = delete;
  basic_spanstream& operator=(basic_spanstream&& rhs) {
    basic_iostream<charT, traits>::operator=(static_cast<basic_iostream<charT, traits>&&>(rhs));
    sb_ = static_cast<buf_type&&>(rhs.sb_);
    return *this;
  }
  void swap(basic_spanstream& rhs) {
    basic_iostream<charT, traits>::swap(rhs);
    sb_.swap(rhs.sb_);
  }

  buf_type* rdbuf() const noexcept { return const_cast<buf_type*>(__builtin_addressof(sb_)); }
  std::span<charT> span() const noexcept { return rdbuf()->span(); }
  void span(std::span<charT> s) noexcept { rdbuf()->span(s); }

private:
  buf_type sb_;
};

template <class charT, class traits>
void swap(basic_spanstream<charT, traits>& x, basic_spanstream<charT, traits>& y) {
  x.swap(y);
}

} // namespace std
