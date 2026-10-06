// libycxx hosted runtime: the eight standard stream objects ([iostream.objects]), ios_base::Init
// and sync_with_stdio.
//
// The objects are defined here as raw storage under their own names (this file does not include
// <iostream>, whose declarations give them their stream types; variable names do not encode
// types in the Itanium ABI), and constructed in place by the first ios_base::Init. On ELF the
// runtime owns one Init object initialized with init_priority before any ordinary static object,
// so the streams exist before any user static initializer runs, and that object's destructor runs
// after every ordinary static destructor and flushes the output streams. Mach-O has no such
// priorities: there <iostream> defines an Init object in every translation unit that includes it
// (_YCXX_HAS_INIT_PRIORITY, config.hpp; DECISIONS §7). They are never destroyed.
//
// Their stream buffers work on the C streams stdin, stdout and stderr through C stdio. While
// synchronized with stdio (the default) they keep no buffer of their own: every character goes
// through putc / getc / ungetc, so output and input interleave with C stdio exactly
// ([ios.members.static]/3). sync_with_stdio(false) gives the output buffers a buffer of their
// own.
//
// The wide objects do wide I/O on the C streams (fputwc / fputws, fgetwc, ungetwc), so the C
// stream becomes wide-oriented and they mix with the C library's wide functions as FILEs do
// ([iostream.objects.overview]/6, [ios.members.static]/3 for the wide streams): wcout leaves
// stdout wide-oriented, and wcin.unget() gives the character back for fgetwc. The C library
// converts, with its LC_CTYPE (the "C" locale until the program calls setlocale, where a
// character outside the basic character set fails). A buffer imbued with a locale whose
// codecvt<wchar_t, char, mbstate_t> is not the one it started with (a program's own facet) does
// what a basic_filebuf<wchar_t> does instead ([iostream.objects.overview]/2): it converts through
// that facet and does byte I/O on the C stream. sync_with_stdio(false) changes only the
// buffering (DECISIONS §7).
//
// The input and output functions of the synchronized objects may be called from several threads
// at once ([iostream.objects.overview]/7; DECISIONS §7). Each buffer's input side (underflow,
// uflow, pbackfail) and the wide buffers' conversion to bytes hold a lock of the buffer's own, a
// futex mutex: one atomic operation each way while uncontended, plain loads and stores in a
// single-threaded process. It guards what the buffer remembers between calls (the character
// last extracted, for sungetc; the wide buffers' conversion states), and makes a peek (read,
// then give back to the C stream) atomic with respect to the other readers of the object, so
// they never see the stream's characters out of order. (The C stream's own lock would do the
// same, but flockfile is not visible to ThreadSanitizer, nor are the C library's internal locks:
// a pushed-back character's storage allocated by one thread and freed by another is reported
// unless a lock it understands orders them.) The narrow output side needs no lock: putc and
// fwrite lock the C stream. The stream objects' state and gcount are relaxed atomics (<ios>,
// <istream>).
#include <istream>
#include <ostream>
#include <cstdio>
#include <cwchar>
#include <new>
#include <ycxx/hosted/thread_support.hpp>

namespace {

// Holds a buffer's lock for the duration of one of its operations.
class __guard {
public:
  explicit __guard(__ycxx::__detail::__futex_mutex& m) noexcept : __m_(m) { __m_.lock(); }
  ~__guard() { __m_.unlock(); }
  __guard(const __guard&) = delete;
  __guard& operator=(const __guard&) = delete;

private:
  __ycxx::__detail::__futex_mutex& __m_;
};

template <class _Tp>
union immortal {
  _Tp __object;
  immortal() {}
  ~immortal() {}
};

// ---- char -------------------------------------------------------------------------------------
class stdio_buf final : public std::streambuf {
public:
  explicit stdio_buf(std::FILE* __f) noexcept : __f_(__f) {}

  // Switches between unbuffered (synchronized) and buffered output.
  void set_buffered(bool b) {
    sync();
    if (b)
      setp(__buf_, __buf_ + sizeof __buf_);
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
    return std::putc(c, __f_) == EOF ? traits_type::eof() : c;
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    if (pbase() != nullptr) {
      if (n <= epptr() - pptr())
        return std::streambuf::xsputn(s, n);
      if (!flush_buffer())
        return 0;
    }
    return static_cast<std::streamsize>(std::fwrite(s, 1, static_cast<std::size_t>(n), __f_));
  }
  int sync() override {
    if (pbase() != nullptr && !flush_buffer())
      return -1;
    return std::fflush(__f_) == 0 ? 0 : -1;
  }
  int_type underflow() override {
    __guard __g(__lock_);
    const int c = std::getc(__f_);
    if (c == EOF)
      return traits_type::eof();
    std::ungetc(c, __f_);
    return c;
  }
  int_type uflow() override {
    __guard __g(__lock_);
    const int c = std::getc(__f_);
    if (c == EOF)
      return traits_type::eof();
    __last_ = c;
    return c;
  }
  int_type pbackfail(int_type c) override {
    __guard __g(__lock_);
    if (traits_type::eq_int_type(c, traits_type::eof())) {
      if (__last_ == EOF)
        return traits_type::eof();
      c = __last_;
    }
    __last_ = EOF;
    return std::ungetc(c, __f_) == EOF ? traits_type::eof() : c;
  }

private:
  bool flush_buffer() {
    const std::size_t n = static_cast<std::size_t>(pptr() - pbase());
    const bool ok = n == 0 || std::fwrite(pbase(), 1, n, __f_) == n;
    setp(__buf_, __buf_ + sizeof __buf_);
    return ok;
  }

  std::FILE* __f_;
  __ycxx::__detail::__futex_mutex __lock_; // the input side
  int __last_ = EOF; // the character last extracted, for sungetc (guarded by lock_)
  char __buf_[1024];
};

// ---- wchar_t ----------------------------------------------------------------------------------
class wstdio_buf final : public std::wstreambuf {
public:
  explicit wstdio_buf(std::FILE* __f) : __f_(__f), __cvt_(&std::use_facet<__cvt_type>(getloc())), c_cvt_(__cvt_) {}

  void set_buffered(bool b) {
    sync();
    if (b)
      setp(__buf_, __buf_ + sizeof __buf_ / sizeof __buf_[0]);
    else
      setp(nullptr, nullptr);
  }

protected:
  using __cvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;

  // Characters already in the put area (sync_with_stdio(false)) are written the way they were
  // put, before the conversion changes.
  void imbue(const std::locale& __loc) override {
    if (pbase() != nullptr)
      flush_buffer();
    __guard __g(__lock_);
    __cvt_ = &std::use_facet<__cvt_type>(__loc);
  }

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
    const wchar_t __w = traits_type::to_char_type(c);
    return write(&__w, 1) ? c : traits_type::eof();
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
    return std::fflush(__f_) == 0 ? 0 : -1;
  }
  // Like the narrow buffer, this one holds no characters: a peeked character goes back to the C
  // stream (ungetwc), and so does a put-back character. C stdio and the other readers of the
  // object then see every character exactly once and in order. Through a codecvt the character's
  // bytes go back (ungetc); a character of more than one byte needs more than the one byte of
  // push-back that ISO C guarantees, which glibc and Darwin's libc provide.
  int_type underflow() override {
    __guard __g(__lock_);
    if (wide_c_io()) {
      const std::wint_t c = std::fgetwc(__f_);
      if (c == WEOF)
        return traits_type::eof();
      std::ungetwc(c, __f_);
      return traits_type::to_int_type(static_cast<wchar_t>(c));
    }
    return read(false);
  }
  int_type uflow() override {
    __guard __g(__lock_);
    int_type c;
    if (wide_c_io()) {
      const std::wint_t __w = std::fgetwc(__f_);
      c = __w == WEOF ? traits_type::eof() : traits_type::to_int_type(static_cast<wchar_t>(__w));
    } else {
      c = read(true);
    }
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
      __last_ = traits_type::to_char_type(c);
      has_last_ = true;
    }
    return c;
  }
  int_type pbackfail(int_type c) override {
    __guard __g(__lock_);
    if (traits_type::eq_int_type(c, traits_type::eof())) {
      if (!has_last_)
        return traits_type::eof();
      c = traits_type::to_int_type(__last_);
    }
    has_last_ = false;
    const wchar_t __w = traits_type::to_char_type(c);
    if (wide_c_io())
      return std::ungetwc(static_cast<std::wint_t>(__w), __f_) == WEOF ? traits_type::eof() : c;
    char bytes[16];
    std::size_t n = 1;
    bytes[0] = static_cast<char>(__w);
    std::mbstate_t __st{};
    const wchar_t* __from_next;
    char* to = bytes;
    const std::codecvt_base::result r = __cvt_->out(__st, &__w, &__w + 1, __from_next, bytes, bytes + sizeof bytes, to);
    if (r == std::codecvt_base::error || r == std::codecvt_base::partial)
      return traits_type::eof();
    if (r == std::codecvt_base::ok)
      n = static_cast<std::size_t>(to - bytes);
    while (n != 0)
      if (std::ungetc(static_cast<unsigned char>(bytes[--n]), __f_) == EOF)
        return traits_type::eof();
    return c;
  }

private:
  // Whether the C library does the I/O and the conversion (the C stream's wide functions), or
  // the buffer's codecvt with the C stream's byte functions (lock_ is held).
  bool wide_c_io() const noexcept { return __cvt_ == c_cvt_; }

  // Writes s[0..n): wide characters, or bytes converted by the codecvt.
  bool write(const wchar_t* s, std::size_t n) {
    __guard __g(__lock_); // for state_ and cvt_; the characters of one call also stay together
    if (wide_c_io())
      return write_wide(s, n);
    char out[256];
    const wchar_t* from = s;
    const wchar_t* const end = s + n;
    while (from != end) {
      const wchar_t* next = from;
      char* to = out;
      const std::codecvt_base::result r = __cvt_->out(__state_, from, end, next, out, out + sizeof out, to);
      if (r == std::codecvt_base::error)
        return false;
      if (r == std::codecvt_base::noconv) {
        for (; from != end; ++from)
          if (std::putc(static_cast<char>(*from), __f_) == EOF)
            return false;
        return true;
      }
      const std::size_t bytes = static_cast<std::size_t>(to - out);
      if (bytes != 0 && std::fwrite(out, 1, bytes, __f_) != bytes)
        return false;
      if (next == from && bytes == 0)
        return false; // no progress
      from = next;
    }
    return true;
  }
  // fputws a null-terminated copy of each piece without a null character; fputwc the null
  // characters (lock_ is held).
  bool write_wide(const wchar_t* s, std::size_t n) {
    wchar_t piece[128];
    while (n != 0) {
      if (*s == L'\0') {
        if (std::fputwc(L'\0', __f_) == WEOF)
          return false;
        ++s;
        --n;
        continue;
      }
      std::size_t k = 0;
      while (k != n && k != sizeof piece / sizeof piece[0] - 1 && s[k] != L'\0') {
        piece[k] = s[k];
        ++k;
      }
      piece[k] = L'\0';
      if (std::fputws(piece, __f_) < 0)
        return false;
      s += k;
      n -= k;
    }
    return true;
  }
  // Reads bytes until they convert to one wide character (lock_ is held). Unless consume is set,
  // they all go back to the C stream and the conversion state is left as it was.
  int_type read(bool consume) {
    char bytes[8];
    int n = 0;
    while (n < static_cast<int>(sizeof bytes)) {
      const int b = std::getc(__f_);
      if (b == EOF)
        return traits_type::eof();
      bytes[n++] = static_cast<char>(b);
      wchar_t __w;
      const char* __from_next;
      wchar_t* __to_next;
      std::mbstate_t __st = in_state_;
      const std::codecvt_base::result r = __cvt_->in(__st, bytes, bytes + n, __from_next, &__w, &__w + 1, __to_next);
      if (r == std::codecvt_base::noconv) {
        if (!consume)
          std::ungetc(static_cast<unsigned char>(bytes[0]), __f_);
        return traits_type::to_int_type(static_cast<wchar_t>(static_cast<unsigned char>(bytes[0])));
      }
      if (r == std::codecvt_base::error)
        return traits_type::eof();
      if (__to_next != &__w) {
        if (consume)
          in_state_ = __st;
        // give back the bytes beyond the character, and the character's own unless consumed
        for (const char* p = bytes + n, *__stop = consume ? __from_next : bytes; p != __stop;)
          std::ungetc(static_cast<unsigned char>(*--p), __f_);
        return traits_type::to_int_type(__w);
      }
    }
    return traits_type::eof();
  }
  bool flush_buffer() {
    const std::size_t n = static_cast<std::size_t>(pptr() - pbase());
    const bool ok = n == 0 || write(pbase(), n);
    setp(__buf_, __buf_ + sizeof __buf_ / sizeof __buf_[0]);
    return ok;
  }

  std::FILE* __f_;
  const __cvt_type* __cvt_;   // the locale's (guarded by lock_)
  const __cvt_type* c_cvt_; // the initial locale's: with it, the C library converts
  __ycxx::__detail::__futex_mutex __lock_; // guards the members below
  std::mbstate_t __state_{};
  std::mbstate_t in_state_{};
  wchar_t __last_ = 0;
  bool has_last_ = false;
  wchar_t __buf_[256];
};

immortal<stdio_buf> cin_buf, cout_buf, cerr_buf;
immortal<wstdio_buf> wcin_buf, wcout_buf, wcerr_buf;

int init_count = 0;  // live ios_base::Init objects (atomic)
bool synced = true;  // sync_with_stdio state

} // namespace

// The objects, as storage; <iostream> declares them with their stream types.
namespace [[__gnu__::__visibility__("hidden")]] std {
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

template <class _Sp>
_Sp& __object(unsigned char* __storage) {
  return *std::launder(reinterpret_cast<_Sp*>(__storage));
}

bool construct_objects() {
  ::new (static_cast<void*>(&cin_buf.__object)) stdio_buf(stdin);
  ::new (static_cast<void*>(&cout_buf.__object)) stdio_buf(stdout);
  ::new (static_cast<void*>(&cerr_buf.__object)) stdio_buf(stderr);
  ::new (static_cast<void*>(&wcin_buf.__object)) wstdio_buf(stdin);
  ::new (static_cast<void*>(&wcout_buf.__object)) wstdio_buf(stdout);
  ::new (static_cast<void*>(&wcerr_buf.__object)) wstdio_buf(stderr);
  std::ostream* out = ::new (static_cast<void*>(std::cout)) std::ostream(&cout_buf.__object);
  std::istream* in = ::new (static_cast<void*>(std::cin)) std::istream(&cin_buf.__object);
  std::ostream* __err = ::new (static_cast<void*>(std::cerr)) std::ostream(&cerr_buf.__object);
  ::new (static_cast<void*>(std::clog)) std::ostream(&cerr_buf.__object);
  in->tie(out);
  __err->setf(std::ios_base::unitbuf);
  __err->tie(out);
  std::wostream* wout = ::new (static_cast<void*>(std::wcout)) std::wostream(&wcout_buf.__object);
  std::wistream* win = ::new (static_cast<void*>(std::wcin)) std::wistream(&wcin_buf.__object);
  std::wostream* werr = ::new (static_cast<void*>(std::wcerr)) std::wostream(&wcerr_buf.__object);
  ::new (static_cast<void*>(std::wclog)) std::wostream(&wcerr_buf.__object);
  win->tie(wout);
  werr->setf(std::ios_base::unitbuf);
  werr->tie(wout);
  return true;
}

void flush_objects() {
  __object<std::ostream>(std::cout).flush();
  __object<std::ostream>(std::cerr).flush();
  __object<std::ostream>(std::clog).flush();
  __object<std::wostream>(std::wcout).flush();
  __object<std::wostream>(std::wcerr).flush();
  __object<std::wostream>(std::wclog).flush();
}

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] std {

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
  const bool __old = synced;
  if (sync != __old) {
    synced = sync;
    cout_buf.__object.set_buffered(!sync);
    wcout_buf.__object.set_buffered(!sync);
  }
  return __old;
}

} // namespace std

namespace {
#if _YCXX_HAS_INIT_PRIORITY
// Initialized before every object with ordinary static initialization (priorities 101 and up
// belong to programs); destroyed after them.
[[__gnu__::__init_priority__(100)]] std::ios_base::Init runtime_init;
#else
// Mach-O orders static initialization only by link order, where the runtime comes after the
// program's objects: there each translation unit that includes <iostream> has an Init object of
// its own (cfg::init_priority, <iostream>), and this one only makes sure the objects exist
// before main.
std::ios_base::Init runtime_init;
#endif
} // namespace
