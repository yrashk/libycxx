// libycxx hosted runtime: the <cuchar> functions (C23 7.30.1) for C libraries that lack them:
// mbrtoc8/c8rtomb where _YCXX_C_HAS_MBRTOC8 is 0 (cmake/ycxx-c-library.cmake), the char16_t and
// char32_t ones too where the C library has no <uchar.h> (_YCXX_C_HAS_UCHAR_H 0). <cuchar> then declares
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
constexpr std::size_t __stored = static_cast<std::size_t>(-3);

struct conv_tail {
  unsigned char pending[3]; // mbrtoc8: UTF-8 code units still to deliver, from pending[first]
  unsigned char first;
  unsigned char npending;
  unsigned char __need;    // c8rtomb: continuation units still expected
  unsigned char length;  // c8rtomb: length of the sequence being assembled
  char32_t partial;      // c8rtomb: the bits of the character so far
  char16_t __low;          // mbrtoc16: the low surrogate still to deliver, or 0
  char16_t __high;         // c16rtomb: the high surrogate received, or 0
};
static_assert(sizeof(conv_tail) <= tail_size);

// The internal object of each function, used for a null state pointer (C23 7.30.1/1).
struct internal_state {
  alignas(::mbstate_t) unsigned char __bytes[(sizeof(::mbstate_t) > tail_size ? sizeof(::mbstate_t) : tail_size) +
                                           tail_size];
};
internal_state mbrtoc8_internal, c8rtomb_internal, mbrtoc16_internal, c16rtomb_internal, mbrtoc32_internal,
    c32rtomb_internal;

// A state object and its size; the internal object for a null pointer.
struct state_ref {
  void* __ps;
  std::size_t size;
  state_ref(void* p, std::size_t n, internal_state& internal) noexcept
      : __ps(p != nullptr ? p : internal.__bytes), size(p != nullptr ? n : sizeof internal.__bytes) {}
  ::mbstate_t* c_state() const noexcept { return static_cast<::mbstate_t*>(__ps); }
  bool has_tail() const noexcept { return size >= 2 * tail_size; }
  conv_tail load() const noexcept {
    conv_tail t;
    __builtin_memcpy(&t, static_cast<const unsigned char*>(__ps) + size - tail_size, sizeof t);
    return t;
  }
  void store(const conv_tail& t) const noexcept {
    __builtin_memcpy(static_cast<unsigned char*>(__ps) + size - tail_size, &t, sizeof t);
  }
};

std::size_t fail(int e) noexcept {
  errno = e;
  return error;
}

bool scalar_value(char32_t c) noexcept { return c <= 0x10FFFF && (c < 0xD800 || c > 0xDFFF); }

// The next character of s as UTF-32, through mbrtowc; r is mbrtowc's result.
std::size_t next_char(const char* s, std::size_t n, ::mbstate_t* __ps, char32_t& c) noexcept {
  wchar_t wc = 0;
  const std::size_t r = ::mbrtowc(&wc, s, n, __ps);
  if (r == error || r == incomplete)
    return r;
  c = static_cast<char32_t>(wc);
  if (!scalar_value(c))
    return fail(EILSEQ);
  return r;
}

std::size_t put_char(char* s, char32_t c, ::mbstate_t* __ps) noexcept {
  if (!scalar_value(c))
    return fail(EILSEQ);
  return ::wcrtomb(s, static_cast<wchar_t>(c), __ps);
}

} // namespace

std::size_t __ycxx::__detail::__c_mbrtoc8(char8_t* __pc8, const char* s, std::size_t n, void* __ps,
                                    std::size_t size) noexcept {
  const state_ref __st(__ps, size, mbrtoc8_internal);
  if (!__st.has_tail())
    return fail(EINVAL);
  conv_tail t = __st.load();
  if (t.npending != 0) { // a further code unit of the last character
    if (__pc8 != nullptr)
      *__pc8 = t.pending[t.first];
    ++t.first;
    --t.npending;
    __st.store(t);
    return __stored;
  }
  if (s == nullptr) { // as mbrtoc8(NULL, "", 1, ps)
    s = "";
    n = 1;
    __pc8 = nullptr;
  }
  char32_t c = 0;
  const std::size_t r = next_char(s, n, __st.c_state(), c);
  if (r == error || r == incomplete)
    return r;
  unsigned char __u[4];
  int __len;
  if (c < 0x80) {
    __u[0] = static_cast<unsigned char>(c);
    __len = 1;
  } else if (c < 0x800) {
    __u[0] = static_cast<unsigned char>(0xC0 | (c >> 6));
    __len = 2;
  } else if (c < 0x10000) {
    __u[0] = static_cast<unsigned char>(0xE0 | (c >> 12));
    __len = 3;
  } else {
    __u[0] = static_cast<unsigned char>(0xF0 | (c >> 18));
    __len = 4;
  }
  for (int i = 1; i < __len; ++i)
    __u[i] = static_cast<unsigned char>(0x80 | ((c >> (6 * (__len - 1 - i))) & 0x3F));
  if (__pc8 != nullptr)
    *__pc8 = __u[0];
  t.first = 0;
  t.npending = static_cast<unsigned char>(__len - 1);
  for (int i = 1; i < __len; ++i)
    t.pending[i - 1] = __u[i];
  __st.store(t);
  return r;
}

std::size_t __ycxx::__detail::__c_c8rtomb(char* s, char8_t __c8, void* __ps, std::size_t size) noexcept {
  const state_ref __st(__ps, size, c8rtomb_internal);
  if (!__st.has_tail())
    return fail(EINVAL);
  char __buf[MB_LEN_MAX];
  if (s == nullptr) { // as c8rtomb(buf, u8'\0', ps)
    s = __buf;
    __c8 = 0;
  }
  conv_tail t = __st.load();
  const unsigned __u = __c8;
  if (t.__need == 0) {
    if (__u < 0x80)
      return put_char(s, __u, __st.c_state());
    if (__u >= 0xC2 && __u <= 0xDF) {
      t.length = 2;
      t.partial = __u & 0x1F;
    } else if (__u >= 0xE0 && __u <= 0xEF) {
      t.length = 3;
      t.partial = __u & 0x0F;
    } else if (__u >= 0xF0 && __u <= 0xF4) {
      t.length = 4;
      t.partial = __u & 0x07;
    } else {
      return fail(EILSEQ);
    }
    t.__need = static_cast<unsigned char>(t.length - 1);
    __st.store(t);
    return 0; // no character completed yet
  }
  if ((__u & 0xC0) != 0x80) {
    t.__need = 0;
    __st.store(t);
    return fail(EILSEQ);
  }
  t.partial = (t.partial << 6) | (__u & 0x3F);
  const bool done = --t.__need == 0;
  __st.store(t);
  if (!done)
    return 0;
  // The shortest form only (scalar values are checked by put_char).
  const char32_t least = t.length == 2 ? 0x80 : t.length == 3 ? 0x800 : 0x10000;
  if (t.partial < least)
    return fail(EILSEQ);
  return put_char(s, t.partial, __st.c_state());
}

std::size_t __ycxx::__detail::__c_mbrtoc16(char16_t* __pc16, const char* s, std::size_t n, void* __ps,
                                     std::size_t size) noexcept {
  const state_ref __st(__ps, size, mbrtoc16_internal);
  if (!__st.has_tail())
    return fail(EINVAL);
  conv_tail t = __st.load();
  if (t.__low != 0) { // the second unit of a surrogate pair
    if (__pc16 != nullptr)
      *__pc16 = t.__low;
    t.__low = 0;
    __st.store(t);
    return __stored;
  }
  if (s == nullptr) {
    s = "";
    n = 1;
    __pc16 = nullptr;
  }
  char32_t c = 0;
  const std::size_t r = next_char(s, n, __st.c_state(), c);
  if (r == error || r == incomplete)
    return r;
  if (c >= 0x10000) {
    c -= 0x10000;
    if (__pc16 != nullptr)
      *__pc16 = static_cast<char16_t>(0xD800 | (c >> 10));
    t.__low = static_cast<char16_t>(0xDC00 | (c & 0x3FF));
    __st.store(t);
  } else if (__pc16 != nullptr) {
    *__pc16 = static_cast<char16_t>(c);
  }
  return r;
}

std::size_t __ycxx::__detail::__c_c16rtomb(char* s, char16_t __c16, void* __ps, std::size_t size) noexcept {
  const state_ref __st(__ps, size, c16rtomb_internal);
  if (!__st.has_tail())
    return fail(EINVAL);
  char __buf[MB_LEN_MAX];
  if (s == nullptr) {
    s = __buf;
    __c16 = 0;
  }
  conv_tail t = __st.load();
  const bool is_high = __c16 >= 0xD800 && __c16 <= 0xDBFF, is_low = __c16 >= 0xDC00 && __c16 <= 0xDFFF;
  if (t.__high != 0) {
    const char16_t __high = t.__high;
    t.__high = 0;
    __st.store(t);
    if (!is_low)
      return fail(EILSEQ);
    return put_char(s, 0x10000 + ((char32_t(__high) - 0xD800) << 10) + (char32_t(__c16) - 0xDC00), __st.c_state());
  }
  if (is_high) {
    t.__high = __c16;
    __st.store(t);
    return 0;
  }
  if (is_low)
    return fail(EILSEQ);
  return put_char(s, __c16, __st.c_state());
}

std::size_t __ycxx::__detail::__c_mbrtoc32(char32_t* __pc32, const char* s, std::size_t n, void* __ps,
                                     std::size_t size) noexcept {
  const state_ref __st(__ps, size, mbrtoc32_internal);
  if (s == nullptr) {
    s = "";
    n = 1;
    __pc32 = nullptr;
  }
  char32_t c = 0;
  const std::size_t r = next_char(s, n, __st.c_state(), c);
  if (r != error && r != incomplete && __pc32 != nullptr)
    *__pc32 = c;
  return r;
}

std::size_t __ycxx::__detail::__c_c32rtomb(char* s, char32_t __c32, void* __ps, std::size_t size) noexcept {
  const state_ref __st(__ps, size, c32rtomb_internal);
  char __buf[MB_LEN_MAX];
  if (s == nullptr) {
    s = __buf;
    __c32 = 0;
  }
  return put_char(s, __c32, __st.c_state());
}
