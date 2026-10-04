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

namespace std::filesystem {
class path;
} // namespace std::filesystem

namespace ycxx::detail {
// src/hosted/fstream.cpp. A file is an opaque FILE*; offsets are bytes.
void* file_open(const char* name, std::ios_base::openmode mode) noexcept; // null on failure
bool file_close(void* f) noexcept;
std::size_t file_read(void* f, char* buf, std::size_t n) noexcept;
bool file_write(void* f, const char* buf, std::size_t n) noexcept;
// Seeks (whence: 0 beg, 1 cur, 2 end); returns the new offset, or -1.
long long file_seek(void* f, long long off, int whence) noexcept;
bool file_flush(void* f) noexcept;
int file_native(void* f) noexcept;
} // namespace ycxx::detail

namespace std {

template <class charT, class traits>
class basic_filebuf : public basic_streambuf<charT, traits> {
  using base = basic_streambuf<charT, traits>;
  using state_type = typename traits::state_type;
  using cvt_type = codecvt<charT, char, state_type>;

public:
  using char_type = charT;
  using int_type = typename traits::int_type;
  using pos_type = typename traits::pos_type;
  using off_type = typename traits::off_type;
  using traits_type = traits;
  using native_handle_type = int;

  // [filebuf.cons]
  basic_filebuf() { set_codecvt(this->getloc()); }
  basic_filebuf(const basic_filebuf&) = delete;
  basic_filebuf(basic_filebuf&& rhs) : base(rhs) { take(rhs); }
  ~basic_filebuf() override {
    if constexpr (ycxx::detail::cfg::exceptions) {
      try {
        close();
      } catch (...) {
      }
    } else {
      close();
    }
    release_buffers();
  }

  // [filebuf.assign]
  basic_filebuf& operator=(const basic_filebuf&) = delete;
  basic_filebuf& operator=(basic_filebuf&& rhs) {
    if (this != __builtin_addressof(rhs)) {
      close();
      release_buffers();
      base::operator=(rhs);
      take(rhs);
    }
    return *this;
  }
  void swap(basic_filebuf& rhs) {
    basic_filebuf tmp(static_cast<basic_filebuf&&>(rhs));
    rhs = static_cast<basic_filebuf&&>(*this);
    *this = static_cast<basic_filebuf&&>(tmp);
  }

  // [filebuf.members]
  bool is_open() const { return file_ != nullptr; }
  basic_filebuf* open(const char* s, ios_base::openmode mode) {
    if (file_ != nullptr)
      return nullptr;
    void* f = ycxx::detail::file_open(s, mode);
    if (f == nullptr)
      return nullptr;
    file_ = f;
    mode_ = mode;
    io_ = io::none;
    state_ = state_type();
    this->setg(nullptr, nullptr, nullptr);
    this->setp(nullptr, nullptr);
    if (!allocate_buffers()) {
      ycxx::detail::file_close(f);
      file_ = nullptr;
      return nullptr;
    }
    return this;
  }
  basic_filebuf* open(const string& s, ios_base::openmode mode) { return open(s.c_str(), mode); }
  // open(const filesystem::path&, openmode): a template, so that <filesystem> need not be
  // included; it takes exactly filesystem::path.
  template <class Path>
    requires is_same_v<Path, filesystem::path>
  basic_filebuf* open(const Path& s, ios_base::openmode mode) {
    return open(s.c_str(), mode);
  }
  basic_filebuf* close() {
    if (file_ == nullptr)
      return nullptr;
    bool ok = true;
    if constexpr (ycxx::detail::cfg::exceptions) {
      try {
        ok = finish_output();
      } catch (...) {
        ycxx::detail::file_close(file_);
        file_ = nullptr;
        reset_areas();
        throw;
      }
    } else {
      ok = finish_output();
    }
    if (!ycxx::detail::file_close(file_))
      ok = false;
    file_ = nullptr;
    reset_areas();
    return ok ? this : nullptr;
  }
  native_handle_type native_handle() const noexcept {
    ycxx::detail::precondition(file_ != nullptr, "std::basic_filebuf::native_handle: the file is not open");
    return ycxx::detail::file_native(file_);
  }

protected:
  // [filebuf.virtuals]
  int_type underflow() override {
    if (file_ == nullptr || !(mode_ & (ios_base::in)))
      return traits::eof();
    if (this->gptr() < this->egptr())
      return traits::to_int_type(*this->gptr());
    if (io_ == io::writing && !finish_output_mode())
      return traits::eof();
    return fill_get_area() ? traits::to_int_type(*this->gptr()) : traits::eof();
  }
  int_type uflow() override {
    const int_type c = underflow();
    if (!traits::eq_int_type(c, traits::eof()))
      this->gbump(1);
    return c;
  }
  int_type pbackfail(int_type c = traits::eof()) override {
    if (file_ == nullptr || !(this->eback() < this->gptr()))
      return traits::eof();
    if (traits::eq_int_type(c, traits::eof())) {
      this->gbump(-1);
      return traits::not_eof(c);
    }
    this->gbump(-1);
    if (!traits::eq(traits::to_char_type(c), *this->gptr()))
      *this->gptr() = traits::to_char_type(c); // only in the buffer, not in the file
    return c;
  }
  int_type overflow(int_type c = traits::eof()) override {
    if (file_ == nullptr || !(mode_ & (ios_base::out | ios_base::app)))
      return traits::eof();
    if (io_ == io::reading && !leave_reading())
      return traits::eof();
    io_ = io::writing;
    last_was_overflow_ = true;
    // the put area (if any) goes to the file, then c into the emptied area
    if (this->pbase() != nullptr && !write_out(this->pbase(), this->pptr()))
      return traits::eof();
    if (!unbuffered_)
      this->setp(ibuf_, ibuf_ + ibuf_size_);
    if (!traits::eq_int_type(c, traits::eof())) {
      if (this->pbase() != nullptr) {
        *this->pptr() = traits::to_char_type(c);
        this->pbump(1);
      } else {
        const charT ch = traits::to_char_type(c);
        if (!write_out(&ch, &ch + 1))
          return traits::eof();
      }
    }
    return traits::not_eof(c);
  }
  basic_streambuf<charT, traits>* setbuf(char_type* s, streamsize n) override {
    if (io_ != io::none)
      return this; // only before any I/O
    release_buffers();
    if (s == nullptr && n == 0) {
      unbuffered_ = true;
    } else if (s != nullptr && n > 1) {
      // the program's array: its first element is the putback position
      ibuf_ = s;
      ibuf_size_ = static_cast<size_t>(n) - 1;
      user_ibuf_ = true;
    }
    if (file_ != nullptr)
      allocate_buffers();
    return this;
  }
  pos_type seekoff(off_type off, ios_base::seekdir way,
                   ios_base::openmode = ios_base::in | ios_base::out) override {
    const int width = cvt_->encoding();
    if (file_ == nullptr || (off != 0 && width <= 0))
      return pos_type(off_type(-1));
    const off_type here = logical_position();
    if (here < 0)
      return pos_type(off_type(-1));
    if (way == ios_base::cur && off == 0) {
      pos_type r(here);
      r.state(state_);
      return r;
    }
    if (!finish_output_mode())
      return pos_type(off_type(-1));
    long long target;
    int whence;
    if (way == ios_base::beg) {
      whence = 0;
      target = width > 0 ? static_cast<long long>(off) * width : 0;
    } else if (way == ios_base::cur) {
      whence = 0;
      target = here + (width > 0 ? static_cast<long long>(off) * width : 0);
    } else if (way == ios_base::end) {
      whence = 2;
      target = width > 0 ? static_cast<long long>(off) * width : 0;
    } else {
      return pos_type(off_type(-1));
    }
    if (whence == 0 && target < 0)
      return pos_type(off_type(-1));
    const long long r = ycxx::detail::file_seek(file_, target, whence);
    discard_get_area();
    if (r < 0)
      return pos_type(off_type(-1));
    state_ = state_type();
    return pos_type(off_type(r));
  }
  pos_type seekpos(pos_type sp, ios_base::openmode = ios_base::in | ios_base::out) override {
    if (file_ == nullptr || off_type(sp) < 0)
      return pos_type(off_type(-1));
    if (!finish_output_mode())
      return pos_type(off_type(-1));
    const long long r = ycxx::detail::file_seek(file_, static_cast<long long>(off_type(sp)), 0);
    discard_get_area();
    if (r < 0)
      return pos_type(off_type(-1));
    state_ = sp.state();
    return sp;
  }
  int sync() override {
    if (file_ == nullptr)
      return 0;
    if (io_ == io::writing) {
      if (!write_out(this->pbase(), this->pptr()))
        return -1;
      if (this->pbase() != nullptr)
        this->setp(this->pbase(), this->epptr());
      return ycxx::detail::file_flush(file_) ? 0 : -1;
    }
    if (io_ == io::reading && always_noconv_) {
      // the file position follows the characters read (implementation-defined, [filebuf.virtuals]/19)
      return leave_reading() ? 0 : -1;
    }
    return 0;
  }
  void imbue(const locale& loc) override { set_codecvt(loc); }

private:
  enum class io : unsigned char { none, reading, writing };

  // ---- buffers ----
  static constexpr size_t default_size = 4096;
  bool allocate_buffers() {
    if (unbuffered_) {
      ibuf_size_ = 1;
    } else if (ibuf_ == nullptr) {
      ibuf_size_ = default_size / sizeof(charT) > 64 ? default_size / sizeof(charT) : 64;
    }
    if (ibuf_ == nullptr) {
      ibuf_ = new (nothrow) charT[ibuf_size_ + 1]; // + 1: the putback position
      if (ibuf_ == nullptr)
        return false;
      user_ibuf_ = false;
    }
    if (!always_noconv_ && xbuf_ == nullptr) {
      xbuf_size_ = default_size > ibuf_size_ * 8 ? default_size : ibuf_size_ * 8;
      xbuf_ = new (nothrow) char[xbuf_size_];
      if (xbuf_ == nullptr)
        return false;
    }
    return true;
  }
  void release_buffers() noexcept {
    if (!user_ibuf_)
      delete[] ibuf_;
    ibuf_ = nullptr;
    ibuf_size_ = 0;
    user_ibuf_ = false;
    delete[] xbuf_;
    xbuf_ = nullptr;
    xbuf_size_ = 0;
    this->setg(nullptr, nullptr, nullptr);
    this->setp(nullptr, nullptr);
  }
  void reset_areas() noexcept {
    this->setg(nullptr, nullptr, nullptr);
    this->setp(nullptr, nullptr);
    io_ = io::none;
    xnext_ = xend_ = 0;
  }
  void take(basic_filebuf& rhs) {
    file_ = rhs.file_;
    mode_ = rhs.mode_;
    io_ = rhs.io_;
    state_ = rhs.state_;
    state_start_ = rhs.state_start_;
    cvt_ = rhs.cvt_;
    always_noconv_ = rhs.always_noconv_;
    unbuffered_ = rhs.unbuffered_;
    last_was_overflow_ = rhs.last_was_overflow_;
    xnext_ = rhs.xnext_;
    xend_ = rhs.xend_;
    // this filebuf gets its own buffers, holding rhs's pending characters
    ibuf_ = rhs.ibuf_;
    ibuf_size_ = rhs.ibuf_size_;
    user_ibuf_ = rhs.user_ibuf_;
    xbuf_ = rhs.xbuf_;
    xbuf_size_ = rhs.xbuf_size_;
    if (user_ibuf_) {
      // a buffer supplied with setbuf stays with rhs: copy the pending characters
      charT* own = new charT[ibuf_size_ + 1];
      const size_t n = ibuf_size_ + 1;
      traits::copy(own, rhs.ibuf_, n);
      relocate(rhs.ibuf_, own);
      ibuf_ = own;
      user_ibuf_ = false;
    }
    rhs.file_ = nullptr;
    rhs.ibuf_ = nullptr;
    rhs.ibuf_size_ = 0;
    rhs.user_ibuf_ = false;
    rhs.xbuf_ = nullptr;
    rhs.xbuf_size_ = 0;
    rhs.reset_areas();
  }
  // the areas point into old (a buffer whose contents were copied to now)
  void relocate(charT* old, charT* now) noexcept {
    if (this->eback() != nullptr)
      this->setg(now + (this->eback() - old), now + (this->gptr() - old), now + (this->egptr() - old));
    if (this->pbase() != nullptr) {
      const auto used = this->pptr() - this->pbase();
      this->setp(now + (this->pbase() - old), now + (this->epptr() - old));
      this->pbump(static_cast<int>(used));
    }
  }
  void set_codecvt(const locale& loc) {
    cvt_ = &use_facet<cvt_type>(loc);
    const bool noconv = cvt_->always_noconv();
    if (noconv != always_noconv_) {
      always_noconv_ = noconv;
      if (!noconv && file_ != nullptr && xbuf_ == nullptr)
        allocate_buffers();
    }
  }

  // ---- output ----
  // Converts and writes [b, e).
  bool write_out(const charT* b, const charT* e) {
    if (b == e)
      return true;
    if (always_noconv_) {
      if constexpr (is_same_v<charT, char>)
        return ycxx::detail::file_write(file_, b, static_cast<size_t>(e - b));
      else
        return write_converted(b, e);
    }
    return write_converted(b, e);
  }
  bool write_converted(const charT* b, const charT* e) {
    while (b != e) {
      const charT* next = b;
      char* to = xbuf_;
      const codecvt_base::result r = cvt_->out(state_, b, e, next, xbuf_, xbuf_ + xbuf_size_, to);
      if (r == codecvt_base::error)
        return false;
      if (r == codecvt_base::noconv) {
        // the characters themselves are the bytes (internT and externT the same type)
        return ycxx::detail::file_write(file_, reinterpret_cast<const char*>(b),
                                        static_cast<size_t>(e - b) * sizeof(charT));
      }
      if (!ycxx::detail::file_write(file_, xbuf_, static_cast<size_t>(to - xbuf_)))
        return false;
      if (next == b && to == xbuf_)
        return false; // no progress
      b = next;
    }
    return true;
  }
  // [filebuf.members]/8: the put area and the unshift sequence of a close
  bool finish_output() {
    bool ok = true;
    if (io_ == io::writing) {
      if (traits::eq_int_type(overflow(traits::eof()), traits::eof()))
        ok = false;
      if (last_was_overflow_ && !always_noconv_ && !unshift())
        ok = false;
    }
    return ok;
  }
  bool unshift() {
    for (;;) {
      char* to = xbuf_;
      const codecvt_base::result r = cvt_->unshift(state_, xbuf_, xbuf_ + xbuf_size_, to);
      if (r == codecvt_base::error)
        return false;
      if (r == codecvt_base::noconv)
        return true;
      if (!ycxx::detail::file_write(file_, xbuf_, static_cast<size_t>(to - xbuf_)))
        return false;
      if (r == codecvt_base::ok)
        return true;
    }
  }
  // Leaves writing mode: the put area and unshift sequence go to the file.
  bool finish_output_mode() {
    if (io_ != io::writing)
      return true;
    const bool ok = finish_output();
    this->setp(nullptr, nullptr);
    io_ = io::none;
    last_was_overflow_ = false;
    return ok;
  }

  // ---- input ----
  // The get area: ibuf_[0] keeps the last character of the previous area (the putback
  // position); the characters read start at ibuf_ + 1.
  // Reads and converts the next characters; false at end of file (or on a conversion error).
  bool fill_get_area() {
    io_ = io::reading;
    last_was_overflow_ = false;
    charT* const start = ibuf_ + 1;
    const bool keep = this->gptr() != nullptr && this->eback() < this->gptr();
    if (keep)
      ibuf_[0] = this->gptr()[-1];
    charT* const first = keep ? ibuf_ : start;
    const size_t room = ibuf_size_;
    if constexpr (is_same_v<charT, char>) {
      if (always_noconv_) {
        const size_t n = ycxx::detail::file_read(file_, start, room);
        this->setg(first, start, start + n);
        xnext_ = xend_ = 0;
        return n != 0;
      }
    }
    for (;;) {
      // the bytes not yet converted move to the front; more are read after them
      if (xnext_ != 0) {
        __builtin_memmove(xbuf_, xbuf_ + xnext_, xend_ - xnext_);
        xend_ -= xnext_;
        xnext_ = 0;
      }
      bool at_eof = false;
      if (xend_ < xbuf_size_) {
        const size_t got = ycxx::detail::file_read(file_, xbuf_ + xend_, xbuf_size_ - xend_);
        at_eof = got == 0;
        xend_ += got;
      }
      if (xend_ == 0) {
        this->setg(first, start, start);
        return false;
      }
      state_start_ = state_;
      const char* from_next = xbuf_;
      charT* to_next = start;
      codecvt_base::result r = codecvt_base::noconv;
      if (!always_noconv_)
        r = cvt_->in(state_, xbuf_, xbuf_ + xend_, from_next, start, start + room, to_next);
      if (r == codecvt_base::noconv) {
        const size_t chars = xend_ / sizeof(charT) < room ? xend_ / sizeof(charT) : room;
        __builtin_memcpy(static_cast<void*>(start), xbuf_, chars * sizeof(charT));
        from_next = xbuf_ + chars * sizeof(charT);
        to_next = start + chars;
      }
      xnext_ = static_cast<size_t>(from_next - xbuf_);
      if (to_next != start) {
        this->setg(first, start, to_next);
        return true;
      }
      if (r == codecvt_base::error || at_eof || (xnext_ == 0 && xend_ == xbuf_size_)) {
        this->setg(first, start, start);
        return false;
      }
      // partial: more bytes are needed for one character
    }
  }
  // The file offset of gptr() while reading, of the next character written while writing.
  off_type logical_position() {
    const long long now = ycxx::detail::file_seek(file_, 0, 1);
    if (now < 0)
      return -1;
    if (io_ == io::writing) {
      if constexpr (is_same_v<charT, char>) {
        if (always_noconv_)
          return static_cast<off_type>(now + (this->pptr() - this->pbase()));
      }
      // converted output: written out first, to know its length
      if (!write_out(this->pbase(), this->pptr()))
        return -1;
      if (this->pbase() != nullptr)
        this->setp(this->pbase(), this->epptr());
      return static_cast<off_type>(ycxx::detail::file_seek(file_, 0, 1));
    }
    if (io_ != io::reading || this->gptr() == nullptr)
      return static_cast<off_type>(now);
    if constexpr (is_same_v<charT, char>) {
      if (always_noconv_)
        return static_cast<off_type>(now - (this->egptr() - this->gptr()));
    }
    // the bytes [0, xend_) of xbuf_ were read last; the characters from ibuf_ + 1 on were
    // converted from them, starting in state_start_
    const charT* const start = ibuf_ + 1;
    const long long base_pos = now - static_cast<long long>(xend_);
    if (this->gptr() < start) { // in the putback position
      const int width = always_noconv_ ? static_cast<int>(sizeof(charT)) : cvt_->encoding();
      return width > 0 ? static_cast<off_type>(base_pos - width) : off_type(-1);
    }
    const size_t done = static_cast<size_t>(this->gptr() - start);
    long long consumed;
    if (always_noconv_) {
      consumed = static_cast<long long>(done * sizeof(charT));
    } else {
      state_type st = state_start_;
      consumed = cvt_->length(st, xbuf_, xbuf_ + xend_, done);
    }
    return static_cast<off_type>(base_pos + consumed);
  }
  // Leaves reading mode: positions the file at gptr() and drops the get area.
  bool leave_reading() {
    const off_type pos = logical_position();
    discard_get_area();
    if (pos < 0)
      return false;
    return ycxx::detail::file_seek(file_, static_cast<long long>(pos), 0) >= 0;
  }
  void discard_get_area() noexcept {
    this->setg(nullptr, nullptr, nullptr);
    xnext_ = xend_ = 0;
    if (io_ == io::reading)
      io_ = io::none;
  }

  void* file_ = nullptr;
  ios_base::openmode mode_ = ios_base::openmode{};
  io io_ = io::none;
  state_type state_{};
  state_type state_start_{}; // the conversion state at the start of the bytes in xbuf_
  const cvt_type* cvt_ = nullptr;
  bool always_noconv_ = true;
  bool unbuffered_ = false;
  bool last_was_overflow_ = false;
  bool user_ibuf_ = false;
  charT* ibuf_ = nullptr; // ibuf_size_ + 1 characters (the first: a putback position)
  size_t ibuf_size_ = 0;
  char* xbuf_ = nullptr; // bytes read but not yet converted, or converted output
  size_t xbuf_size_ = 0;
  size_t xnext_ = 0, xend_ = 0; // xbuf_: [0, xnext_) converted, [xnext_, xend_) pending
};

template <class charT, class traits>
void swap(basic_filebuf<charT, traits>& x, basic_filebuf<charT, traits>& y) {
  x.swap(y);
}

} // namespace std
