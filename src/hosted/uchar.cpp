// libycxx hosted runtime: the <cuchar> functions (C23 7.30.1) for C libraries that lack them:
// mbrtoc8/c8rtomb where YCXX_C_HAS_MBRTOC8 is 0 (cmake/ycxx-c-library.cmake), the char16_t and
// char32_t ones too where the C library has no <uchar.h> (YCXX_C_HAS_UCHAR_H 0). <cuchar> then declares
// the std:: functions as calls of these; elsewhere they are unused, but built everywhere so that
// every platform compiles them.
//
// A character is converted between the multibyte encoding of the current locale and wchar_t by
// the C library's mbrtowc/wcrtomb, whose wchar_t is UTF-32 (glibc; Darwin in its UTF-8 and C
// locales), and between UTF-32 and UTF-8 or UTF-16 code units here. The mbstate_t object holds
// both: the C library's state at its start, and in its last 16 bytes the code units still to be
// delivered (each returned with (size_t)-3, consuming no input) or the character c8rtomb or
// c16rtomb is assembling. That needs an mbstate_t larger than the C library's own state: Darwin's
// is 128 bytes, of which its conversion states use at most 16 at the start. A state object
// smaller than 32 bytes (glibc's 8) is rejected with EINVAL by the functions that need the room.
#include <cerrno>
#include <climits>
#include <cuchar>
#include <cwchar>

namespace {

constexpr std::size_t tail_size = 16;
constexpr std::size_t error = static_cast<std::size_t>(-1);
constexpr std::size_t incomplete = static_cast<std::size_t>(-2);
constexpr std::size_t stored = static_cast<std::size_t>(-3);

struct conv_tail {
  unsigned char pending[3]; // mbrtoc8: UTF-8 code units still to deliver, from pending[first]
  unsigned char first;
  unsigned char npending;
  unsigned char need;    // c8rtomb: continuation units still expected
  unsigned char length;  // c8rtomb: length of the sequence being assembled
  char32_t partial;      // c8rtomb: the bits of the character so far
  char16_t low;          // mbrtoc16: the low surrogate still to deliver, or 0
  char16_t high;         // c16rtomb: the high surrogate received, or 0
};
static_assert(sizeof(conv_tail) <= tail_size);

// The internal object of each function, used for a null state pointer (C23 7.30.1/1).
struct internal_state {
  alignas(::mbstate_t) unsigned char bytes[(sizeof(::mbstate_t) > tail_size ? sizeof(::mbstate_t) : tail_size) +
                                           tail_size];
};
internal_state mbrtoc8_internal, c8rtomb_internal, mbrtoc16_internal, c16rtomb_internal, mbrtoc32_internal,
    c32rtomb_internal;

// A state object and its size; the internal object for a null pointer.
struct state_ref {
  void* ps;
  std::size_t size;
  state_ref(void* p, std::size_t n, internal_state& internal) noexcept
      : ps(p != nullptr ? p : internal.bytes), size(p != nullptr ? n : sizeof internal.bytes) {}
  ::mbstate_t* c_state() const noexcept { return static_cast<::mbstate_t*>(ps); }
  bool has_tail() const noexcept { return size >= 2 * tail_size; }
  conv_tail load() const noexcept {
    conv_tail t;
    __builtin_memcpy(&t, static_cast<const unsigned char*>(ps) + size - tail_size, sizeof t);
    return t;
  }
  void store(const conv_tail& t) const noexcept {
    __builtin_memcpy(static_cast<unsigned char*>(ps) + size - tail_size, &t, sizeof t);
  }
};

std::size_t fail(int e) noexcept {
  errno = e;
  return error;
}

bool scalar_value(char32_t c) noexcept { return c <= 0x10FFFF && (c < 0xD800 || c > 0xDFFF); }

// The next character of s as UTF-32, through mbrtowc; r is mbrtowc's result.
std::size_t next_char(const char* s, std::size_t n, ::mbstate_t* ps, char32_t& c) noexcept {
  wchar_t wc = 0;
  const std::size_t r = ::mbrtowc(&wc, s, n, ps);
  if (r == error || r == incomplete)
    return r;
  c = static_cast<char32_t>(wc);
  if (!scalar_value(c))
    return fail(EILSEQ);
  return r;
}

std::size_t put_char(char* s, char32_t c, ::mbstate_t* ps) noexcept {
  if (!scalar_value(c))
    return fail(EILSEQ);
  return ::wcrtomb(s, static_cast<wchar_t>(c), ps);
}

} // namespace

std::size_t ycxx::detail::c_mbrtoc8(char8_t* pc8, const char* s, std::size_t n, void* ps,
                                    std::size_t size) noexcept {
  const state_ref st(ps, size, mbrtoc8_internal);
  if (!st.has_tail())
    return fail(EINVAL);
  conv_tail t = st.load();
  if (t.npending != 0) { // a further code unit of the last character
    if (pc8 != nullptr)
      *pc8 = t.pending[t.first];
    ++t.first;
    --t.npending;
    st.store(t);
    return stored;
  }
  if (s == nullptr) { // as mbrtoc8(NULL, "", 1, ps)
    s = "";
    n = 1;
    pc8 = nullptr;
  }
  char32_t c = 0;
  const std::size_t r = next_char(s, n, st.c_state(), c);
  if (r == error || r == incomplete)
    return r;
  unsigned char u[4];
  int len;
  if (c < 0x80) {
    u[0] = static_cast<unsigned char>(c);
    len = 1;
  } else if (c < 0x800) {
    u[0] = static_cast<unsigned char>(0xC0 | (c >> 6));
    len = 2;
  } else if (c < 0x10000) {
    u[0] = static_cast<unsigned char>(0xE0 | (c >> 12));
    len = 3;
  } else {
    u[0] = static_cast<unsigned char>(0xF0 | (c >> 18));
    len = 4;
  }
  for (int i = 1; i < len; ++i)
    u[i] = static_cast<unsigned char>(0x80 | ((c >> (6 * (len - 1 - i))) & 0x3F));
  if (pc8 != nullptr)
    *pc8 = u[0];
  t.first = 0;
  t.npending = static_cast<unsigned char>(len - 1);
  for (int i = 1; i < len; ++i)
    t.pending[i - 1] = u[i];
  st.store(t);
  return r;
}

std::size_t ycxx::detail::c_c8rtomb(char* s, char8_t c8, void* ps, std::size_t size) noexcept {
  const state_ref st(ps, size, c8rtomb_internal);
  if (!st.has_tail())
    return fail(EINVAL);
  char buf[MB_LEN_MAX];
  if (s == nullptr) { // as c8rtomb(buf, u8'\0', ps)
    s = buf;
    c8 = 0;
  }
  conv_tail t = st.load();
  const unsigned u = c8;
  if (t.need == 0) {
    if (u < 0x80)
      return put_char(s, u, st.c_state());
    if (u >= 0xC2 && u <= 0xDF) {
      t.length = 2;
      t.partial = u & 0x1F;
    } else if (u >= 0xE0 && u <= 0xEF) {
      t.length = 3;
      t.partial = u & 0x0F;
    } else if (u >= 0xF0 && u <= 0xF4) {
      t.length = 4;
      t.partial = u & 0x07;
    } else {
      return fail(EILSEQ);
    }
    t.need = static_cast<unsigned char>(t.length - 1);
    st.store(t);
    return 0; // no character completed yet
  }
  if ((u & 0xC0) != 0x80) {
    t.need = 0;
    st.store(t);
    return fail(EILSEQ);
  }
  t.partial = (t.partial << 6) | (u & 0x3F);
  const bool done = --t.need == 0;
  st.store(t);
  if (!done)
    return 0;
  // The shortest form only (scalar values are checked by put_char).
  const char32_t least = t.length == 2 ? 0x80 : t.length == 3 ? 0x800 : 0x10000;
  if (t.partial < least)
    return fail(EILSEQ);
  return put_char(s, t.partial, st.c_state());
}

std::size_t ycxx::detail::c_mbrtoc16(char16_t* pc16, const char* s, std::size_t n, void* ps,
                                     std::size_t size) noexcept {
  const state_ref st(ps, size, mbrtoc16_internal);
  if (!st.has_tail())
    return fail(EINVAL);
  conv_tail t = st.load();
  if (t.low != 0) { // the second unit of a surrogate pair
    if (pc16 != nullptr)
      *pc16 = t.low;
    t.low = 0;
    st.store(t);
    return stored;
  }
  if (s == nullptr) {
    s = "";
    n = 1;
    pc16 = nullptr;
  }
  char32_t c = 0;
  const std::size_t r = next_char(s, n, st.c_state(), c);
  if (r == error || r == incomplete)
    return r;
  if (c >= 0x10000) {
    c -= 0x10000;
    if (pc16 != nullptr)
      *pc16 = static_cast<char16_t>(0xD800 | (c >> 10));
    t.low = static_cast<char16_t>(0xDC00 | (c & 0x3FF));
    st.store(t);
  } else if (pc16 != nullptr) {
    *pc16 = static_cast<char16_t>(c);
  }
  return r;
}

std::size_t ycxx::detail::c_c16rtomb(char* s, char16_t c16, void* ps, std::size_t size) noexcept {
  const state_ref st(ps, size, c16rtomb_internal);
  if (!st.has_tail())
    return fail(EINVAL);
  char buf[MB_LEN_MAX];
  if (s == nullptr) {
    s = buf;
    c16 = 0;
  }
  conv_tail t = st.load();
  const bool is_high = c16 >= 0xD800 && c16 <= 0xDBFF, is_low = c16 >= 0xDC00 && c16 <= 0xDFFF;
  if (t.high != 0) {
    const char16_t high = t.high;
    t.high = 0;
    st.store(t);
    if (!is_low)
      return fail(EILSEQ);
    return put_char(s, 0x10000 + ((char32_t(high) - 0xD800) << 10) + (char32_t(c16) - 0xDC00), st.c_state());
  }
  if (is_high) {
    t.high = c16;
    st.store(t);
    return 0;
  }
  if (is_low)
    return fail(EILSEQ);
  return put_char(s, c16, st.c_state());
}

std::size_t ycxx::detail::c_mbrtoc32(char32_t* pc32, const char* s, std::size_t n, void* ps,
                                     std::size_t size) noexcept {
  const state_ref st(ps, size, mbrtoc32_internal);
  if (s == nullptr) {
    s = "";
    n = 1;
    pc32 = nullptr;
  }
  char32_t c = 0;
  const std::size_t r = next_char(s, n, st.c_state(), c);
  if (r != error && r != incomplete && pc32 != nullptr)
    *pc32 = c;
  return r;
}

std::size_t ycxx::detail::c_c32rtomb(char* s, char32_t c32, void* ps, std::size_t size) noexcept {
  const state_ref st(ps, size, c32rtomb_internal);
  char buf[MB_LEN_MAX];
  if (s == nullptr) {
    s = buf;
    c32 = 0;
  }
  return put_char(s, c32, st.c_state());
}
