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

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class __charT, class __traits, class _Allocator>
class basic_stringbuf : public basic_streambuf<__charT, __traits> {
  using string_type = basic_string<__charT, __traits, _Allocator>;
  using __sv_type = basic_string_view<__charT, __traits>;
  using base = basic_streambuf<__charT, __traits>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using allocator_type = _Allocator;

  // [stringbuf.cons]
  basic_stringbuf() : basic_stringbuf(ios_base::in | ios_base::out) {}
  explicit basic_stringbuf(ios_base::openmode __which) : __mode_(__which) { __init_buf_ptrs(); }
  explicit basic_stringbuf(const string_type& s, ios_base::openmode __which = ios_base::in | ios_base::out)
      : __mode_(__which), __buf_(s) {
    __init_buf_ptrs();
  }
  explicit basic_stringbuf(const _Allocator& a) : basic_stringbuf(ios_base::in | ios_base::out, a) {}
  basic_stringbuf(ios_base::openmode __which, const _Allocator& a) : __mode_(__which), __buf_(a) { __init_buf_ptrs(); }
  explicit basic_stringbuf(string_type&& s, ios_base::openmode __which = ios_base::in | ios_base::out)
      : __mode_(__which), __buf_(static_cast<string_type&&>(s)) {
    __init_buf_ptrs();
  }
  template <class _SAlloc>
  basic_stringbuf(const basic_string<__charT, __traits, _SAlloc>& s, const _Allocator& a)
      : basic_stringbuf(s, ios_base::in | ios_base::out, a) {}
  template <class _SAlloc>
  basic_stringbuf(const basic_string<__charT, __traits, _SAlloc>& s, ios_base::openmode __which, const _Allocator& a)
      : __mode_(__which), __buf_(s.data(), s.size(), a) {
    __init_buf_ptrs();
  }
  template <class _SAlloc>
    requires(!is_same_v<_SAlloc, _Allocator>)
  explicit basic_stringbuf(const basic_string<__charT, __traits, _SAlloc>& s,
                           ios_base::openmode __which = ios_base::in | ios_base::out)
      : __mode_(__which), __buf_(s.data(), s.size()) {
    __init_buf_ptrs();
  }
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  explicit basic_stringbuf(const _Tp& t, ios_base::openmode __which = ios_base::in | ios_base::out)
      : basic_stringbuf(t, __which, _Allocator()) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  basic_stringbuf(const _Tp& t, const _Allocator& a) : basic_stringbuf(t, ios_base::in | ios_base::out, a) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  basic_stringbuf(const _Tp& t, ios_base::openmode __which, const _Allocator& a)
      : __mode_(__which), __buf_(__sv_type(t), a) {
    __init_buf_ptrs();
  }
  basic_stringbuf(const basic_stringbuf&) = delete;
  basic_stringbuf(basic_stringbuf&& __rhs) : base(__rhs), __mode_(__rhs.__mode_) {
    const __offsets __o = __rhs.save();
    __buf_ = static_cast<string_type&&>(__rhs.__buf_);
    __restore(__o);
    __rhs.reset();
  }
  basic_stringbuf(basic_stringbuf&& __rhs, const _Allocator& a) : base(__rhs), __mode_(__rhs.__mode_), __buf_(a) {
    const __offsets __o = __rhs.save();
    __buf_ = string_type(static_cast<string_type&&>(__rhs.__buf_), a);
    __restore(__o);
    __rhs.reset();
  }

  // [stringbuf.assign]
  basic_stringbuf& operator=(const basic_stringbuf&) = delete;
  basic_stringbuf& operator=(basic_stringbuf&& __rhs) {
    if (this == __builtin_addressof(__rhs))
      return *this;
    const __offsets __o = __rhs.save();
    base::operator=(__rhs);
    __mode_ = __rhs.__mode_;
    __buf_ = static_cast<string_type&&>(__rhs.__buf_);
    __restore(__o);
    __rhs.reset();
    return *this;
  }
  void swap(basic_stringbuf& __rhs) noexcept(allocator_traits<_Allocator>::propagate_on_container_swap::value ||
                                           allocator_traits<_Allocator>::is_always_equal::value) {
    const __offsets __mine = save(), __theirs = __rhs.save();
    base::swap(__rhs);
    const ios_base::openmode m = __mode_;
    __mode_ = __rhs.__mode_;
    __rhs.__mode_ = m;
    __buf_.swap(__rhs.__buf_);
    __restore(__theirs);
    __rhs.__restore(__mine);
  }

  // [stringbuf.members]
  allocator_type get_allocator() const noexcept { return __buf_.get_allocator(); }
  string_type str() const& { return string_type(view(), get_allocator()); }
  template <class _SAlloc>
    requires __ycxx::__detail::__qualifies_as_allocator<_SAlloc>
  basic_string<__charT, __traits, _SAlloc> str(const _SAlloc& __sa) const {
    return basic_string<__charT, __traits, _SAlloc>(view(), __sa);
  }
  string_type str() && {
    const __sv_type __v = view();
    const size_t start = __v.empty() ? 0 : static_cast<size_t>(__v.data() - __buf_.data());
    if (start != 0)
      __buf_.erase(0, start);
    __buf_.resize(__v.size());
    string_type r = static_cast<string_type&&>(__buf_);
    __buf_.clear();
    __init_buf_ptrs();
    return r;
  }
  basic_string_view<__charT, __traits> view() const noexcept {
    if (__mode_ & ios_base::out)
      return __sv_type(this->pbase(), __high_mark());
    if (__mode_ & ios_base::in)
      return __sv_type(this->eback(), static_cast<size_t>(this->egptr() - this->eback()));
    return __sv_type();
  }
  void str(const string_type& s) {
    __buf_ = s;
    __init_buf_ptrs();
  }
  template <class _SAlloc>
    requires(!is_same_v<_SAlloc, _Allocator>)
  void str(const basic_string<__charT, __traits, _SAlloc>& s) {
    __buf_.assign(s.data(), s.size());
    __init_buf_ptrs();
  }
  void str(string_type&& s) {
    __buf_ = static_cast<string_type&&>(s);
    __init_buf_ptrs();
  }
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  void str(const _Tp& t) {
    const __sv_type sv = t;
    __buf_ = sv;
    __init_buf_ptrs();
  }

protected:
  // [stringbuf.virtuals]
  // Not in the draft's list of overriders: characters written through the put area (sputc
  // does not call a virtual function while it has room) are part of the input sequence too
  // ([stringbuf.virtuals]/1), so in_avail() counts them.
  streamsize showmanyc() override {
    if (!(__mode_ & ios_base::in) || this->gptr() == nullptr)
      return 0;
    if (__mode_ & ios_base::out)
      __extend_get_area();
    return static_cast<streamsize>(this->egptr() - this->gptr());
  }
  int_type underflow() override {
    if (!(__mode_ & ios_base::in) || this->gptr() == nullptr)
      return __traits::eof();
    if (__mode_ & ios_base::out)
      __extend_get_area();
    if (this->gptr() < this->egptr())
      return __traits::to_int_type(*this->gptr());
    return __traits::eof();
  }
  int_type pbackfail(int_type c = __traits::eof()) override {
    if (this->eback() == nullptr || !(this->eback() < this->gptr()))
      return __traits::eof();
    if (__traits::eq_int_type(c, __traits::eof())) {
      this->gbump(-1);
      return __traits::not_eof(c);
    }
    if (__traits::eq(__traits::to_char_type(c), this->gptr()[-1])) {
      this->gbump(-1);
      return c;
    }
    if (__mode_ & ios_base::out) {
      this->gbump(-1);
      *this->gptr() = __traits::to_char_type(c);
      return c;
    }
    return __traits::eof();
  }
  int_type overflow(int_type c = __traits::eof()) override {
    if (__traits::eq_int_type(c, __traits::eof()))
      return __traits::not_eof(c);
    if (!(__mode_ & ios_base::out))
      return __traits::eof();
    if (this->pptr() == this->epptr()) {
      // [stringbuf.virtuals]/8: a larger array, holding the old one plus a write position
      if (__buf_.size() >= __buf_.max_size())
        return __traits::eof();
      __offsets __o = save();
      size_t __want = __buf_.size() < 16 ? 32 : __buf_.size() + __buf_.size() / 2;
      if (__want > __buf_.max_size() || __want < __buf_.size())
        __want = __buf_.max_size();
      __buf_.resize(__want);
      __o.put = true;
      __o.get = (__mode_ & ios_base::in) != 0;
      __restore(__o);
    }
    *this->pptr() = __traits::to_char_type(c);
    this->pbump(1);
    if (__mode_ & ios_base::in)
      __extend_get_area();
    return c;
  }
  basic_streambuf<__charT, __traits>* setbuf(__charT*, streamsize) override { return this; }
  pos_type seekoff(off_type __off, ios_base::seekdir __way,
                   ios_base::openmode __which = ios_base::in | ios_base::out) override {
    const bool in = (__which & ios_base::in) != 0, out = (__which & ios_base::out) != 0;
    if (!in && !out)
      return pos_type(off_type(-1));
    if (in && out && __way == ios_base::cur)
      return pos_type(off_type(-1));
    const off_type __hm = static_cast<off_type>(__mode_ & ios_base::out ? __high_mark() : __in_end());
    off_type __newoff;
    if (__way == ios_base::beg)
      __newoff = 0;
    else if (__way == ios_base::cur)
      __newoff = in ? static_cast<off_type>(this->gptr() - this->eback()) : static_cast<off_type>(this->pptr() - this->pbase());
    else if (__way == ios_base::end)
      __newoff = __hm;
    else
      return pos_type(off_type(-1));
    if ((in && this->gptr() == nullptr && __newoff != 0) || (out && this->pptr() == nullptr && __newoff != 0))
      return pos_type(off_type(-1));
    // positioning a sequence the mode does not have fails, unless it stays at 0
    if ((in && !(__mode_ & ios_base::in)) || (out && !(__mode_ & ios_base::out)))
      if (__newoff + __off != 0)
        return pos_type(off_type(-1));
    if (__off < -__newoff || __off > __hm - __newoff)
      return pos_type(off_type(-1));
    __newoff += __off;
    if (in && this->gptr() != nullptr) {
      if (__mode_ & ios_base::out)
        __extend_get_area();
      this->setg(this->eback(), this->eback() + __newoff, this->egptr());
    }
    if (out && this->pptr() != nullptr) {
      const off_type __keep = static_cast<off_type>(__high_mark());
      this->setp(this->pbase(), this->epptr());
      __advance_put(static_cast<size_t>(__newoff));
      __hm_ = static_cast<size_t>(__keep);
    }
    return pos_type(__newoff);
  }
  pos_type seekpos(pos_type __sp, ios_base::openmode __which = ios_base::in | ios_base::out) override {
    const pos_type r = seekoff(off_type(__sp), ios_base::beg, __which);
    return r == pos_type(off_type(-1)) ? r : __sp;
  }

private:
  // the pointers as offsets into buf_ (for moving and swapping, which can move the characters)
  struct __offsets {
    bool get, put;
    size_t __gnext, __gend, __pnext, __hm;
  };
  __offsets save() const noexcept {
    __offsets __o{};
    __o.get = this->eback() != nullptr;
    __o.put = this->pbase() != nullptr;
    if (__o.get) {
      __o.__gnext = static_cast<size_t>(this->gptr() - this->eback());
      __o.__gend = static_cast<size_t>(this->egptr() - this->eback());
    }
    if (__o.put)
      __o.__pnext = static_cast<size_t>(this->pptr() - this->pbase());
    __o.__hm = __mode_ & ios_base::out ? __high_mark() : 0;
    return __o;
  }
  void __restore(const __offsets& __o) noexcept {
    __charT* p = __buf_.data();
    if (__o.get)
      this->setg(p, p + __o.__gnext, p + __o.__gend);
    else
      this->setg(nullptr, nullptr, nullptr);
    if (__o.put) {
      this->setp(p, p + __buf_.size());
      __advance_put(__o.__pnext);
    } else {
      this->setp(nullptr, nullptr);
    }
    __hm_ = __o.__hm;
  }
  // rhs after a move: empty, as if std::move(rhs).str() had been called
  void reset() {
    __buf_.clear();
    __init_buf_ptrs();
  }

  void __advance_put(size_t n) noexcept {
    while (n > static_cast<size_t>(__INT_MAX__)) {
      this->pbump(__INT_MAX__);
      n -= static_cast<size_t>(__INT_MAX__);
    }
    this->pbump(static_cast<int>(n));
  }
  size_t __high_mark() const noexcept {
    const size_t p = static_cast<size_t>(this->pptr() - this->pbase());
    return p > __hm_ ? p : __hm_;
  }
  size_t __in_end() const noexcept { return static_cast<size_t>(this->egptr() - this->eback()); }
  // the get area ends at the high mark (shared buffer with ios_base::out)
  void __extend_get_area() noexcept {
    __hm_ = __high_mark();
    this->setg(this->eback(), this->gptr(), this->pbase() + __hm_);
  }

  // [stringbuf.members]/2-3
  void __init_buf_ptrs() {
    __hm_ = __buf_.size();
    if (__mode_ & ios_base::out) {
      __buf_.resize(__buf_.capacity() > __hm_ ? __buf_.capacity() : __hm_);
      __charT* p = __buf_.data();
      this->setp(p, p + __buf_.size());
      if (__mode_ & ios_base::ate)
        __advance_put(__hm_);
    } else {
      this->setp(nullptr, nullptr);
    }
    __charT* p = __buf_.data();
    if (__mode_ & ios_base::in)
      this->setg(p, p, p + __hm_);
    else
      this->setg(nullptr, nullptr, nullptr);
  }

  ios_base::openmode __mode_;
  string_type __buf_;
  size_t __hm_ = 0; // the high mark, as a count (see high_mark())
};

template <class __charT, class __traits, class _Allocator>
void swap(basic_stringbuf<__charT, __traits, _Allocator>& __x, basic_stringbuf<__charT, __traits, _Allocator>& y) noexcept(
    noexcept(__x.swap(y))) {
  __x.swap(y);
}

// [istringstream]
template <class __charT, class __traits, class _Allocator>
class basic_istringstream : public basic_istream<__charT, __traits> {
  using string_type = basic_string<__charT, __traits, _Allocator>;
  using __buf_type = basic_stringbuf<__charT, __traits, _Allocator>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using allocator_type = _Allocator;

  basic_istringstream() : basic_istringstream(ios_base::in) {}
  explicit basic_istringstream(ios_base::openmode __which)
      : basic_istream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(__which | ios_base::in) {}
  explicit basic_istringstream(const string_type& s, ios_base::openmode __which = ios_base::in)
      : basic_istream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which | ios_base::in) {}
  basic_istringstream(ios_base::openmode __which, const _Allocator& a)
      : basic_istream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(__which | ios_base::in, a) {}
  explicit basic_istringstream(string_type&& s, ios_base::openmode __which = ios_base::in)
      : basic_istream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(static_cast<string_type&&>(s), __which | ios_base::in) {}
  template <class _SAlloc>
  basic_istringstream(const basic_string<__charT, __traits, _SAlloc>& s, const _Allocator& a)
      : basic_istringstream(s, ios_base::in, a) {}
  template <class _SAlloc>
  basic_istringstream(const basic_string<__charT, __traits, _SAlloc>& s, ios_base::openmode __which, const _Allocator& a)
      : basic_istream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which | ios_base::in, a) {}
  template <class _SAlloc>
    requires(!is_same_v<_SAlloc, _Allocator>)
  explicit basic_istringstream(const basic_string<__charT, __traits, _SAlloc>& s, ios_base::openmode __which = ios_base::in)
      : basic_istream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which | ios_base::in) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  explicit basic_istringstream(const _Tp& t, ios_base::openmode __which = ios_base::in)
      : basic_istringstream(t, __which, _Allocator()) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  basic_istringstream(const _Tp& t, const _Allocator& a) : basic_istringstream(t, ios_base::in, a) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  basic_istringstream(const _Tp& t, ios_base::openmode __which, const _Allocator& a)
      : basic_istream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(t, __which | ios_base::in, a) {}
  basic_istringstream(const basic_istringstream&) = delete;
  basic_istringstream(basic_istringstream&& __rhs)
      : basic_istream<__charT, __traits>(static_cast<basic_istream<__charT, __traits>&&>(__rhs)),
        __sb_(static_cast<__buf_type&&>(__rhs.__sb_)) {
    basic_istream<__charT, __traits>::set_rdbuf(__builtin_addressof(__sb_));
  }
  basic_istringstream& operator=(const basic_istringstream&) = delete;
  basic_istringstream& operator=(basic_istringstream&& __rhs) {
    basic_istream<__charT, __traits>::operator=(static_cast<basic_istream<__charT, __traits>&&>(__rhs));
    __sb_ = static_cast<__buf_type&&>(__rhs.__sb_);
    return *this;
  }
  void swap(basic_istringstream& __rhs) {
    basic_istream<__charT, __traits>::swap(__rhs);
    __sb_.swap(__rhs.__sb_);
  }

  __buf_type* rdbuf() const { return const_cast<__buf_type*>(__builtin_addressof(__sb_)); }
  string_type str() const& { return rdbuf()->str(); }
  template <class _SAlloc>
    requires __ycxx::__detail::__qualifies_as_allocator<_SAlloc>
  basic_string<__charT, __traits, _SAlloc> str(const _SAlloc& __sa) const {
    return rdbuf()->str(__sa);
  }
  string_type str() && { return static_cast<__buf_type&&>(*rdbuf()).str(); }
  basic_string_view<__charT, __traits> view() const noexcept { return rdbuf()->view(); }
  void str(const string_type& s) { rdbuf()->str(s); }
  template <class _SAlloc>
    requires(!is_same_v<_SAlloc, _Allocator>)
  void str(const basic_string<__charT, __traits, _SAlloc>& s) {
    rdbuf()->str(s);
  }
  void str(string_type&& s) { rdbuf()->str(static_cast<string_type&&>(s)); }
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  void str(const _Tp& t) {
    rdbuf()->str(t);
  }

private:
  __buf_type __sb_;
};

template <class __charT, class __traits, class _Allocator>
void swap(basic_istringstream<__charT, __traits, _Allocator>& __x, basic_istringstream<__charT, __traits, _Allocator>& y) {
  __x.swap(y);
}

// [ostringstream]
template <class __charT, class __traits, class _Allocator>
class basic_ostringstream : public basic_ostream<__charT, __traits> {
  using string_type = basic_string<__charT, __traits, _Allocator>;
  using __buf_type = basic_stringbuf<__charT, __traits, _Allocator>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using allocator_type = _Allocator;

  basic_ostringstream() : basic_ostringstream(ios_base::out) {}
  explicit basic_ostringstream(ios_base::openmode __which)
      : basic_ostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(__which | ios_base::out) {}
  explicit basic_ostringstream(const string_type& s, ios_base::openmode __which = ios_base::out)
      : basic_ostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which | ios_base::out) {}
  basic_ostringstream(ios_base::openmode __which, const _Allocator& a)
      : basic_ostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(__which | ios_base::out, a) {}
  explicit basic_ostringstream(string_type&& s, ios_base::openmode __which = ios_base::out)
      : basic_ostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(static_cast<string_type&&>(s), __which | ios_base::out) {}
  template <class _SAlloc>
  basic_ostringstream(const basic_string<__charT, __traits, _SAlloc>& s, const _Allocator& a)
      : basic_ostringstream(s, ios_base::out, a) {}
  template <class _SAlloc>
  basic_ostringstream(const basic_string<__charT, __traits, _SAlloc>& s, ios_base::openmode __which, const _Allocator& a)
      : basic_ostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which | ios_base::out, a) {}
  template <class _SAlloc>
    requires(!is_same_v<_SAlloc, _Allocator>)
  explicit basic_ostringstream(const basic_string<__charT, __traits, _SAlloc>& s, ios_base::openmode __which = ios_base::out)
      : basic_ostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which | ios_base::out) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  explicit basic_ostringstream(const _Tp& t, ios_base::openmode __which = ios_base::out)
      : basic_ostringstream(t, __which, _Allocator()) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  basic_ostringstream(const _Tp& t, const _Allocator& a) : basic_ostringstream(t, ios_base::out, a) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  basic_ostringstream(const _Tp& t, ios_base::openmode __which, const _Allocator& a)
      : basic_ostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(t, __which | ios_base::out, a) {}
  basic_ostringstream(const basic_ostringstream&) = delete;
  basic_ostringstream(basic_ostringstream&& __rhs)
      : basic_ostream<__charT, __traits>(static_cast<basic_ostream<__charT, __traits>&&>(__rhs)),
        __sb_(static_cast<__buf_type&&>(__rhs.__sb_)) {
    basic_ostream<__charT, __traits>::set_rdbuf(__builtin_addressof(__sb_));
  }
  basic_ostringstream& operator=(const basic_ostringstream&) = delete;
  basic_ostringstream& operator=(basic_ostringstream&& __rhs) {
    basic_ostream<__charT, __traits>::operator=(static_cast<basic_ostream<__charT, __traits>&&>(__rhs));
    __sb_ = static_cast<__buf_type&&>(__rhs.__sb_);
    return *this;
  }
  void swap(basic_ostringstream& __rhs) {
    basic_ostream<__charT, __traits>::swap(__rhs);
    __sb_.swap(__rhs.__sb_);
  }

  __buf_type* rdbuf() const { return const_cast<__buf_type*>(__builtin_addressof(__sb_)); }
  string_type str() const& { return rdbuf()->str(); }
  template <class _SAlloc>
    requires __ycxx::__detail::__qualifies_as_allocator<_SAlloc>
  basic_string<__charT, __traits, _SAlloc> str(const _SAlloc& __sa) const {
    return rdbuf()->str(__sa);
  }
  string_type str() && { return static_cast<__buf_type&&>(*rdbuf()).str(); }
  basic_string_view<__charT, __traits> view() const noexcept { return rdbuf()->view(); }
  void str(const string_type& s) { rdbuf()->str(s); }
  template <class _SAlloc>
    requires(!is_same_v<_SAlloc, _Allocator>)
  void str(const basic_string<__charT, __traits, _SAlloc>& s) {
    rdbuf()->str(s);
  }
  void str(string_type&& s) { rdbuf()->str(static_cast<string_type&&>(s)); }
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  void str(const _Tp& t) {
    rdbuf()->str(t);
  }

private:
  __buf_type __sb_;
};

template <class __charT, class __traits, class _Allocator>
void swap(basic_ostringstream<__charT, __traits, _Allocator>& __x, basic_ostringstream<__charT, __traits, _Allocator>& y) {
  __x.swap(y);
}

// [stringstream]
template <class __charT, class __traits, class _Allocator>
class basic_stringstream : public basic_iostream<__charT, __traits> {
  using string_type = basic_string<__charT, __traits, _Allocator>;
  using __buf_type = basic_stringbuf<__charT, __traits, _Allocator>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using allocator_type = _Allocator;

  basic_stringstream() : basic_stringstream(ios_base::out | ios_base::in) {}
  explicit basic_stringstream(ios_base::openmode __which)
      : basic_iostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(__which) {}
  explicit basic_stringstream(const string_type& s, ios_base::openmode __which = ios_base::out | ios_base::in)
      : basic_iostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which) {}
  basic_stringstream(ios_base::openmode __which, const _Allocator& a)
      : basic_iostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(__which, a) {}
  explicit basic_stringstream(string_type&& s, ios_base::openmode __which = ios_base::out | ios_base::in)
      : basic_iostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(static_cast<string_type&&>(s), __which) {}
  template <class _SAlloc>
  basic_stringstream(const basic_string<__charT, __traits, _SAlloc>& s, const _Allocator& a)
      : basic_stringstream(s, ios_base::out | ios_base::in, a) {}
  template <class _SAlloc>
  basic_stringstream(const basic_string<__charT, __traits, _SAlloc>& s, ios_base::openmode __which, const _Allocator& a)
      : basic_iostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which, a) {}
  template <class _SAlloc>
    requires(!is_same_v<_SAlloc, _Allocator>)
  explicit basic_stringstream(const basic_string<__charT, __traits, _SAlloc>& s,
                              ios_base::openmode __which = ios_base::out | ios_base::in)
      : basic_iostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(s, __which) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  explicit basic_stringstream(const _Tp& t, ios_base::openmode __which = ios_base::out | ios_base::in)
      : basic_stringstream(t, __which, _Allocator()) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  basic_stringstream(const _Tp& t, const _Allocator& a) : basic_stringstream(t, ios_base::out | ios_base::in, a) {}
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  basic_stringstream(const _Tp& t, ios_base::openmode __which, const _Allocator& a)
      : basic_iostream<__charT, __traits>(__builtin_addressof(__sb_)), __sb_(t, __which, a) {}
  basic_stringstream(const basic_stringstream&) = delete;
  basic_stringstream(basic_stringstream&& __rhs)
      : basic_iostream<__charT, __traits>(static_cast<basic_iostream<__charT, __traits>&&>(__rhs)),
        __sb_(static_cast<__buf_type&&>(__rhs.__sb_)) {
    basic_istream<__charT, __traits>::set_rdbuf(__builtin_addressof(__sb_));
  }
  basic_stringstream& operator=(const basic_stringstream&) = delete;
  basic_stringstream& operator=(basic_stringstream&& __rhs) {
    basic_iostream<__charT, __traits>::operator=(static_cast<basic_iostream<__charT, __traits>&&>(__rhs));
    __sb_ = static_cast<__buf_type&&>(__rhs.__sb_);
    return *this;
  }
  void swap(basic_stringstream& __rhs) {
    basic_iostream<__charT, __traits>::swap(__rhs);
    __sb_.swap(__rhs.__sb_);
  }

  __buf_type* rdbuf() const { return const_cast<__buf_type*>(__builtin_addressof(__sb_)); }
  string_type str() const& { return rdbuf()->str(); }
  template <class _SAlloc>
    requires __ycxx::__detail::__qualifies_as_allocator<_SAlloc>
  basic_string<__charT, __traits, _SAlloc> str(const _SAlloc& __sa) const {
    return rdbuf()->str(__sa);
  }
  string_type str() && { return static_cast<__buf_type&&>(*rdbuf()).str(); }
  basic_string_view<__charT, __traits> view() const noexcept { return rdbuf()->view(); }
  void str(const string_type& s) { rdbuf()->str(s); }
  template <class _SAlloc>
    requires(!is_same_v<_SAlloc, _Allocator>)
  void str(const basic_string<__charT, __traits, _SAlloc>& s) {
    rdbuf()->str(s);
  }
  void str(string_type&& s) { rdbuf()->str(static_cast<string_type&&>(s)); }
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  void str(const _Tp& t) {
    rdbuf()->str(t);
  }

private:
  __buf_type __sb_;
};

template <class __charT, class __traits, class _Allocator>
void swap(basic_stringstream<__charT, __traits, _Allocator>& __x, basic_stringstream<__charT, __traits, _Allocator>& y) {
  __x.swap(y);
}

}} // namespace std
