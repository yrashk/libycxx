// libycxx hosted runtime: the eight standard stream objects ([iostream.objects]), ios_base::Init
// and sync_with_stdio.
//
// The objects are defined here as raw storage under their own names (this file does not include
// <iostream>, whose declarations give them their stream types; variable names do not encode
// types in the Itanium ABI), and constructed in place by the first ios_base::Init. The runtime
// owns one Init object initialized with init_priority before any ordinary static object, so the
// streams exist before any user static initializer runs, and that object's destructor runs after
// every ordinary static destructor and flushes the output streams. They are never destroyed.
//
// Their stream buffers work on the C streams stdin, stdout and stderr through C stdio. While
// synchronized with stdio (the default) they keep no buffer of their own: every character goes
// through putc / getc / ungetc, so output and input interleave with C stdio exactly
// ([ios.members.static]/3). sync_with_stdio(false) gives the output buffers a buffer of their
// own. The wide objects convert through the codecvt<wchar_t, char, mbstate_t> of their buffer's
// locale and write bytes, so they do not set the C streams' orientation.
#include <istream>
#include <ostream>
#include <cstdio>
#include <new>

namespace {

template <class T>
union immortal {
  T object;
  immortal() {}
  ~immortal() {}
};

// ---- char -------------------------------------------------------------------------------------
class stdio_buf final : public std::streambuf {
public:
  explicit stdio_buf(std::FILE* f) noexcept : f_(f) {}

  // Switches between unbuffered (synchronized) and buffered output.
  void set_buffered(bool b) {
    sync();
    if (b)
      setp(buf_, buf_ + sizeof buf_);
    else
      setp(nullptr, nullptr);
  }

protected:
  int_type overflow(int_type c) override {
    if (pbase() != nullptr && !flush_buffer())
      return traits_type::eof();
    if (traits_type::eq_int_type(c, traits_type::eof()))
      return traits_type::not_eof(c);
    if (pbase() != nullptr) {
      *pptr() = traits_type::to_char_type(c);
      pbump(1);
      return c;
    }
    return std::putc(c, f_) == EOF ? traits_type::eof() : c;
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    if (pbase() != nullptr) {
      if (n <= epptr() - pptr())
        return std::streambuf::xsputn(s, n);
      if (!flush_buffer())
        return 0;
    }
    return static_cast<std::streamsize>(std::fwrite(s, 1, static_cast<std::size_t>(n), f_));
  }
  int sync() override {
    if (pbase() != nullptr && !flush_buffer())
      return -1;
    return std::fflush(f_) == 0 ? 0 : -1;
  }
  int_type underflow() override {
    const int c = std::getc(f_);
    if (c == EOF)
      return traits_type::eof();
    std::ungetc(c, f_);
    return c;
  }
  int_type uflow() override {
    const int c = std::getc(f_);
    if (c == EOF)
      return traits_type::eof();
    last_ = c;
    return c;
  }
  int_type pbackfail(int_type c) override {
    if (traits_type::eq_int_type(c, traits_type::eof())) {
      if (last_ == EOF)
        return traits_type::eof();
      c = last_;
    }
    last_ = EOF;
    return std::ungetc(c, f_) == EOF ? traits_type::eof() : c;
  }

private:
  bool flush_buffer() {
    const std::size_t n = static_cast<std::size_t>(pptr() - pbase());
    const bool ok = n == 0 || std::fwrite(pbase(), 1, n, f_) == n;
    setp(buf_, buf_ + sizeof buf_);
    return ok;
  }

  std::FILE* f_;
  int last_ = EOF; // the character last extracted, for sungetc
  char buf_[1024];
};

// ---- wchar_t ----------------------------------------------------------------------------------
class wstdio_buf final : public std::wstreambuf {
public:
  explicit wstdio_buf(std::FILE* f) : f_(f), cvt_(&std::use_facet<cvt_type>(getloc())) {}

  void set_buffered(bool b) {
    sync();
    if (b)
      setp(buf_, buf_ + sizeof buf_ / sizeof buf_[0]);
    else
      setp(nullptr, nullptr);
  }

protected:
  using cvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;

  void imbue(const std::locale& loc) override { cvt_ = &std::use_facet<cvt_type>(loc); }

  int_type overflow(int_type c) override {
    if (pbase() != nullptr && !flush_buffer())
      return traits_type::eof();
    if (traits_type::eq_int_type(c, traits_type::eof()))
      return traits_type::not_eof(c);
    if (pbase() != nullptr) {
      *pptr() = traits_type::to_char_type(c);
      pbump(1);
      return c;
    }
    const wchar_t w = traits_type::to_char_type(c);
    return write(&w, 1) ? c : traits_type::eof();
  }
  std::streamsize xsputn(const wchar_t* s, std::streamsize n) override {
    if (pbase() != nullptr) {
      if (n <= epptr() - pptr())
        return std::wstreambuf::xsputn(s, n);
      if (!flush_buffer())
        return 0;
    }
    return write(s, static_cast<std::size_t>(n)) ? n : 0;
  }
  int sync() override {
    if (pbase() != nullptr && !flush_buffer())
      return -1;
    return std::fflush(f_) == 0 ? 0 : -1;
  }
  int_type underflow() override {
    if (!has_peek_) {
      const int_type c = read();
      if (traits_type::eq_int_type(c, traits_type::eof()))
        return c;
      peek_ = traits_type::to_char_type(c);
      has_peek_ = true;
    }
    return traits_type::to_int_type(peek_);
  }
  int_type uflow() override {
    const int_type c = underflow();
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
      has_peek_ = false;
      last_ = traits_type::to_char_type(c);
      has_last_ = true;
    }
    return c;
  }
  int_type pbackfail(int_type c) override {
    if (has_peek_)
      return traits_type::eof(); // one character of putback
    if (traits_type::eq_int_type(c, traits_type::eof())) {
      if (!has_last_)
        return traits_type::eof();
      c = traits_type::to_int_type(last_);
    }
    has_last_ = false;
    peek_ = traits_type::to_char_type(c);
    has_peek_ = true;
    return c;
  }

private:
  // Converts and writes s[0..n) as bytes.
  bool write(const wchar_t* s, std::size_t n) {
    char out[256];
    const wchar_t* from = s;
    const wchar_t* const end = s + n;
    while (from != end) {
      const wchar_t* next = from;
      char* to = out;
      const std::codecvt_base::result r = cvt_->out(state_, from, end, next, out, out + sizeof out, to);
      if (r == std::codecvt_base::error)
        return false;
      if (r == std::codecvt_base::noconv) {
        for (; from != end; ++from)
          if (std::putc(static_cast<char>(*from), f_) == EOF)
            return false;
        return true;
      }
      const std::size_t bytes = static_cast<std::size_t>(to - out);
      if (bytes != 0 && std::fwrite(out, 1, bytes, f_) != bytes)
        return false;
      if (next == from && bytes == 0)
        return false; // no progress
      from = next;
    }
    return true;
  }
  // Reads bytes until they convert to one wide character.
  int_type read() {
    char bytes[8];
    int n = 0;
    while (n < static_cast<int>(sizeof bytes)) {
      const int b = std::getc(f_);
      if (b == EOF)
        return traits_type::eof();
      bytes[n++] = static_cast<char>(b);
      wchar_t w;
      const char* from_next;
      wchar_t* to_next;
      std::mbstate_t st = in_state_;
      const std::codecvt_base::result r = cvt_->in(st, bytes, bytes + n, from_next, &w, &w + 1, to_next);
      if (r == std::codecvt_base::noconv)
        return traits_type::to_int_type(static_cast<wchar_t>(static_cast<unsigned char>(bytes[0])));
      if (r == std::codecvt_base::error)
        return traits_type::eof();
      if (to_next != &w) {
        in_state_ = st;
        // give back bytes beyond the character, if any
        for (const char* p = bytes + n; p != from_next;)
          std::ungetc(static_cast<unsigned char>(*--p), f_);
        return traits_type::to_int_type(w);
      }
    }
    return traits_type::eof();
  }
  bool flush_buffer() {
    const std::size_t n = static_cast<std::size_t>(pptr() - pbase());
    const bool ok = n == 0 || write(pbase(), n);
    setp(buf_, buf_ + sizeof buf_ / sizeof buf_[0]);
    return ok;
  }

  std::FILE* f_;
  const cvt_type* cvt_;
  std::mbstate_t state_{};
  std::mbstate_t in_state_{};
  wchar_t peek_ = 0;
  bool has_peek_ = false;
  wchar_t last_ = 0;
  bool has_last_ = false;
  wchar_t buf_[256];
};

immortal<stdio_buf> cin_buf, cout_buf, cerr_buf;
immortal<wstdio_buf> wcin_buf, wcout_buf, wcerr_buf;

int init_count = 0;  // live ios_base::Init objects (atomic)
bool synced = true;  // sync_with_stdio state

} // namespace

// The objects, as storage; <iostream> declares them with their stream types.
namespace std {
alignas(istream) unsigned char cin[sizeof(istream)];
alignas(ostream) unsigned char cout[sizeof(ostream)];
alignas(ostream) unsigned char cerr[sizeof(ostream)];
alignas(ostream) unsigned char clog[sizeof(ostream)];
alignas(wistream) unsigned char wcin[sizeof(wistream)];
alignas(wostream) unsigned char wcout[sizeof(wostream)];
alignas(wostream) unsigned char wcerr[sizeof(wostream)];
alignas(wostream) unsigned char wclog[sizeof(wostream)];
} // namespace std

namespace {

template <class S>
S& object(unsigned char* storage) {
  return *std::launder(reinterpret_cast<S*>(storage));
}

bool construct_objects() {
  ::new (static_cast<void*>(&cin_buf.object)) stdio_buf(stdin);
  ::new (static_cast<void*>(&cout_buf.object)) stdio_buf(stdout);
  ::new (static_cast<void*>(&cerr_buf.object)) stdio_buf(stderr);
  ::new (static_cast<void*>(&wcin_buf.object)) wstdio_buf(stdin);
  ::new (static_cast<void*>(&wcout_buf.object)) wstdio_buf(stdout);
  ::new (static_cast<void*>(&wcerr_buf.object)) wstdio_buf(stderr);
  std::ostream* out = ::new (static_cast<void*>(std::cout)) std::ostream(&cout_buf.object);
  std::istream* in = ::new (static_cast<void*>(std::cin)) std::istream(&cin_buf.object);
  std::ostream* err = ::new (static_cast<void*>(std::cerr)) std::ostream(&cerr_buf.object);
  ::new (static_cast<void*>(std::clog)) std::ostream(&cerr_buf.object);
  in->tie(out);
  err->setf(std::ios_base::unitbuf);
  err->tie(out);
  std::wostream* wout = ::new (static_cast<void*>(std::wcout)) std::wostream(&wcout_buf.object);
  std::wistream* win = ::new (static_cast<void*>(std::wcin)) std::wistream(&wcin_buf.object);
  std::wostream* werr = ::new (static_cast<void*>(std::wcerr)) std::wostream(&wcerr_buf.object);
  ::new (static_cast<void*>(std::wclog)) std::wostream(&wcerr_buf.object);
  win->tie(wout);
  werr->setf(std::ios_base::unitbuf);
  werr->tie(wout);
  return true;
}

void flush_objects() {
  object<std::ostream>(std::cout).flush();
  object<std::ostream>(std::cerr).flush();
  object<std::ostream>(std::clog).flush();
  object<std::wostream>(std::wcout).flush();
  object<std::wostream>(std::wcerr).flush();
  object<std::wostream>(std::wclog).flush();
}

} // namespace

namespace std {

ios_base::Init::Init() {
  static const bool constructed = construct_objects();
  (void)constructed;
  __atomic_add_fetch(&init_count, 1, __ATOMIC_ACQ_REL);
}

ios_base::Init::~Init() {
  if (__atomic_sub_fetch(&init_count, 1, __ATOMIC_ACQ_REL) == 0)
    flush_objects();
}

bool ios_base::sync_with_stdio(bool sync) {
  const bool old = synced;
  if (sync != old) {
    synced = sync;
    cout_buf.object.set_buffered(!sync);
    wcout_buf.object.set_buffered(!sync);
  }
  return old;
}

} // namespace std

namespace {
// Initialized before every object with ordinary static initialization (priorities 101 and up
// belong to programs); destroyed after them.
[[gnu::init_priority(100)]] std::ios_base::Init runtime_init;
} // namespace
