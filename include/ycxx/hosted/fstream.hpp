// libycxx hosted: basic_filebuf and the file streams ([file.streams]).
//
// A filebuf works on a C stdio FILE (opened with fopen, exactly per the open-mode table of
// [filebuf.members], with stdio's own buffering turned off) through the out-of-line helpers of
// the hosted runtime (src/hosted/fstream.cpp), so this header needs no C header. The filebuf
// keeps its own buffer: a get area while reading, a put area while writing (the joint file
// position of [filebuf.general]/3: switching from writing to reading writes the put area out;
// from reading to writing seeks the file back to the logical position). native_handle_type is
// the POSIX file descriptor (int).
//
// Characters are converted with the codecvt<charT, char, state_type> of the buffer's locale
// unless it always_noconv(). With a conversion, the get area holds the characters converted
// from the bytes last read; the position of a character in it is recovered with
// codecvt::length from the conversion state at the start of those bytes.
#pragma once

#include <ycxx/hosted/istream.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std { namespace filesystem {
class path;
}} // namespace std::filesystem

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// src/hosted/fstream.cpp. A file is an opaque FILE*; offsets are bytes.
void* __file_open(const char* name, std::ios_base::openmode __mode) noexcept; // null on failure
bool __file_close(void* __f) noexcept;
std::size_t __file_read(void* __f, char* __buf, std::size_t n) noexcept;
bool __file_write(void* __f, const char* __buf, std::size_t n) noexcept;
// Seeks (whence: 0 beg, 1 cur, 2 end); returns the new offset, or -1.
long long __file_seek(void* __f, long long __off, int __whence) noexcept;
bool __file_flush(void* __f) noexcept;
int __file_native(void* __f) noexcept;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class __charT, class __traits>
class basic_filebuf : public basic_streambuf<__charT, __traits> {
  using base = basic_streambuf<__charT, __traits>;
  using state_type = typename __traits::state_type;
  using __cvt_type = codecvt<__charT, char, state_type>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using native_handle_type = int;

  // [filebuf.cons]
  basic_filebuf() { __set_codecvt(this->getloc()); }
  basic_filebuf(const basic_filebuf&) = delete;
  basic_filebuf(basic_filebuf&& __rhs) : base(__rhs) { take(__rhs); }
  ~basic_filebuf() override {
    if constexpr (__ycxx::__detail::__cfg::exceptions) {
      try {
        close();
      } catch (...) {
      }
    } else {
      close();
    }
    __release_buffers();
  }

  // [filebuf.assign]
  basic_filebuf& operator=(const basic_filebuf&) = delete;
  basic_filebuf& operator=(basic_filebuf&& __rhs) {
    if (this != __builtin_addressof(__rhs)) {
      close();
      __release_buffers();
      base::operator=(__rhs);
      take(__rhs);
    }
    return *this;
  }
  void swap(basic_filebuf& __rhs) {
    basic_filebuf __tmp(static_cast<basic_filebuf&&>(__rhs));
    __rhs = static_cast<basic_filebuf&&>(*this);
    *this = static_cast<basic_filebuf&&>(__tmp);
  }

  // [filebuf.members]
  bool is_open() const { return __file_ != nullptr; }
  basic_filebuf* open(const char* s, ios_base::openmode __mode) {
    if (__file_ != nullptr)
      return nullptr;
    void* __f = __ycxx::__detail::__file_open(s, __mode);
    if (__f == nullptr)
      return nullptr;
    __file_ = __f;
    __mode_ = __mode;
    __io_ = __io::none;
    __state_ = state_type();
    this->setg(nullptr, nullptr, nullptr);
    this->setp(nullptr, nullptr);
    if (!__allocate_buffers()) {
      __ycxx::__detail::__file_close(__f);
      __file_ = nullptr;
      return nullptr;
    }
    return this;
  }
  basic_filebuf* open(const string& s, ios_base::openmode __mode) { return open(s.c_str(), __mode); }
  // open(const filesystem::path&, openmode): a template, so that <filesystem> need not be
  // included; it takes exactly filesystem::path.
  template <class _Path>
    requires is_same_v<_Path, filesystem::path>
  basic_filebuf* open(const _Path& s, ios_base::openmode __mode) {
    return open(s.c_str(), __mode);
  }
  basic_filebuf* close() {
    if (__file_ == nullptr)
      return nullptr;
    bool ok = true;
    if constexpr (__ycxx::__detail::__cfg::exceptions) {
      try {
        ok = __finish_output();
      } catch (...) {
        __ycxx::__detail::__file_close(__file_);
        __file_ = nullptr;
        __reset_areas();
        throw;
      }
    } else {
      ok = __finish_output();
    }
    if (!__ycxx::__detail::__file_close(__file_))
      ok = false;
    __file_ = nullptr;
    __reset_areas();
    return ok ? this : nullptr;
  }
  native_handle_type native_handle() const noexcept {
    __ycxx::__detail::__precondition(__file_ != nullptr, "std::basic_filebuf::native_handle: the file is not open");
    return __ycxx::__detail::__file_native(__file_);
  }

protected:
  // [filebuf.virtuals]
  int_type underflow() override {
    if (__file_ == nullptr || !(__mode_ & (ios_base::in)))
      return __traits::eof();
    if (this->gptr() < this->egptr())
      return __traits::to_int_type(*this->gptr());
    if (__io_ == __io::__writing && (!__finish_output_mode() || !__ycxx::__detail::__file_flush(__file_)))
      return __traits::eof(); // (C stdio wants a flush between writing and reading)
    return __fill_get_area() ? __traits::to_int_type(*this->gptr()) : __traits::eof();
  }
  int_type uflow() override {
    const int_type c = underflow();
    if (!__traits::eq_int_type(c, __traits::eof()))
      this->gbump(1);
    return c;
  }
  int_type pbackfail(int_type c = __traits::eof()) override {
    if (__file_ == nullptr || !(this->eback() < this->gptr()))
      return __traits::eof();
    if (__traits::eq_int_type(c, __traits::eof())) {
      this->gbump(-1);
      return __traits::not_eof(c);
    }
    this->gbump(-1);
    if (!__traits::eq(__traits::to_char_type(c), *this->gptr()))
      *this->gptr() = __traits::to_char_type(c); // only in the buffer, not in the file
    return c;
  }
  int_type overflow(int_type c = __traits::eof()) override {
    if (__file_ == nullptr || !(__mode_ & (ios_base::out | ios_base::app)))
      return __traits::eof();
    if (__io_ == __io::__reading && !__leave_reading())
      return __traits::eof();
    __io_ = __io::__writing;
    __last_was_overflow_ = true;
    // the put area (if any) goes to the file, then c into the emptied area
    if (this->pbase() != nullptr && !__write_out(this->pbase(), this->pptr()))
      return __traits::eof();
    if (!__unbuffered_)
      this->setp(__ibuf_, __ibuf_ + __ibuf_size_);
    if (!__traits::eq_int_type(c, __traits::eof())) {
      if (this->pbase() != nullptr) {
        *this->pptr() = __traits::to_char_type(c);
        this->pbump(1);
      } else {
        const __charT __ch = __traits::to_char_type(c);
        if (!__write_out(&__ch, &__ch + 1))
          return __traits::eof();
      }
    }
    return __traits::not_eof(c);
  }
  basic_streambuf<__charT, __traits>* setbuf(char_type* s, streamsize n) override {
    if (__io_ != __io::none)
      return this; // only before any I/O
    __release_buffers();
    if (s == nullptr && n == 0) {
      __unbuffered_ = true;
    } else if (s != nullptr && n > 1) {
      // the program's array: its first element is the putback position
      __ibuf_ = s;
      __ibuf_size_ = static_cast<size_t>(n) - 1;
      __user_ibuf_ = true;
    }
    if (__file_ != nullptr)
      __allocate_buffers();
    return this;
  }
  pos_type seekoff(off_type __off, ios_base::seekdir __way,
                   ios_base::openmode = ios_base::in | ios_base::out) override {
    const int width = __cvt_->encoding();
    if (__file_ == nullptr || (__off != 0 && width <= 0))
      return pos_type(off_type(-1));
    const off_type __here = __logical_position();
    if (__here < 0)
      return pos_type(off_type(-1));
    if (__way == ios_base::cur && __off == 0) {
      pos_type r(__here);
      r.state(__state_);
      return r;
    }
    if (!__finish_output_mode())
      return pos_type(off_type(-1));
    long long target;
    int __whence;
    if (__way == ios_base::beg) {
      __whence = 0;
      target = width > 0 ? static_cast<long long>(__off) * width : 0;
    } else if (__way == ios_base::cur) {
      __whence = 0;
      target = __here + (width > 0 ? static_cast<long long>(__off) * width : 0);
    } else if (__way == ios_base::end) {
      __whence = 2;
      target = width > 0 ? static_cast<long long>(__off) * width : 0;
    } else {
      return pos_type(off_type(-1));
    }
    if (__whence == 0 && target < 0)
      return pos_type(off_type(-1));
    const long long r = __ycxx::__detail::__file_seek(__file_, target, __whence);
    __discard_get_area();
    if (r < 0)
      return pos_type(off_type(-1));
    __state_ = state_type();
    return pos_type(off_type(r));
  }
  pos_type seekpos(pos_type __sp, ios_base::openmode = ios_base::in | ios_base::out) override {
    if (__file_ == nullptr || off_type(__sp) < 0)
      return pos_type(off_type(-1));
    if (!__finish_output_mode())
      return pos_type(off_type(-1));
    const long long r = __ycxx::__detail::__file_seek(__file_, static_cast<long long>(off_type(__sp)), 0);
    __discard_get_area();
    if (r < 0)
      return pos_type(off_type(-1));
    __state_ = __sp.state();
    return __sp;
  }
  int sync() override {
    if (__file_ == nullptr)
      return 0;
    if (__io_ == __io::__writing) {
      if (!__write_out(this->pbase(), this->pptr()))
        return -1;
      if (this->pbase() != nullptr)
        this->setp(this->pbase(), this->epptr());
      return __ycxx::__detail::__file_flush(__file_) ? 0 : -1;
    }
    if (__io_ == __io::__reading && __always_noconv_) {
      // the file position follows the characters read (implementation-defined, [filebuf.virtuals]/19)
      return __leave_reading() ? 0 : -1;
    }
    return 0;
  }
  void imbue(const locale& __loc) override { __set_codecvt(__loc); }

private:
  enum class __io : unsigned char { none, __reading, __writing };

  // ---- buffers ----
  static constexpr size_t __default_size = 4096;
  bool __allocate_buffers() {
    if (__unbuffered_) {
      __ibuf_size_ = 1;
    } else if (__ibuf_ == nullptr) {
      __ibuf_size_ = __default_size / sizeof(__charT) > 64 ? __default_size / sizeof(__charT) : 64;
    }
    if (__ibuf_ == nullptr) {
      __ibuf_ = new (nothrow) __charT[__ibuf_size_ + 1]; // + 1: the putback position
      if (__ibuf_ == nullptr)
        return false;
      __user_ibuf_ = false;
    }
    if (!__always_noconv_ && __xbuf_ == nullptr) {
      __xbuf_size_ = __default_size > __ibuf_size_ * 8 ? __default_size : __ibuf_size_ * 8;
      __xbuf_ = new (nothrow) char[__xbuf_size_];
      if (__xbuf_ == nullptr)
        return false;
    }
    return true;
  }
  void __release_buffers() noexcept {
    if (!__user_ibuf_)
      delete[] __ibuf_;
    __ibuf_ = nullptr;
    __ibuf_size_ = 0;
    __user_ibuf_ = false;
    delete[] __xbuf_;
    __xbuf_ = nullptr;
    __xbuf_size_ = 0;
    this->setg(nullptr, nullptr, nullptr);
    this->setp(nullptr, nullptr);
  }
  void __reset_areas() noexcept {
    this->setg(nullptr, nullptr, nullptr);
    this->setp(nullptr, nullptr);
    __io_ = __io::none;
    __xnext_ = __xend_ = 0;
  }
  void take(basic_filebuf& __rhs) {
    __file_ = __rhs.__file_;
    __mode_ = __rhs.__mode_;
    __io_ = __rhs.__io_;
    __state_ = __rhs.__state_;
    __state_start_ = __rhs.__state_start_;
    __cvt_ = __rhs.__cvt_;
    __always_noconv_ = __rhs.__always_noconv_;
    __unbuffered_ = __rhs.__unbuffered_;
    __last_was_overflow_ = __rhs.__last_was_overflow_;
    __xnext_ = __rhs.__xnext_;
    __xend_ = __rhs.__xend_;
    // this filebuf gets its own buffers, holding rhs's pending characters
    __ibuf_ = __rhs.__ibuf_;
    __ibuf_size_ = __rhs.__ibuf_size_;
    __user_ibuf_ = __rhs.__user_ibuf_;
    __xbuf_ = __rhs.__xbuf_;
    __xbuf_size_ = __rhs.__xbuf_size_;
    if (__user_ibuf_) {
      // a buffer supplied with setbuf stays with rhs: copy the pending characters
      __charT* __own = new __charT[__ibuf_size_ + 1];
      const size_t n = __ibuf_size_ + 1;
      __traits::copy(__own, __rhs.__ibuf_, n);
      __relocate(__rhs.__ibuf_, __own);
      __ibuf_ = __own;
      __user_ibuf_ = false;
    }
    __rhs.__file_ = nullptr;
    __rhs.__ibuf_ = nullptr;
    __rhs.__ibuf_size_ = 0;
    __rhs.__user_ibuf_ = false;
    __rhs.__xbuf_ = nullptr;
    __rhs.__xbuf_size_ = 0;
    __rhs.__reset_areas();
  }
  // the areas point into old (a buffer whose contents were copied to now)
  void __relocate(__charT* __old, __charT* now) noexcept {
    if (this->eback() != nullptr)
      this->setg(now + (this->eback() - __old), now + (this->gptr() - __old), now + (this->egptr() - __old));
    if (this->pbase() != nullptr) {
      const auto __y_used = this->pptr() - this->pbase();
      this->setp(now + (this->pbase() - __old), now + (this->epptr() - __old));
      this->pbump(static_cast<int>(__y_used));
    }
  }
  void __set_codecvt(const locale& __loc) {
    __cvt_ = &use_facet<__cvt_type>(__loc);
    const bool noconv = __cvt_->always_noconv();
    if (noconv != __always_noconv_) {
      __always_noconv_ = noconv;
      if (!noconv && __file_ != nullptr && __xbuf_ == nullptr)
        __allocate_buffers();
    }
  }

  // ---- output ----
  // Converts and writes [b, e).
  bool __write_out(const __charT* b, const __charT* e) {
    if (b == e)
      return true;
    if (__always_noconv_) {
      if constexpr (is_same_v<__charT, char>)
        return __ycxx::__detail::__file_write(__file_, b, static_cast<size_t>(e - b));
      else
        return __write_converted(b, e);
    }
    return __write_converted(b, e);
  }
  bool __write_converted(const __charT* b, const __charT* e) {
    while (b != e) {
      const __charT* next = b;
      char* to = __xbuf_;
      const codecvt_base::result r = __cvt_->out(__state_, b, e, next, __xbuf_, __xbuf_ + __xbuf_size_, to);
      if (r == codecvt_base::error)
        return false;
      if (r == codecvt_base::noconv) {
        // the characters themselves are the bytes (internT and externT the same type)
        return __ycxx::__detail::__file_write(__file_, reinterpret_cast<const char*>(b),
                                        static_cast<size_t>(e - b) * sizeof(__charT));
      }
      if (!__ycxx::__detail::__file_write(__file_, __xbuf_, static_cast<size_t>(to - __xbuf_)))
        return false;
      if (next == b && to == __xbuf_)
        return false; // no progress
      b = next;
    }
    return true;
  }
  // [filebuf.members]/8: the put area and the unshift sequence of a close
  bool __finish_output() {
    bool ok = true;
    if (__io_ == __io::__writing) {
      if (__traits::eq_int_type(overflow(__traits::eof()), __traits::eof()))
        ok = false;
      if (__last_was_overflow_ && !__always_noconv_ && !unshift())
        ok = false;
    }
    return ok;
  }
  bool unshift() {
    for (;;) {
      char* to = __xbuf_;
      const codecvt_base::result r = __cvt_->unshift(__state_, __xbuf_, __xbuf_ + __xbuf_size_, to);
      if (r == codecvt_base::error)
        return false;
      if (r == codecvt_base::noconv)
        return true;
      if (!__ycxx::__detail::__file_write(__file_, __xbuf_, static_cast<size_t>(to - __xbuf_)))
        return false;
      if (r == codecvt_base::ok)
        return true;
    }
  }
  // Leaves writing mode: the put area and unshift sequence go to the file.
  bool __finish_output_mode() {
    if (__io_ != __io::__writing)
      return true;
    const bool ok = __finish_output();
    this->setp(nullptr, nullptr);
    __io_ = __io::none;
    __last_was_overflow_ = false;
    return ok;
  }

  // ---- input ----
  // The get area: ibuf_[0] keeps the last character of the previous area (the putback
  // position); the characters read start at ibuf_ + 1.
  // Reads and converts the next characters; false at end of file (or on a conversion error).
  bool __fill_get_area() {
    __io_ = __io::__reading;
    __last_was_overflow_ = false;
    __charT* const start = __ibuf_ + 1;
    const bool __keep = this->gptr() != nullptr && this->eback() < this->gptr();
    if (__keep)
      __ibuf_[0] = this->gptr()[-1];
    __charT* const first = __keep ? __ibuf_ : start;
    const size_t __room = __ibuf_size_;
    if constexpr (is_same_v<__charT, char>) {
      if (__always_noconv_) {
        const size_t n = __ycxx::__detail::__file_read(__file_, start, __room);
        this->setg(first, start, start + n);
        __xnext_ = __xend_ = 0;
        return n != 0;
      }
    }
    for (;;) {
      // the bytes not yet converted move to the front; more are read after them
      if (__xnext_ != 0) {
        __builtin_memmove(__xbuf_, __xbuf_ + __xnext_, __xend_ - __xnext_);
        __xend_ -= __xnext_;
        __xnext_ = 0;
      }
      bool __at_eof = false;
      if (__xend_ < __xbuf_size_) {
        const size_t __got = __ycxx::__detail::__file_read(__file_, __xbuf_ + __xend_, __xbuf_size_ - __xend_);
        __at_eof = __got == 0;
        __xend_ += __got;
      }
      if (__xend_ == 0) {
        this->setg(first, start, start);
        return false;
      }
      __state_start_ = __state_;
      const char* __from_next = __xbuf_;
      __charT* __to_next = start;
      codecvt_base::result r = codecvt_base::noconv;
      if (!__always_noconv_)
        r = __cvt_->in(__state_, __xbuf_, __xbuf_ + __xend_, __from_next, start, start + __room, __to_next);
      if (r == codecvt_base::noconv) {
        const size_t __chars = __xend_ / sizeof(__charT) < __room ? __xend_ / sizeof(__charT) : __room;
        __builtin_memcpy(static_cast<void*>(start), __xbuf_, __chars * sizeof(__charT));
        __from_next = __xbuf_ + __chars * sizeof(__charT);
        __to_next = start + __chars;
      }
      __xnext_ = static_cast<size_t>(__from_next - __xbuf_);
      if (__to_next != start) {
        this->setg(first, start, __to_next);
        return true;
      }
      if (r == codecvt_base::error || __at_eof || (__xnext_ == 0 && __xend_ == __xbuf_size_)) {
        this->setg(first, start, start);
        return false;
      }
      // partial: more bytes are needed for one character
    }
  }
  // The file offset of gptr() while reading, of the next character written while writing.
  off_type __logical_position() {
    const long long now = __ycxx::__detail::__file_seek(__file_, 0, 1);
    if (now < 0)
      return -1;
    if (__io_ == __io::__writing) {
      if constexpr (is_same_v<__charT, char>) {
        if (__always_noconv_)
          return static_cast<off_type>(now + (this->pptr() - this->pbase()));
      }
      // converted output: written out first, to know its length
      if (!__write_out(this->pbase(), this->pptr()))
        return -1;
      if (this->pbase() != nullptr)
        this->setp(this->pbase(), this->epptr());
      return static_cast<off_type>(__ycxx::__detail::__file_seek(__file_, 0, 1));
    }
    if (__io_ != __io::__reading || this->gptr() == nullptr)
      return static_cast<off_type>(now);
    if constexpr (is_same_v<__charT, char>) {
      if (__always_noconv_)
        return static_cast<off_type>(now - (this->egptr() - this->gptr()));
    }
    // the bytes [0, xend_) of xbuf_ were read last; the characters from ibuf_ + 1 on were
    // converted from them, starting in state_start_
    const __charT* const start = __ibuf_ + 1;
    const long long __base_pos = now - static_cast<long long>(__xend_);
    if (this->gptr() < start) { // in the putback position
      const int width = __always_noconv_ ? static_cast<int>(sizeof(__charT)) : __cvt_->encoding();
      return width > 0 ? static_cast<off_type>(__base_pos - width) : off_type(-1);
    }
    const size_t done = static_cast<size_t>(this->gptr() - start);
    long long __consumed;
    if (__always_noconv_) {
      __consumed = static_cast<long long>(done * sizeof(__charT));
    } else {
      state_type __st = __state_start_;
      __consumed = __cvt_->length(__st, __xbuf_, __xbuf_ + __xend_, done);
    }
    return static_cast<off_type>(__base_pos + __consumed);
  }
  // Leaves reading mode: positions the file at gptr() and drops the get area.
  bool __leave_reading() {
    const off_type __pos = __logical_position();
    __discard_get_area();
    if (__pos < 0)
      return false;
    return __ycxx::__detail::__file_seek(__file_, static_cast<long long>(__pos), 0) >= 0;
  }
  void __discard_get_area() noexcept {
    this->setg(nullptr, nullptr, nullptr);
    __xnext_ = __xend_ = 0;
    if (__io_ == __io::__reading)
      __io_ = __io::none;
  }

  void* __file_ = nullptr;
  ios_base::openmode __mode_ = ios_base::openmode{};
  __io __io_ = __io::none;
  state_type __state_{};
  state_type __state_start_{}; // the conversion state at the start of the bytes in xbuf_
  const __cvt_type* __cvt_ = nullptr;
  bool __always_noconv_ = true;
  bool __unbuffered_ = false;
  bool __last_was_overflow_ = false;
  bool __user_ibuf_ = false;
  __charT* __ibuf_ = nullptr; // ibuf_size_ + 1 characters (the first: a putback position)
  size_t __ibuf_size_ = 0;
  char* __xbuf_ = nullptr; // bytes read but not yet converted, or converted output
  size_t __xbuf_size_ = 0;
  size_t __xnext_ = 0, __xend_ = 0; // xbuf_: [0, xnext_) converted, [xnext_, xend_) pending
};

template <class __charT, class __traits>
void swap(basic_filebuf<__charT, __traits>& __x, basic_filebuf<__charT, __traits>& y) {
  __x.swap(y);
}

// [ifstream]
template <class __charT, class __traits>
class basic_ifstream : public basic_istream<__charT, __traits> {
  using __stream_base = basic_istream<__charT, __traits>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using native_handle_type = typename basic_filebuf<__charT, __traits>::native_handle_type;

  // [ifstream.cons]
  basic_ifstream() : __stream_base(__builtin_addressof(__sb_)) {}
  explicit basic_ifstream(const char* s, ios_base::openmode __mode = ios_base::in) : __stream_base(__builtin_addressof(__sb_)) {
    if (__sb_.open(s, __mode | ios_base::in) == nullptr)
      this->setstate(ios_base::failbit);
  }
  explicit basic_ifstream(const string& s, ios_base::openmode __mode = ios_base::in) : basic_ifstream(s.c_str(), __mode) {}
  // the constructor and open taking filesystem::path: templates, so that <filesystem> need not
  // be included ([ifstream.cons]: is_same_v<T, filesystem::path>)
  template <class _Tp>
    requires is_same_v<_Tp, filesystem::path>
  explicit basic_ifstream(const _Tp& s, ios_base::openmode __mode = ios_base::in) : basic_ifstream(s.c_str(), __mode) {}
  basic_ifstream(const basic_ifstream&) = delete;
  basic_ifstream(basic_ifstream&& __rhs)
      : __stream_base(static_cast<__stream_base&&>(__rhs)), __sb_(static_cast<basic_filebuf<__charT, __traits>&&>(__rhs.__sb_)) {
    __stream_base::set_rdbuf(__builtin_addressof(__sb_));
  }
  basic_ifstream& operator=(const basic_ifstream&) = delete;
  basic_ifstream& operator=(basic_ifstream&& __rhs) {
    __stream_base::operator=(static_cast<__stream_base&&>(__rhs));
    __sb_ = static_cast<basic_filebuf<__charT, __traits>&&>(__rhs.__sb_);
    return *this;
  }

  // [ifstream.swap]
  void swap(basic_ifstream& __rhs) {
    __stream_base::swap(__rhs);
    __sb_.swap(__rhs.__sb_);
  }

  // [ifstream.members]
  basic_filebuf<__charT, __traits>* rdbuf() const {
    return const_cast<basic_filebuf<__charT, __traits>*>(__builtin_addressof(__sb_));
  }
  native_handle_type native_handle() const noexcept { return rdbuf()->native_handle(); }
  bool is_open() const { return rdbuf()->is_open(); }
  void open(const char* s, ios_base::openmode __mode = ios_base::in) {
    if (rdbuf()->open(s, __mode | ios_base::in) != nullptr)
      this->clear();
    else
      this->setstate(ios_base::failbit);
  }
  void open(const string& s, ios_base::openmode __mode = ios_base::in) { open(s.c_str(), __mode); }
  template <class _Tp>
    requires is_same_v<_Tp, filesystem::path>
  void open(const _Tp& s, ios_base::openmode __mode = ios_base::in) {
    open(s.c_str(), __mode);
  }
  void close() {
    if (rdbuf()->close() == nullptr)
      this->setstate(ios_base::failbit);
  }

private:
  basic_filebuf<__charT, __traits> __sb_;
};

template <class __charT, class __traits>
void swap(basic_ifstream<__charT, __traits>& __x, basic_ifstream<__charT, __traits>& y) {
  __x.swap(y);
}

// [ofstream]
template <class __charT, class __traits>
class basic_ofstream : public basic_ostream<__charT, __traits> {
  using __stream_base = basic_ostream<__charT, __traits>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using native_handle_type = typename basic_filebuf<__charT, __traits>::native_handle_type;

  // [ofstream.cons]
  basic_ofstream() : __stream_base(__builtin_addressof(__sb_)) {}
  explicit basic_ofstream(const char* s, ios_base::openmode __mode = ios_base::out) : __stream_base(__builtin_addressof(__sb_)) {
    if (__sb_.open(s, __mode | ios_base::out) == nullptr)
      this->setstate(ios_base::failbit);
  }
  explicit basic_ofstream(const string& s, ios_base::openmode __mode = ios_base::out) : basic_ofstream(s.c_str(), __mode) {}
  // the constructor and open taking filesystem::path: templates, so that <filesystem> need not
  // be included ([ofstream.cons]: is_same_v<T, filesystem::path>)
  template <class _Tp>
    requires is_same_v<_Tp, filesystem::path>
  explicit basic_ofstream(const _Tp& s, ios_base::openmode __mode = ios_base::out) : basic_ofstream(s.c_str(), __mode) {}
  basic_ofstream(const basic_ofstream&) = delete;
  basic_ofstream(basic_ofstream&& __rhs)
      : __stream_base(static_cast<__stream_base&&>(__rhs)), __sb_(static_cast<basic_filebuf<__charT, __traits>&&>(__rhs.__sb_)) {
    __stream_base::set_rdbuf(__builtin_addressof(__sb_));
  }
  basic_ofstream& operator=(const basic_ofstream&) = delete;
  basic_ofstream& operator=(basic_ofstream&& __rhs) {
    __stream_base::operator=(static_cast<__stream_base&&>(__rhs));
    __sb_ = static_cast<basic_filebuf<__charT, __traits>&&>(__rhs.__sb_);
    return *this;
  }

  // [ofstream.swap]
  void swap(basic_ofstream& __rhs) {
    __stream_base::swap(__rhs);
    __sb_.swap(__rhs.__sb_);
  }

  // [ofstream.members]
  basic_filebuf<__charT, __traits>* rdbuf() const {
    return const_cast<basic_filebuf<__charT, __traits>*>(__builtin_addressof(__sb_));
  }
  native_handle_type native_handle() const noexcept { return rdbuf()->native_handle(); }
  bool is_open() const { return rdbuf()->is_open(); }
  void open(const char* s, ios_base::openmode __mode = ios_base::out) {
    if (rdbuf()->open(s, __mode | ios_base::out) != nullptr)
      this->clear();
    else
      this->setstate(ios_base::failbit);
  }
  void open(const string& s, ios_base::openmode __mode = ios_base::out) { open(s.c_str(), __mode); }
  template <class _Tp>
    requires is_same_v<_Tp, filesystem::path>
  void open(const _Tp& s, ios_base::openmode __mode = ios_base::out) {
    open(s.c_str(), __mode);
  }
  void close() {
    if (rdbuf()->close() == nullptr)
      this->setstate(ios_base::failbit);
  }

private:
  basic_filebuf<__charT, __traits> __sb_;
};

template <class __charT, class __traits>
void swap(basic_ofstream<__charT, __traits>& __x, basic_ofstream<__charT, __traits>& y) {
  __x.swap(y);
}

// [fstream]
template <class __charT, class __traits>
class basic_fstream : public basic_iostream<__charT, __traits> {
  using __stream_base = basic_iostream<__charT, __traits>;

public:
  using char_type = __charT;
  using int_type = typename __traits::int_type;
  using pos_type = typename __traits::pos_type;
  using off_type = typename __traits::off_type;
  using traits_type = __traits;
  using native_handle_type = typename basic_filebuf<__charT, __traits>::native_handle_type;

  // [fstream.cons]
  basic_fstream() : __stream_base(__builtin_addressof(__sb_)) {}
  explicit basic_fstream(const char* s, ios_base::openmode __mode = ios_base::in | ios_base::out) : __stream_base(__builtin_addressof(__sb_)) {
    if (__sb_.open(s, __mode) == nullptr)
      this->setstate(ios_base::failbit);
  }
  explicit basic_fstream(const string& s, ios_base::openmode __mode = ios_base::in | ios_base::out) : basic_fstream(s.c_str(), __mode) {}
  // the constructor and open taking filesystem::path: templates, so that <filesystem> need not
  // be included ([fstream.cons]: is_same_v<T, filesystem::path>)
  template <class _Tp>
    requires is_same_v<_Tp, filesystem::path>
  explicit basic_fstream(const _Tp& s, ios_base::openmode __mode = ios_base::in | ios_base::out) : basic_fstream(s.c_str(), __mode) {}
  basic_fstream(const basic_fstream&) = delete;
  basic_fstream(basic_fstream&& __rhs)
      : __stream_base(static_cast<__stream_base&&>(__rhs)), __sb_(static_cast<basic_filebuf<__charT, __traits>&&>(__rhs.__sb_)) {
    __stream_base::set_rdbuf(__builtin_addressof(__sb_));
  }
  basic_fstream& operator=(const basic_fstream&) = delete;
  basic_fstream& operator=(basic_fstream&& __rhs) {
    __stream_base::operator=(static_cast<__stream_base&&>(__rhs));
    __sb_ = static_cast<basic_filebuf<__charT, __traits>&&>(__rhs.__sb_);
    return *this;
  }

  // [fstream.swap]
  void swap(basic_fstream& __rhs) {
    __stream_base::swap(__rhs);
    __sb_.swap(__rhs.__sb_);
  }

  // [fstream.members]
  basic_filebuf<__charT, __traits>* rdbuf() const {
    return const_cast<basic_filebuf<__charT, __traits>*>(__builtin_addressof(__sb_));
  }
  native_handle_type native_handle() const noexcept { return rdbuf()->native_handle(); }
  bool is_open() const { return rdbuf()->is_open(); }
  void open(const char* s, ios_base::openmode __mode = ios_base::in | ios_base::out) {
    if (rdbuf()->open(s, __mode) != nullptr)
      this->clear();
    else
      this->setstate(ios_base::failbit);
  }
  void open(const string& s, ios_base::openmode __mode = ios_base::in | ios_base::out) { open(s.c_str(), __mode); }
  template <class _Tp>
    requires is_same_v<_Tp, filesystem::path>
  void open(const _Tp& s, ios_base::openmode __mode = ios_base::in | ios_base::out) {
    open(s.c_str(), __mode);
  }
  void close() {
    if (rdbuf()->close() == nullptr)
      this->setstate(ios_base::failbit);
  }

private:
  basic_filebuf<__charT, __traits> __sb_;
};

template <class __charT, class __traits>
void swap(basic_fstream<__charT, __traits>& __x, basic_fstream<__charT, __traits>& y) {
  __x.swap(y);
}

} // namespace std
