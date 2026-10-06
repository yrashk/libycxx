// libycxx hosted: basic_streambuf ([streambuf]).
#pragma once

#include <ycxx/hosted/ios.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// basic_syncbuf's base marks itself in its basic_streambuf part, so the emit_on_flush family of
// manipulators can recognize one without RTTI.
struct __streambuf_tag_access {
  template <class __charT, class __traits>
  static bool __is_syncbuf(const std::basic_streambuf<__charT, __traits>& __sb) noexcept {
    return __sb.__is_syncbuf_;
  }
  template <class __charT, class __traits>
  static void __set_syncbuf(std::basic_streambuf<__charT, __traits>& __sb) noexcept {
    __sb.__is_syncbuf_ = true;
  }
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class __charT, class __traits>
class basic_streambuf {
public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;

  virtual ~basic_streambuf() {}

  // [streambuf.locales]
  locale pubimbue(const locale& __loc) {
    imbue(__loc);
    locale __old = __loc_;
    __loc_ = __loc;
    return __old;
  }
  locale getloc() const { return __loc_; }

  // [streambuf.buffer]
  basic_streambuf* pubsetbuf(char_type* s, streamsize n) { return setbuf(s, n); }
  pos_type pubseekoff(off_type __off, ios_base::seekdir __way, ios_base::openmode __which = ios_base::in | ios_base::out) {
    return seekoff(__off, __way, __which);
  }
  pos_type pubseekpos(pos_type __sp, ios_base::openmode __which = ios_base::in | ios_base::out) {
    return seekpos(__sp, __which);
  }
  int pubsync() { return sync(); }

  // [streambuf.pub.get]
  streamsize in_avail() {
    if (__gnext_ < __gend_)
      return static_cast<streamsize>(__gend_ - __gnext_);
    return showmanyc();
  }
  int_type snextc() {
    if (__traits::eq_int_type(sbumpc(), __traits::eof()))
      return __traits::eof();
    return sgetc();
  }
  int_type sbumpc() {
    if (__gnext_ < __gend_)
      return __traits::to_int_type(*__gnext_++);
    return uflow();
  }
  int_type sgetc() {
    if (__gnext_ < __gend_)
      return __traits::to_int_type(*__gnext_);
    return underflow();
  }
  streamsize sgetn(char_type* s, streamsize n) { return xsgetn(s, n); }

  // [streambuf.pub.pback]
  int_type sputbackc(char_type c) {
    if (__gbeg_ < __gnext_ && __traits::__eq(c, __gnext_[-1]))
      return __traits::to_int_type(*--__gnext_);
    return pbackfail(__traits::to_int_type(c));
  }
  int_type sungetc() {
    if (__gbeg_ < __gnext_)
      return __traits::to_int_type(*--__gnext_);
    return pbackfail();
  }

  // [streambuf.pub.put]
  int_type sputc(char_type c) {
    if (__pnext_ < __pend_) {
      *__pnext_++ = c;
      return __traits::to_int_type(c);
    }
    return overflow(__traits::to_int_type(c));
  }
  streamsize sputn(const char_type* s, streamsize n) { return xsputn(s, n); }

protected:
  basic_streambuf() : __loc_() {}
  basic_streambuf(const basic_streambuf& __rhs) = default;
  basic_streambuf& operator=(const basic_streambuf& __rhs) = default;
  void swap(basic_streambuf& __rhs) {
    char_type** __mine[] = {&__gbeg_, &__gnext_, &__gend_, &__pbeg_, &__pnext_, &__pend_};
    char_type** __theirs[] = {&__rhs.__gbeg_, &__rhs.__gnext_, &__rhs.__gend_, &__rhs.__pbeg_, &__rhs.__pnext_, &__rhs.__pend_};
    for (int i = 0; i < 6; ++i) {
      char_type* t = *__mine[i];
      *__mine[i] = *__theirs[i];
      *__theirs[i] = t;
    }
    locale __l = __loc_;
    __loc_ = __rhs.__loc_;
    __rhs.__loc_ = __l;
  }

  // [streambuf.get.area]
  char_type* eback() const { return __gbeg_; }
  char_type* gptr() const { return __gnext_; }
  char_type* egptr() const { return __gend_; }
  void gbump(int n) { __gnext_ += n; }
  void setg(char_type* __gbeg, char_type* __gnext, char_type* __gend) {
    __gbeg_ = __gbeg;
    __gnext_ = __gnext;
    __gend_ = __gend;
  }

  // [streambuf.put.area]
  char_type* pbase() const { return __pbeg_; }
  char_type* pptr() const { return __pnext_; }
  char_type* epptr() const { return __pend_; }
  void pbump(int n) { __pnext_ += n; }
  void setp(char_type* __pbeg, char_type* __pend) {
    __pbeg_ = __pbeg;
    __pnext_ = __pbeg;
    __pend_ = __pend;
  }

  // [streambuf.virt.locales]
  virtual void imbue(const locale&) {}

  // [streambuf.virt.buffer]
  virtual basic_streambuf* setbuf(char_type*, streamsize) { return this; }
  virtual pos_type seekoff(off_type, ios_base::seekdir, ios_base::openmode = ios_base::in | ios_base::out) {
    return pos_type(off_type(-1));
  }
  virtual pos_type seekpos(pos_type, ios_base::openmode = ios_base::in | ios_base::out) {
    return pos_type(off_type(-1));
  }
  virtual int sync() { return 0; }

  // [streambuf.virt.get]
  virtual streamsize showmanyc() { return 0; }
  virtual streamsize xsgetn(char_type* s, streamsize n) {
    streamsize done = 0;
    while (done < n) {
      if (__gnext_ < __gend_) {
        streamsize k = static_cast<streamsize>(__gend_ - __gnext_);
        if (k > n - done)
          k = n - done;
        __traits::copy(s + done, __gnext_, static_cast<size_t>(k));
        __gnext_ += k;
        done += k;
      } else {
        const int_type c = uflow();
        if (__traits::eq_int_type(c, __traits::eof()))
          break;
        s[done++] = __traits::to_char_type(c);
      }
    }
    return done;
  }
  virtual int_type underflow() { return __traits::eof(); }
  virtual int_type uflow() {
    if (__traits::eq_int_type(underflow(), __traits::eof()))
      return __traits::eof();
    return __traits::to_int_type(*__gnext_++);
  }

  // [streambuf.virt.pback]
  virtual int_type pbackfail(int_type = __traits::eof()) { return __traits::eof(); }

  // [streambuf.virt.put]
  virtual streamsize xsputn(const char_type* s, streamsize n) {
    streamsize done = 0;
    while (done < n) {
      if (__pnext_ < __pend_) {
        streamsize k = static_cast<streamsize>(__pend_ - __pnext_);
        if (k > n - done)
          k = n - done;
        __traits::copy(__pnext_, s + done, static_cast<size_t>(k));
        __pnext_ += k;
        done += k;
      } else {
        if (__traits::eq_int_type(overflow(__traits::to_int_type(s[done])), __traits::eof()))
          break;
        ++done;
      }
    }
    return done;
  }
  virtual int_type overflow(int_type = __traits::eof()) { return __traits::eof(); }

private:
  char_type* __gbeg_ = nullptr;
  char_type* __gnext_ = nullptr;
  char_type* __gend_ = nullptr;
  char_type* __pbeg_ = nullptr;
  char_type* __pnext_ = nullptr;
  char_type* __pend_ = nullptr;
  locale __loc_;
  bool __is_syncbuf_ = false;
  friend __ycxx::__detail::__streambuf_tag_access;
};

} // namespace std
