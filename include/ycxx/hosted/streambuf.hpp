// libycxx hosted: basic_streambuf ([streambuf]).
#pragma once

#include <ycxx/hosted/ios.hpp>

namespace ycxx::detail {
// basic_syncbuf's base marks itself in its basic_streambuf part, so the emit_on_flush family of
// manipulators can recognize one without RTTI.
struct streambuf_tag_access {
  template <class charT, class traits>
  static bool is_syncbuf(const std::basic_streambuf<charT, traits>& sb) noexcept {
    return sb.is_syncbuf_;
  }
  template <class charT, class traits>
  static void set_syncbuf(std::basic_streambuf<charT, traits>& sb) noexcept {
    sb.is_syncbuf_ = true;
  }
};
} // namespace ycxx::detail

namespace std {

template <class charT, class traits>
class basic_streambuf {
public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;

  virtual ~basic_streambuf() {}

  // [streambuf.locales]
  locale pubimbue(const locale& loc) {
    imbue(loc);
    locale old = loc_;
    loc_ = loc;
    return old;
  }
  locale getloc() const { return loc_; }

  // [streambuf.buffer]
  basic_streambuf* pubsetbuf(char_type* s, streamsize n) { return setbuf(s, n); }
  pos_type pubseekoff(off_type off, ios_base::seekdir way, ios_base::openmode which = ios_base::in | ios_base::out) {
    return seekoff(off, way, which);
  }
  pos_type pubseekpos(pos_type sp, ios_base::openmode which = ios_base::in | ios_base::out) {
    return seekpos(sp, which);
  }
  int pubsync() { return sync(); }

  // [streambuf.pub.get]
  streamsize in_avail() {
    if (gnext_ < gend_)
      return static_cast<streamsize>(gend_ - gnext_);
    return showmanyc();
  }
  int_type snextc() {
    if (traits::eq_int_type(sbumpc(), traits::eof()))
      return traits::eof();
    return sgetc();
  }
  int_type sbumpc() {
    if (gnext_ < gend_)
      return traits::to_int_type(*gnext_++);
    return uflow();
  }
  int_type sgetc() {
    if (gnext_ < gend_)
      return traits::to_int_type(*gnext_);
    return underflow();
  }
  streamsize sgetn(char_type* s, streamsize n) { return xsgetn(s, n); }

  // [streambuf.pub.pback]
  int_type sputbackc(char_type c) {
    if (gbeg_ < gnext_ && traits::eq(c, gnext_[-1]))
      return traits::to_int_type(*--gnext_);
    return pbackfail(traits::to_int_type(c));
  }
  int_type sungetc() {
    if (gbeg_ < gnext_)
      return traits::to_int_type(*--gnext_);
    return pbackfail();
  }

  // [streambuf.pub.put]
  int_type sputc(char_type c) {
    if (pnext_ < pend_) {
      *pnext_++ = c;
      return traits::to_int_type(c);
    }
    return overflow(traits::to_int_type(c));
  }
  streamsize sputn(const char_type* s, streamsize n) { return xsputn(s, n); }

protected:
  basic_streambuf() : loc_() {}
  basic_streambuf(const basic_streambuf& rhs) = default;
  basic_streambuf& operator=(const basic_streambuf& rhs) = default;
  void swap(basic_streambuf& rhs) {
    char_type** mine[] = {&gbeg_, &gnext_, &gend_, &pbeg_, &pnext_, &pend_};
    char_type** theirs[] = {&rhs.gbeg_, &rhs.gnext_, &rhs.gend_, &rhs.pbeg_, &rhs.pnext_, &rhs.pend_};
    for (int i = 0; i < 6; ++i) {
      char_type* t = *mine[i];
      *mine[i] = *theirs[i];
      *theirs[i] = t;
    }
    locale l = loc_;
    loc_ = rhs.loc_;
    rhs.loc_ = l;
  }

  // [streambuf.get.area]
  char_type* eback() const { return gbeg_; }
  char_type* gptr() const { return gnext_; }
  char_type* egptr() const { return gend_; }
  void gbump(int n) { gnext_ += n; }
  void setg(char_type* gbeg, char_type* gnext, char_type* gend) {
    gbeg_ = gbeg;
    gnext_ = gnext;
    gend_ = gend;
  }

  // [streambuf.put.area]
  char_type* pbase() const { return pbeg_; }
  char_type* pptr() const { return pnext_; }
  char_type* epptr() const { return pend_; }
  void pbump(int n) { pnext_ += n; }
  void setp(char_type* pbeg, char_type* pend) {
    pbeg_ = pbeg;
    pnext_ = pbeg;
    pend_ = pend;
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
      if (gnext_ < gend_) {
        streamsize k = static_cast<streamsize>(gend_ - gnext_);
        if (k > n - done)
          k = n - done;
        traits::copy(s + done, gnext_, static_cast<size_t>(k));
        gnext_ += k;
        done += k;
      } else {
        const int_type c = uflow();
        if (traits::eq_int_type(c, traits::eof()))
          break;
        s[done++] = traits::to_char_type(c);
      }
    }
    return done;
  }
  virtual int_type underflow() { return traits::eof(); }
  virtual int_type uflow() {
    if (traits::eq_int_type(underflow(), traits::eof()))
      return traits::eof();
    return traits::to_int_type(*gnext_++);
  }

  // [streambuf.virt.pback]
  virtual int_type pbackfail(int_type = traits::eof()) { return traits::eof(); }

  // [streambuf.virt.put]
  virtual streamsize xsputn(const char_type* s, streamsize n) {
    streamsize done = 0;
    while (done < n) {
      if (pnext_ < pend_) {
        streamsize k = static_cast<streamsize>(pend_ - pnext_);
        if (k > n - done)
          k = n - done;
        traits::copy(pnext_, s + done, static_cast<size_t>(k));
        pnext_ += k;
        done += k;
      } else {
        if (traits::eq_int_type(overflow(traits::to_int_type(s[done])), traits::eof()))
          break;
        ++done;
      }
    }
    return done;
  }
  virtual int_type overflow(int_type = traits::eof()) { return traits::eof(); }

private:
  char_type* gbeg_ = nullptr;
  char_type* gnext_ = nullptr;
  char_type* gend_ = nullptr;
  char_type* pbeg_ = nullptr;
  char_type* pnext_ = nullptr;
  char_type* pend_ = nullptr;
  locale loc_;
  bool is_syncbuf_ = false;
  friend ycxx::detail::streambuf_tag_access;
};

} // namespace std
