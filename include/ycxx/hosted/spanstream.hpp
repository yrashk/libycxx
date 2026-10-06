// libycxx hosted: basic_spanbuf and the span streams ([span.streams]).
#pragma once

#include <ycxx/core/ranges_base.hpp>
#include <ycxx/core/span.hpp>
#include <ycxx/hosted/istream.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class __charT, class __traits>
class basic_spanbuf : public basic_streambuf<__charT, __traits> {
  using base = basic_streambuf<__charT, __traits>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;

  // [spanbuf.cons]
  basic_spanbuf() : basic_spanbuf(ios_base::in | ios_base::out) {}
  explicit basic_spanbuf(ios_base::openmode __which) : basic_spanbuf(std::span<__charT>(), __which) {}
  explicit basic_spanbuf(std::span<__charT> s, ios_base::openmode __which = ios_base::in | ios_base::out)
      : __mode_(__which) {
    span(s);
  }
  basic_spanbuf(const basic_spanbuf&) = delete;
  basic_spanbuf(basic_spanbuf&& __rhs) : base(static_cast<base&&>(__rhs)), __mode_(__rhs.__mode_), __buf_(__rhs.__buf_) {}

  // [spanbuf.assign]
  basic_spanbuf& operator=(const basic_spanbuf&) = delete;
  basic_spanbuf& operator=(basic_spanbuf&& __rhs) {
    basic_spanbuf __tmp{static_cast<basic_spanbuf&&>(__rhs)};
    this->swap(__tmp);
    return *this;
  }
  void swap(basic_spanbuf& __rhs) {
    base::swap(__rhs);
    const ios_base::openmode m = __mode_;
    __mode_ = __rhs.__mode_;
    __rhs.__mode_ = m;
    const std::span<__charT> b = __buf_;
    __buf_ = __rhs.__buf_;
    __rhs.__buf_ = b;
  }

  // [spanbuf.members]
  std::span<__charT> span() const noexcept {
    if (__mode_ & ios_base::out)
      return std::span<__charT>(this->pbase(), this->pptr());
    return __buf_;
  }
  void span(std::span<__charT> s) noexcept {
    __buf_ = s;
    __charT* const p = s.data();
    if (__mode_ & ios_base::out) {
      this->setp(p, p + s.size());
      if (__mode_ & ios_base::ate)
        __advance_put(s.size());
    } else {
      this->setp(nullptr, nullptr);
    }
    if (__mode_ & ios_base::in)
      this->setg(p, p, p + s.size());
    else
      this->setg(nullptr, nullptr, nullptr);
  }

protected:
  // [spanbuf.virtuals]
  basic_streambuf<__charT, __traits>* setbuf(__charT* s, streamsize n) override {
    this->span(std::span<__charT>(s, static_cast<size_t>(n)));
    return this;
  }
  pos_type seekoff(off_type __off, ios_base::seekdir __way,
                   ios_base::openmode __which = ios_base::in | ios_base::out) override {
    const bool in = (__which & ios_base::in) != 0, out = (__which & ios_base::out) != 0;
    const pos_type fail = pos_type(off_type(-1));
    if (!in && !out)
      return fail;
    if (in && out && __way == ios_base::cur)
      return fail;
    // [spanbuf.virtuals]/4-5, for each sequence to be positioned
    off_type __newoff_in = 0, __newoff_out = 0;
    if (in && !position(__way, __off, this->gptr(), this->eback(), __newoff_in))
      return fail;
    if (out && !position(__way, __off, this->pptr(), this->pbase(), __newoff_out))
      return fail;
    if (in && this->gptr() != nullptr)
      this->setg(this->eback(), this->eback() + __newoff_in, this->egptr());
    if (out && this->pptr() != nullptr) {
      this->setp(this->pbase(), this->epptr());
      __advance_put(static_cast<size_t>(__newoff_out));
    }
    return pos_type(out ? __newoff_out : __newoff_in);
  }
  pos_type seekpos(pos_type __sp, ios_base::openmode __which = ios_base::in | ios_base::out) override {
    return seekoff(off_type(__sp), ios_base::beg, __which);
  }

private:
  bool position(ios_base::seekdir __way, off_type __off, __charT* next, __charT* beg, off_type& __newoff) const {
    off_type __baseoff;
    if (__way == ios_base::beg)
      __baseoff = 0;
    else if (__way == ios_base::cur)
      __baseoff = static_cast<off_type>(next - beg);
    else if (__way == ios_base::end)
      __baseoff = (__mode_ & ios_base::out) && !(__mode_ & ios_base::in) ? static_cast<off_type>(this->pptr() - this->pbase())
                                                                   : static_cast<off_type>(__buf_.size());
    else
      return false;
    off_type r;
    if (__builtin_add_overflow(__baseoff, __off, &r) || r < 0 || static_cast<size_t>(r) > __buf_.size())
      return false;
    if (next == nullptr && r != 0)
      return false;
    __newoff = r;
    return true;
  }
  void __advance_put(size_t n) noexcept {
    while (n > static_cast<size_t>(__INT_MAX__)) {
      this->pbump(__INT_MAX__);
      n -= static_cast<size_t>(__INT_MAX__);
    }
    this->pbump(static_cast<int>(n));
  }

  ios_base::openmode __mode_;
  std::span<__charT> __buf_;
};

template <class __charT, class __traits>
void swap(basic_spanbuf<__charT, __traits>& __x, basic_spanbuf<__charT, __traits>& y) {
  __x.swap(y);
}

// [ispanstream]
template <class __charT, class __traits>
class basic_ispanstream : public basic_istream<__charT, __traits> {
  using __buf_type = basic_spanbuf<__charT, __traits>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;

  explicit basic_ispanstream(std::span<__charT> s, ios_base::openmode __which = ios_base::in)
      : basic_istream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which | ios_base::in) {}
  basic_ispanstream(const basic_ispanstream&) = delete;
  basic_ispanstream(basic_ispanstream&& __rhs)
      : basic_istream<__charT, __traits>(static_cast<basic_istream<__charT, __traits>&&>(__rhs)),
        __sb_(static_cast<__buf_type&&>(__rhs.__sb_)) {
    basic_istream<__charT, __traits>::set_rdbuf(__builtin_addressof(__sb_));
  }
  template <class _ROS>
    requires ranges::borrowed_range<_ROS> && (!convertible_to<_ROS, std::span<__charT>>) &&
             convertible_to<_ROS, std::span<const __charT>>
  explicit basic_ispanstream(_ROS&& s) : basic_ispanstream(__as_mutable(static_cast<_ROS&&>(s))) {}
  basic_ispanstream& operator=(const basic_ispanstream&) = delete;
  basic_ispanstream& operator=(basic_ispanstream&& __rhs) {
    basic_istream<__charT, __traits>::operator=(static_cast<basic_istream<__charT, __traits>&&>(__rhs));
    __sb_ = static_cast<__buf_type&&>(__rhs.__sb_);
    return *this;
  }
  void swap(basic_ispanstream& __rhs) {
    basic_istream<__charT, __traits>::swap(__rhs);
    __sb_.swap(__rhs.__sb_);
  }

  __buf_type* rdbuf() const noexcept { return const_cast<__buf_type*>(__builtin_addressof(__sb_)); }
  std::span<const __charT> span() const noexcept { return rdbuf()->span(); }
  void span(std::span<__charT> s) noexcept { rdbuf()->span(s); }
  template <class _ROS>
    requires ranges::borrowed_range<_ROS> && (!convertible_to<_ROS, std::span<__charT>>) &&
             convertible_to<_ROS, std::span<const __charT>>
  void span(_ROS&& s) noexcept {
    this->span(__as_mutable(static_cast<_ROS&&>(s)));
  }

private:
  template <class _ROS>
  static std::span<__charT> __as_mutable(_ROS&& s) noexcept {
    const std::span<const __charT> __sp(static_cast<_ROS&&>(s));
    return std::span<__charT>(const_cast<__charT*>(__sp.data()), __sp.size());
  }
  __buf_type __sb_;
};

template <class __charT, class __traits>
void swap(basic_ispanstream<__charT, __traits>& __x, basic_ispanstream<__charT, __traits>& y) {
  __x.swap(y);
}

// [ospanstream]
template <class __charT, class __traits>
class basic_ospanstream : public basic_ostream<__charT, __traits> {
  using __buf_type = basic_spanbuf<__charT, __traits>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;

  explicit basic_ospanstream(std::span<__charT> s, ios_base::openmode __which = ios_base::out)
      : basic_ostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which | ios_base::out) {}
  basic_ospanstream(const basic_ospanstream&) = delete;
  basic_ospanstream(basic_ospanstream&& __rhs) noexcept
      : basic_ostream<__charT, __traits>(static_cast<basic_ostream<__charT, __traits>&&>(__rhs)),
        __sb_(static_cast<__buf_type&&>(__rhs.__sb_)) {
    basic_ostream<__charT, __traits>::set_rdbuf(__builtin_addressof(__sb_));
  }
  basic_ospanstream& operator=(const basic_ospanstream&) = delete;
  basic_ospanstream& operator=(basic_ospanstream&& __rhs) {
    basic_ostream<__charT, __traits>::operator=(static_cast<basic_ostream<__charT, __traits>&&>(__rhs));
    __sb_ = static_cast<__buf_type&&>(__rhs.__sb_);
    return *this;
  }
  void swap(basic_ospanstream& __rhs) {
    basic_ostream<__charT, __traits>::swap(__rhs);
    __sb_.swap(__rhs.__sb_);
  }

  __buf_type* rdbuf() const noexcept { return const_cast<__buf_type*>(__builtin_addressof(__sb_)); }
  std::span<__charT> span() const noexcept { return rdbuf()->span(); }
  void span(std::span<__charT> s) noexcept { rdbuf()->span(s); }

private:
  __buf_type __sb_;
};

template <class __charT, class __traits>
void swap(basic_ospanstream<__charT, __traits>& __x, basic_ospanstream<__charT, __traits>& y) {
  __x.swap(y);
}

// [spanstream]
template <class __charT, class __traits>
class basic_spanstream : public basic_iostream<__charT, __traits> {
  using __buf_type = basic_spanbuf<__charT, __traits>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;

  explicit basic_spanstream(std::span<__charT> s, ios_base::openmode __which = ios_base::out | ios_base::in)
      : basic_iostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which) {}
  basic_spanstream(const basic_spanstream&) = delete;
  basic_spanstream(basic_spanstream&& __rhs)
      : basic_iostream<__charT, __traits>(static_cast<basic_iostream<__charT, __traits>&&>(__rhs)),
        __sb_(static_cast<__buf_type&&>(__rhs.__sb_)) {
    basic_istream<__charT, __traits>::set_rdbuf(__builtin_addressof(__sb_));
  }
  basic_spanstream& operator=(const basic_spanstream&) = delete;
  basic_spanstream& operator=(basic_spanstream&& __rhs) {
    basic_iostream<__charT, __traits>::operator=(static_cast<basic_iostream<__charT, __traits>&&>(__rhs));
    __sb_ = static_cast<__buf_type&&>(__rhs.__sb_);
    return *this;
  }
  void swap(basic_spanstream& __rhs) {
    basic_iostream<__charT, __traits>::swap(__rhs);
    __sb_.swap(__rhs.__sb_);
  }

  __buf_type* rdbuf() const noexcept { return const_cast<__buf_type*>(__builtin_addressof(__sb_)); }
  std::span<__charT> span() const noexcept { return rdbuf()->span(); }
  void span(std::span<__charT> s) noexcept { rdbuf()->span(s); }

private:
  __buf_type __sb_;
};

template <class __charT, class __traits>
void swap(basic_spanstream<__charT, __traits>& __x, basic_spanstream<__charT, __traits>& y) {
  __x.swap(y);
}

} // namespace std
