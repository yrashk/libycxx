// libycxx hosted runtime: named locales on the C library's locales (DECISIONS §7).
//
// A name other than "C", "POSIX" and "C.UTF-8" is valid for a category when the C library's
// newlocale accepts it for that category. Each (name, category) pair the program uses is one
// named_locale: a locale_t holding that category and LC_CTYPE of the name (so strings can be
// converted in the name's own encoding), shared by every facet built from it, reference-counted
// (the facets hold the references), freed with the last of them. What the facets read from it is
// computed once: the ctype<char> table and case mappings when the LC_CTYPE entry is opened, the
// numpunct, moneypunct and time_get data (names, formats, eras, alternative digits) when such a
// facet is constructed. Nothing here changes
// the global C locale or another thread's: a C function without a _l form runs with the locale
// installed for the calling thread only (uselocale), and the thread's own locale is restored
// before returning, also when an exception passes.
//
// localeconv has no _l form in glibc and fills one static object, so it is called under a lock
// of this file (a program that calls localeconv itself in another thread at the same moment can
// still race with it; Darwin's localeconv_l has no such object and is used where it exists).
#include <locale>
#include <climits>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <typeinfo>
#include <ycxx/hosted/memory_resource.hpp> // __ycxx::__detail::__pal_lock
#include "locale_named.hpp"

#include <ctype.h>
#include <langinfo.h>
#include <locale.h>
#include <nl_types.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#if _YCXX_TARGET_DARWIN
// Darwin declares the _l functions in <xlocale.h>, for the headers included before it.
#  include <xlocale.h>
#endif

namespace {

constexpr int ctype_index = 1;
constexpr int c_masks[__ycxx::__detail::__locale_ncategories] = {LC_COLLATE_MASK, LC_CTYPE_MASK, LC_MONETARY_MASK,
                                                           LC_NUMERIC_MASK, LC_TIME_MASK, LC_MESSAGES_MASK};

__ycxx::__detail::__pal_lock cache_lock; // the list of open locales and their counts

struct lock_guard {
  explicit lock_guard(__ycxx::__detail::__pal_lock& __l) noexcept : l_(__l) { l_.lock(); }
  ~lock_guard() { l_.unlock(); }
  lock_guard(const lock_guard&) = delete;
  __ycxx::__detail::__pal_lock& l_;
};

// The calling thread's locale is loc while this object lives.
struct thread_locale {
  explicit thread_locale(locale_t __loc) noexcept : old_(::uselocale(__loc)) {}
  ~thread_locale() { ::uselocale(old_); }
  thread_locale(const thread_locale&) = delete;
  locale_t old_;
};

// Hosted std::mbstate_t is the C library's ::mbstate_t (DECISIONS §3), so a facet's state goes to
// mbrtowc/wcrtomb as it is.
static_assert(std::is_same_v<std::mbstate_t, ::mbstate_t>);

constexpr std::size_t mb_error = static_cast<std::size_t>(-1);
constexpr std::size_t mb_incomplete = static_cast<std::size_t>(-2);

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __named_locale {
  std::size_t __refs; // under cache_lock
  __named_locale* next;
  int category; // index
  std::string name;
  locale_t __loc;
  // LC_CTYPE only: ctype<char>'s table and case mappings, ctype<wchar_t>'s widen (btowc, WEOF for
  // a byte that is not a character by itself) and narrow (the pairs of widen, by wide value)
  std::ctype_base::mask table[256];
  unsigned char upper[256], lower[256];
  wchar_t widen[256];
  struct narrow_pair {
    wchar_t __w;
    unsigned char c;
  } narrow[256];
  int nnarrow;
  int mb_max; // MB_CUR_MAX
};

}} // namespace __ycxx::__detail

namespace {

using __ycxx::__detail::__named_locale;

__named_locale* open_list = nullptr; // under cache_lock

[[noreturn]] void bad_name(const char* what, const char* name) {
  std::string __msg(what);
  __msg += ": unsupported locale name \"";
  if (name != nullptr)
    __msg += name;
  __msg += '"';
  ::__ycxx::__detail::__throw_runtime_error(__msg.c_str());
}

// newlocale for category c (an index) of name, with the name's LC_CTYPE where it has one.
locale_t new_c_locale(const char* name, int c) noexcept {
  locale_t __l = ::newlocale(c_masks[c] | LC_CTYPE_MASK, name, static_cast<locale_t>(0));
  if (__l == static_cast<locale_t>(0))
    __l = ::newlocale(c_masks[c], name, static_cast<locale_t>(0));
  return __l;
}

void fill_ctype(__named_locale& h) {
  using _Bp = std::ctype_base;
  {
    thread_locale in(h.__loc);
    for (int c = 0; c < 256; ++c)
      h.widen[c] = static_cast<wchar_t>(::btowc(c));
    h.mb_max = static_cast<int>(MB_CUR_MAX);
  }
  for (int c = 0; c < 256; ++c) {
    _Bp::mask m = 0;
    // a byte that is not a character by itself (a UTF-8 lead or continuation byte) has no class
    // and no case: Darwin's is*_l and to*_l read such a byte as the code point of its value
    if (h.widen[c] == static_cast<wchar_t>(WEOF)) {
      h.table[c] = 0;
      h.upper[c] = h.lower[c] = static_cast<unsigned char>(c);
      continue;
    }
    if (isspace_l(c, h.__loc))
      m |= _Bp::space;
    if (isprint_l(c, h.__loc))
      m |= _Bp::print;
    if (iscntrl_l(c, h.__loc))
      m |= _Bp::cntrl;
    if (isupper_l(c, h.__loc))
      m |= _Bp::upper;
    if (islower_l(c, h.__loc))
      m |= _Bp::lower;
    if (isalpha_l(c, h.__loc))
      m |= _Bp::alpha;
    if (isdigit_l(c, h.__loc))
      m |= _Bp::digit;
    if (ispunct_l(c, h.__loc))
      m |= _Bp::punct;
    if (isxdigit_l(c, h.__loc))
      m |= _Bp::xdigit;
    if (isblank_l(c, h.__loc))
      m |= _Bp::blank;
    h.table[c] = m;
    h.upper[c] = static_cast<unsigned char>(toupper_l(c, h.__loc));
    h.lower[c] = static_cast<unsigned char>(tolower_l(c, h.__loc));
  }
  h.nnarrow = 0;
  for (int c = 0; c < 256; ++c)
    if (h.widen[c] != static_cast<wchar_t>(WEOF))
      h.narrow[h.nnarrow++] = {h.widen[c], static_cast<unsigned char>(c)};
  // by wide value; the smallest byte first among equal values (insertion sort: 256 entries once)
  for (int i = 1; i < h.nnarrow; ++i)
    for (int __j = i; __j > 0 && h.narrow[__j].__w < h.narrow[__j - 1].__w; --__j) {
      const auto t = h.narrow[__j];
      h.narrow[__j] = h.narrow[__j - 1];
      h.narrow[__j - 1] = t;
    }
}

// Releases a reference at the end of a scope.
struct named_ref {
  __named_locale* h;
  ~named_ref() { __ycxx::__detail::__named_release(h); }
};

// ---- localeconv --------------------------------------------------------------------------------

struct lconv_copy {
  std::string decimal_point, thousands_sep, grouping, int_curr_symbol, currency_symbol, mon_decimal_point,
      mon_thousands_sep, mon_grouping, positive_sign, negative_sign;
  char int_frac_digits, frac_digits, p_cs_precedes, p_sep_by_space, n_cs_precedes, n_sep_by_space, p_sign_posn,
      n_sign_posn, int_p_cs_precedes, int_p_sep_by_space, int_n_cs_precedes, int_n_sep_by_space, int_p_sign_posn,
      int_n_sign_posn;
};

void copy_lconv(const struct lconv& __l, lconv_copy& __o) {
  __o.decimal_point = __l.decimal_point;
  __o.thousands_sep = __l.thousands_sep;
  __o.grouping = __l.grouping;
  __o.int_curr_symbol = __l.int_curr_symbol;
  __o.currency_symbol = __l.currency_symbol;
  __o.mon_decimal_point = __l.mon_decimal_point;
  __o.mon_thousands_sep = __l.mon_thousands_sep;
  __o.mon_grouping = __l.mon_grouping;
  __o.positive_sign = __l.positive_sign;
  __o.negative_sign = __l.negative_sign;
  __o.int_frac_digits = __l.int_frac_digits;
  __o.frac_digits = __l.frac_digits;
  __o.p_cs_precedes = __l.p_cs_precedes;
  __o.p_sep_by_space = __l.p_sep_by_space;
  __o.n_cs_precedes = __l.n_cs_precedes;
  __o.n_sep_by_space = __l.n_sep_by_space;
  __o.p_sign_posn = __l.p_sign_posn;
  __o.n_sign_posn = __l.n_sign_posn;
  __o.int_p_cs_precedes = __l.int_p_cs_precedes;
  __o.int_p_sep_by_space = __l.int_p_sep_by_space;
  __o.int_n_cs_precedes = __l.int_n_cs_precedes;
  __o.int_n_sep_by_space = __l.int_n_sep_by_space;
  __o.int_p_sign_posn = __l.int_p_sign_posn;
  __o.int_n_sign_posn = __l.int_n_sign_posn;
}

// Whether the C library has localeconv_l (Darwin; found by argument-dependent lookup on locale_t,
// so no preprocessor test is needed).
template <class _Lp>
concept has_localeconv_l = requires(_Lp __l) { localeconv_l(__l); };

template <class _Lp>
void read_lconv(_Lp __loc, lconv_copy& __o) {
  if constexpr (has_localeconv_l<_Lp>) {
    copy_lconv(*localeconv_l(__loc), __o);
  } else {
    // localeconv's static object (only where there is no localeconv_l: an unused lock is an error
    // under -Werror with Clang on Darwin).
    static constinit __ycxx::__detail::__pal_lock lconv_lock;
    lock_guard __g(lconv_lock);
    thread_locale in(__loc);
    copy_lconv(*::localeconv(), __o);
  }
}

// ---- strings in the locale's encoding ------------------------------------------------------------

// s converted to wide characters by mbrtowc in loc (stops at an invalid sequence).
std::wstring to_wide(locale_t __loc, const char* s) {
  std::wstring r;
  thread_locale in(__loc);
  ::mbstate_t __st{};
  const char* end = s + std::strlen(s);
  while (s != end) {
    wchar_t wc;
    std::size_t n = ::mbrtowc(&wc, s, static_cast<std::size_t>(end - s), &__st);
    if (n == mb_error || n == mb_incomplete)
      break;
    if (n == 0)
      n = 1;
    r.push_back(wc);
    s += n;
  }
  return r;
}

bool space_like(locale_t __loc, wchar_t wc) noexcept {
  // the no-break spaces, which iswspace does not count, separate digit groups in many locales
  return wc == 0xA0 || wc == 0x2007 || wc == 0x202F || iswspace_l(static_cast<wint_t>(wc), __loc);
}

// A separator as one char: itself when it is one byte; ' ' for a space character of more bytes
// (fr_FR.UTF-8's U+202F); dflt when empty or any other character of more bytes.
char narrow_sep(locale_t __loc, const std::string& s, char __dflt) {
  if (s.size() == 1)
    return s[0];
  if (s.empty())
    return __dflt;
  const std::wstring __w = to_wide(__loc, s.c_str());
  return __w.size() == 1 && space_like(__loc, __w[0]) ? ' ' : __dflt;
}
wchar_t wide_sep(locale_t __loc, const std::string& s, wchar_t __dflt) {
  if (s.empty())
    return __dflt;
  const std::wstring __w = to_wide(__loc, s.c_str());
  return __w.empty() ? __dflt : __w[0];
}

std::string __convert(locale_t, const char* s, char) { return s; }
std::wstring __convert(locale_t __loc, const char* s, wchar_t) { return to_wide(__loc, s); }

template <class __charT>
__charT separator(locale_t __loc, const std::string& s, __charT __dflt) {
  if constexpr (std::is_same_v<__charT, char>)
    return narrow_sep(__loc, s, __dflt);
  else
    return wide_sep(__loc, s, __dflt);
}

// ---- numpunct, moneypunct ---------------------------------------------------------------------

template <class __charT>
void load_numpunct(const char* name, __charT& __point, __charT& __sep, std::string& grouping) {
  named_ref h{__ycxx::__detail::__named_open(name, std::locale::numeric, "std::numpunct_byname")};
  if (h.h == nullptr)
    return;
  lconv_copy __l;
  read_lconv(h.h->__loc, __l);
  __point = separator<__charT>(h.h->__loc, __l.decimal_point, __charT('.'));
  __sep = separator<__charT>(h.h->__loc, __l.thousands_sep, __charT(','));
  grouping = __l.thousands_sep.empty() ? std::string() : __l.grouping;
}

// The pattern of POSIX's cs_precedes, sep_by_space and sign_posn ([locale.moneypunct.general]/3:
// none never first, space neither first nor last). A separating space is a space field; where
// the format has no space, the none field marks where internal padding goes.
std::money_base::pattern money_pattern(char cs_precedes, char sep_by_space, char sign_posn,
                                       std::money_base::pattern __dflt) {
  using _Mp = std::money_base;
  if (cs_precedes == CHAR_MAX || sep_by_space == CHAR_MAX || sign_posn == CHAR_MAX || sep_by_space < 0 ||
      sep_by_space > 2 || sign_posn < 0 || sign_posn > 4)
    return __dflt;
  const char _Sp = _Mp::symbol, _Gp = _Mp::sign, _Vp = _Mp::value, gap = sep_by_space == 0 ? _Mp::none : _Mp::space;
  const char __sp = _Mp::space;
  const bool __two = sep_by_space == 2; // a space between symbol and sign when adjacent, else sign and value
  if (cs_precedes) {
    switch (sign_posn) {
    case 0: // parentheses around both: the sign string "()" before both
    case 1:
    case 3: // sign symbol value
      return __two ? _Mp::pattern{{_Gp, __sp, _Sp, _Vp}} : _Mp::pattern{{_Gp, _Sp, gap, _Vp}};
    case 2: // symbol value sign
      return __two ? _Mp::pattern{{_Sp, _Vp, __sp, _Gp}} : _Mp::pattern{{_Sp, gap, _Vp, _Gp}};
    default: // 4: symbol sign value
      return __two ? _Mp::pattern{{_Sp, __sp, _Gp, _Vp}} : _Mp::pattern{{_Sp, _Gp, gap, _Vp}};
    }
  }
  switch (sign_posn) {
  case 0:
  case 1: // sign value symbol
    return __two ? _Mp::pattern{{_Gp, __sp, _Vp, _Sp}} : _Mp::pattern{{_Gp, _Vp, gap, _Sp}};
  case 3: // value sign symbol
    return __two ? _Mp::pattern{{_Vp, _Gp, __sp, _Sp}} : _Mp::pattern{{_Vp, gap, _Gp, _Sp}};
  default: // 2, 4: value symbol sign
    return __two ? _Mp::pattern{{_Vp, _Sp, __sp, _Gp}} : _Mp::pattern{{_Vp, gap, _Sp, _Gp}};
  }
}

template <class __charT>
void load_money(const char* name, bool intl, __ycxx::__detail::__money_data<__charT>& d) {
  named_ref h{__ycxx::__detail::__named_open(name, std::locale::monetary, "std::moneypunct_byname")};
  if (h.h == nullptr)
    return;
  lconv_copy __l;
  read_lconv(h.h->__loc, __l);
  const locale_t __loc = h.h->__loc;
  d.__point = separator<__charT>(__loc, __l.mon_decimal_point, d.__point);
  d.__sep = separator<__charT>(__loc, __l.mon_thousands_sep, d.__sep);
  d.grouping = __l.mon_thousands_sep.empty() ? std::string() : __l.mon_grouping;
  d.symbol = __convert(__loc, intl ? __l.int_curr_symbol.c_str() : __l.currency_symbol.c_str(), __charT());
  d.__positive = __convert(__loc, __l.positive_sign.c_str(), __charT());
  d.__negative = __convert(__loc, __l.negative_sign.c_str(), __charT());
  const char __frac = intl ? __l.int_frac_digits : __l.frac_digits;
  d.frac_digits = __frac == CHAR_MAX || __frac < 0 ? 0 : __frac;
  const char __pcs = intl ? __l.int_p_cs_precedes : __l.p_cs_precedes, psep = intl ? __l.int_p_sep_by_space : __l.p_sep_by_space,
             ppos = intl ? __l.int_p_sign_posn : __l.p_sign_posn, ncs = intl ? __l.int_n_cs_precedes : __l.n_cs_precedes,
             nsep = intl ? __l.int_n_sep_by_space : __l.n_sep_by_space, npos = intl ? __l.int_n_sign_posn : __l.n_sign_posn;
  // C's int_curr_symbol ends with the character that separates it from the value (C23
  // 7.11.2.1): when it precedes the value, that character is the separation sep_by_space 1 asks
  // for, and a space field as well would double it ("USD  1.00")
  const bool own_sep = intl && __l.int_curr_symbol.size() == 4 && __l.int_curr_symbol[3] == ' ';
  auto __sep = [&](char __cs, char __sp) { return own_sep && __cs == 1 && __sp == 1 ? char(0) : __sp; };
  d.__pos = money_pattern(__pcs, __sep(__pcs, psep), ppos, d.__pos);
  d.__neg = money_pattern(ncs, __sep(ncs, nsep), npos, d.__neg);
  if (ppos == 0)
    d.__positive = __convert(__loc, "()", __charT());
  if (npos == 0)
    d.__negative = __convert(__loc, "()", __charT());
}

// ---- time ----------------------------------------------------------------------------------------

std::time_base::dateorder order_of(const char* __fmt) noexcept {
  char __seen[3];
  int n = 0;
  for (const char* p = __fmt; *p && n < 3; ++p) {
    if (*p != '%' || p[1] == '\0')
      continue;
    ++p;
    if ((*p == 'E' || *p == 'O') && p[1] != '\0')
      ++p;
    char k = 0;
    switch (*p) {
    case 'd':
    case 'e':
      k = 'd';
      break;
    case 'm':
    case 'b':
    case 'B':
    case 'h':
      k = 'm';
      break;
    case 'y':
    case 'Y':
      k = 'y';
      break;
    case 'D': // %m/%d/%y
      return std::time_base::mdy;
    case 'F': // %Y-%m-%d
      return std::time_base::ymd;
    }
    if (k != 0)
      __seen[n++] = k;
  }
  if (n != 3)
    return std::time_base::no_order;
  if (__seen[0] == 'd' && __seen[1] == 'm' && __seen[2] == 'y')
    return std::time_base::dmy;
  if (__seen[0] == 'm' && __seen[1] == 'd' && __seen[2] == 'y')
    return std::time_base::mdy;
  if (__seen[0] == 'y' && __seen[1] == 'm' && __seen[2] == 'd')
    return std::time_base::ymd;
  if (__seen[0] == 'y' && __seen[1] == 'd' && __seen[2] == 'm')
    return std::time_base::ydm;
  return std::time_base::no_order;
}

// A date "[-]yyyy/mm/dd" of an era segment, as a comparable number (yyyy * 10000 + mm * 100 + dd);
// its year in y.
bool era_date(std::string_view s, long long& __key, long long& y) noexcept {
  bool __neg = false;
  if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
    __neg = s[0] == '-';
    s.remove_prefix(1);
  }
  long long __part[3] = {0, 0, 0};
  int k = 0, digits = 0;
  for (char c : s) {
    if (c == '/' && k < 2 && digits != 0) {
      ++k;
      digits = 0;
    } else if (c >= '0' && c <= '9' && digits < 9) {
      __part[k] = __part[k] * 10 + (c - '0');
      ++digits;
    } else {
      return false;
    }
  }
  if (k != 2 || digits == 0 || __part[1] < 1 || __part[1] > 12 || __part[2] < 1 || __part[2] > 31)
    return false;
  y = __neg ? -__part[0] : __part[0];
  __key = y * 10000 + __part[1] * 100 + __part[2];
  return true;
}

// One era segment "direction:offset:start_date:end_date:era_name:era_format" (POSIX, LC_TIME
// era); false (d unchanged) when s does not have that form.
template <class __charT>
bool add_era(locale_t __loc, std::string_view s, __ycxx::__detail::__time_data<__charT>& d) {
  std::string_view f[6];
  for (int k = 0; k < 5; ++k) {
    const std::size_t c = s.find(':');
    if (c == std::string_view::npos)
      return false;
    f[k] = s.substr(0, c);
    s.remove_prefix(c + 1);
  }
  f[5] = s;
  if (f[0].size() != 1 || (f[0][0] != '+' && f[0][0] != '-') || f[1].empty() || f[4].empty())
    return false;
  long long __offset = 0;
  for (char c : f[1]) {
    if (c < '0' || c > '9' || __offset > 1'000'000)
      return false;
    __offset = __offset * 10 + (c - '0');
  }
  long long __start_key, __start_year, __end_key, __end_year;
  if (!era_date(f[2], __start_key, __start_year))
    return false;
  bool __later; // the end date follows the start date
  if (f[3] == "+*")
    __later = true;
  else if (f[3] == "-*")
    __later = false;
  else if (era_date(f[3], __end_key, __end_year))
    __later = __end_key >= __start_key;
  else
    return false;
  if (d.__neras == static_cast<int>(sizeof d.__eras / sizeof d.__eras[0]))
    return false;
  // '+': the numbers grow from the start date towards the end date; '-': they shrink
  const int __step = (f[0][0] == '+') == __later ? 1 : -1;
  const std::string __name(f[4]), __fmt(f[5]);
  const std::basic_string<__charT> __n = __convert(__loc, __name.c_str(), __charT());
  const std::basic_string<__charT> __ft = __convert(__loc, __fmt.c_str(), __charT());
  __ycxx::__detail::__time_era& e = d.__eras[d.__neras++];
  e = {__start_year, __offset, __step, d.__era_text.size(), __n.size(), d.__era_text.size() + __n.size(), __ft.size()};
  d.__era_text += __n;
  d.__era_text += __ft;
  return true;
}

// The eras of LC_TIME (nl_langinfo ERA). POSIX gives the segments separated by ';'; glibc
// separates them by NULs and gives their count as the item _NL_TIME_ERA_NUM_ENTRIES
// (_YCXX_C_HAS_ERA_NUM_ENTRIES, found by cmake/ycxx-c-library.cmake).
template <class __charT>
void load_eras(locale_t __loc, __ycxx::__detail::__time_data<__charT>& d) {
  const char* p = ::nl_langinfo_l(ERA, __loc);
  if (p == nullptr || *p == '\0')
    return;
#if _YCXX_C_HAS_ERA_NUM_ENTRIES
  const auto __count = reinterpret_cast<std::uintptr_t>(::nl_langinfo_l(_NL_TIME_ERA_NUM_ENTRIES, __loc));
  if (__count != 0) {
    for (std::uintptr_t k = 0; k < __count && k < 64; ++k) {
      const std::string_view s(p);
      if (!add_era(__loc, s, d))
        return;
      p += s.size() + 1;
    }
    return;
  }
#endif
  std::string_view s(p);
  while (!s.empty()) {
    const std::size_t __semi = s.find(';');
    if (!add_era(__loc, s.substr(0, __semi), d) || __semi == std::string_view::npos)
      return;
    s.remove_prefix(__semi + 1);
  }
}

// The alternative digits of 0-99, as the C library writes them: strftime_l("%Oy") of the years
// 1900-1999 (so how the C library stores ALT_DIGITS does not matter). None when every one is the
// decimal form.
template <class __charT>
void load_alt_digits(locale_t __loc, __ycxx::__detail::__time_data<__charT>& d) {
  std::basic_string<__charT> __text;
  unsigned short __pos[101];
  bool __own = false;
  for (int k = 0; k < 100; ++k) {
    std::tm t{};
    t.tm_year = k;
    t.tm_mday = 1;
    char __buf[64];
    const std::size_t n = ::strftime_l(__buf, sizeof __buf, "%Oy", &t, __loc);
    __buf[n < sizeof __buf ? n : 0] = '\0';
    const char __dec[3] = {static_cast<char>('0' + k / 10), static_cast<char>('0' + k % 10), '\0'};
    if (n == 0 || std::strcmp(__buf, __dec) != 0)
      __own = true;
    __pos[k] = static_cast<unsigned short>(__text.size());
    __text += __convert(__loc, __buf, __charT());
    if (__text.size() > 60000)
      return;
  }
  if (!__own)
    return;
  __pos[100] = static_cast<unsigned short>(__text.size());
  d.__alt_text = static_cast<std::basic_string<__charT>&&>(__text);
  for (int k = 0; k <= 100; ++k)
    d.__alt_pos[k] = __pos[k];
}

template <class __charT>
bool load_time(const char* name, __ycxx::__detail::__time_data<__charT>& d) {
  named_ref h{__ycxx::__detail::__named_open(name, std::locale::time, "std::time_get_byname")};
  if (h.h == nullptr)
    return false;
  const locale_t __loc = h.h->__loc;
  static constexpr nl_item items[40] = {
      DAY_1,  DAY_2,  DAY_3,  DAY_4,  DAY_5,   DAY_6,   DAY_7,   ABDAY_1, ABDAY_2, ABDAY_3,
      ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7, MON_1,  MON_2,   MON_3,   MON_4,   MON_5,   MON_6,
      MON_7,  MON_8,  MON_9,  MON_10, MON_11,  MON_12,  ABMON_1, ABMON_2, ABMON_3, ABMON_4,
      ABMON_5, ABMON_6, ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12, AM_STR, PM_STR};
  for (int i = 0; i < 40; ++i)
    d.__names[i] = __convert(__loc, ::nl_langinfo_l(items[i], __loc), __charT());
  d.__d_t_fmt = __convert(__loc, ::nl_langinfo_l(D_T_FMT, __loc), __charT());
  const char* __x = ::nl_langinfo_l(D_FMT, __loc);
  d.__d_fmt = __convert(__loc, __x, __charT());
  d.__t_fmt = __convert(__loc, ::nl_langinfo_l(T_FMT, __loc), __charT());
  d.__t_fmt_ampm = __convert(__loc, ::nl_langinfo_l(T_FMT_AMPM, __loc), __charT());
  d.__order = order_of(__x);
  d.__era_d_t_fmt = __convert(__loc, ::nl_langinfo_l(ERA_D_T_FMT, __loc), __charT());
  d.__era_d_fmt = __convert(__loc, ::nl_langinfo_l(ERA_D_FMT, __loc), __charT());
  d.__era_t_fmt = __convert(__loc, ::nl_langinfo_l(ERA_T_FMT, __loc), __charT());
  load_eras(__loc, d);
  load_alt_digits(__loc, d);
  return true;
}

// strftime_l / wcsftime_l of one conversion; the length of the result (once more in 1024 if it
// does not fit in cap:
// both return 0 both for an empty result and for one that does not fit; one conversion of a C
// library locale is far shorter than 1024 characters, so 0 there means empty).
template <class __charT>
std::size_t put_time(locale_t __loc, __charT* __buf, std::size_t __cap, const std::tm* t, char format, char __modifier) {
  __charT __fmt[4] = {__charT('%')};
  int k = 1;
  if (__modifier != 0)
    __fmt[k++] = __charT(__modifier);
  __fmt[k++] = __charT(static_cast<unsigned char>(format));
  __fmt[k] = __charT();
  auto __call = [&](__charT* to, std::size_t n) -> std::size_t {
    if constexpr (std::is_same_v<__charT, char>)
      return ::strftime_l(to, n, __fmt, t, __loc);
    else
      return ::wcsftime_l(to, n, __fmt, t, __loc);
  };
  if (__cap > 1) {
    const std::size_t n = __call(__buf, __cap);
    if (n != 0)
      return n;
  }
  if (__cap >= 1024)
    return 0;
  __charT big[1024];
  const std::size_t n = __call(big, 1024);
  for (std::size_t i = 0; i < n && i < __cap; ++i)
    __buf[i] = big[i];
  return n;
}

// ---- collate ------------------------------------------------------------------------------------

// The runs of [low, high) between embedded null characters, compared run by run.
template <class __charT, class Coll>
int compare_runs(const __charT* __low1, const __charT* __high1, const __charT* __low2, const __charT* __high2, Coll coll) {
  const std::basic_string<__charT> a(__low1, __high1), b(__low2, __high2);
  const __charT *p = a.c_str(), *__pe = p + a.size(), *__q = b.c_str(), *qe = __q + b.size();
  for (;;) {
    const int r = coll(p, __q);
    if (r != 0)
      return r < 0 ? -1 : 1;
    p += std::char_traits<__charT>::length(p);
    __q += std::char_traits<__charT>::length(__q);
    if (p == __pe || __q == qe)
      return p == __pe ? (__q == qe ? 0 : -1) : 1;
    ++p;
    ++__q;
  }
}

template <class __charT, class Xfrm>
std::basic_string<__charT> transform_runs(const __charT* __low, const __charT* __high, Xfrm xfrm) {
  const std::basic_string<__charT> a(__low, __high);
  const __charT *p = a.c_str(), *__pe = p + a.size();
  std::basic_string<__charT> r;
  for (;;) {
    std::size_t __have = r.size();
    std::size_t n = 2 * std::char_traits<__charT>::length(p) + 16;
    for (;;) {
      r.resize(__have + n);
      const std::size_t m = xfrm(r.data() + __have, p, n);
      if (m < n) {
        r.resize(__have + m);
        break;
      }
      n = m + 1;
    }
    p += std::char_traits<__charT>::length(p);
    if (p == __pe)
      return r;
    r.push_back(__charT()); // keeps the runs apart, ordered before any key character
    ++p;
  }
}

template <class __charT>
long __hash_of(const std::basic_string<__charT>& s) noexcept {
  unsigned long h = 14695981039346656037ul; // FNV-1a over the key
  for (__charT c : s) {
    h ^= static_cast<unsigned long>(static_cast<std::make_unsigned_t<__charT>>(c));
    h *= 1099511628211ul;
  }
  return static_cast<long>(h);
}

// ---- messages -------------------------------------------------------------------------------------

// The open catalogs: messages_base::catalog is an index into this table (under cache_lock).
struct catalog_table {
  nl_catd* d = nullptr;
  int n = 0;
};
catalog_table catalogs;
const nl_catd no_catalog = reinterpret_cast<nl_catd>(-1);

int catalog_add(nl_catd c) {
  lock_guard __g(cache_lock);
  for (int i = 0; i < catalogs.n; ++i)
    if (catalogs.d[i] == no_catalog) {
      catalogs.d[i] = c;
      return i;
    }
  const int n = catalogs.n == 0 ? 8 : 2 * catalogs.n;
  nl_catd* d = static_cast<nl_catd*>(::operator new(sizeof(nl_catd) * static_cast<std::size_t>(n), std::nothrow));
  if (d == nullptr)
    return -1;
  for (int i = 0; i < n; ++i)
    d[i] = i < catalogs.n ? catalogs.d[i] : no_catalog;
  ::operator delete(catalogs.d);
  catalogs.d = d;
  const int k = catalogs.n;
  catalogs.n = n;
  catalogs.d[k] = c;
  return k;
}
nl_catd catalog_get(int c) {
  lock_guard __g(cache_lock);
  return c >= 0 && c < catalogs.n ? catalogs.d[c] : no_catalog;
}
nl_catd catalog_remove(int c) {
  lock_guard __g(cache_lock);
  if (c < 0 || c >= catalogs.n)
    return no_catalog;
  const nl_catd d = catalogs.d[c];
  catalogs.d[c] = no_catalog;
  return d;
}

int open_catalog(const __named_locale* h, const std::string& __fn) {
  nl_catd d;
  {
    thread_locale in(h->__loc); // catopen's NL_CAT_LOCALE reads the thread's LC_MESSAGES
    d = ::catopen(__fn.c_str(), NL_CAT_LOCALE);
  }
  if (d == no_catalog)
    return -1;
  const int k = catalog_add(d);
  if (k < 0)
    ::catclose(d);
  return k;
}

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

__named_locale* __named_open(const char* name, int cat, const char* what) {
  if (name == nullptr)
    bad_name(what, name);
  const int c = __category_index(cat);
  std::string part;
  if (!__locale_name_part(name, c, part))
    bad_name(what, name);
  if (__classic_locale_name(part.c_str()) != nullptr)
    return nullptr;
  {
    lock_guard __g(cache_lock);
    for (__named_locale* p = open_list; p != nullptr; p = p->next)
      if (p->category == c && p->name == part) {
        ++p->__refs;
        return p;
      }
  }
  const locale_t __loc = new_c_locale(part.c_str(), c);
  if (__loc == static_cast<locale_t>(0))
    bad_name(what, part.c_str());
  __named_locale* h;
  if constexpr (__cfg::exceptions) {
    try {
      h = new __named_locale{1, nullptr, c, part, __loc, {}, {}, {}, {}, {}, 0, 1};
    } catch (...) {
      ::freelocale(__loc);
      throw;
    }
  } else {
    h = new __named_locale{1, nullptr, c, part, __loc, {}, {}, {}, {}, {}, 0, 1};
  }
  if (c == ctype_index)
    fill_ctype(*h); // does not throw
  // another thread may have opened the same one meanwhile: keep the first
  __named_locale* __mine = h;
  {
    lock_guard __g(cache_lock);
    for (__named_locale* p = open_list; p != nullptr; p = p->next)
      if (p->category == c && p->name == part) {
        ++p->__refs;
        h = p;
        break;
      }
    if (h == __mine) {
      h->next = open_list;
      open_list = h;
    }
  }
  if (h != __mine) {
    ::freelocale(__mine->__loc);
    delete __mine;
  }
  return h;
}

void __named_release(__named_locale* h) noexcept {
  if (h == nullptr)
    return;
  {
    lock_guard __g(cache_lock);
    if (--h->__refs != 0)
      return;
    for (__named_locale** p = &open_list; *p != nullptr; p = &(*p)->next)
      if (*p == h) {
        *p = h->next;
        break;
      }
  }
  ::freelocale(h->__loc);
  delete h;
}

const std::ctype_base::mask* __named_ctype_table(const __named_locale* h) noexcept { return h ? h->table : nullptr; }
const unsigned char* __named_toupper_table(const __named_locale* h) noexcept { return h ? h->upper : nullptr; }
const unsigned char* __named_tolower_table(const __named_locale* h) noexcept { return h ? h->lower : nullptr; }

void __named_numpunct(const char* name, char& __point, char& __sep, std::string& grouping) {
  load_numpunct(name, __point, __sep, grouping);
}
void __named_numpunct(const char* name, wchar_t& __point, wchar_t& __sep, std::string& grouping) {
  load_numpunct(name, __point, __sep, grouping);
}
void __named_money_data(const char* name, bool intl, __money_data<char>& d) { load_money(name, intl, d); }
void __named_money_data(const char* name, bool intl, __money_data<wchar_t>& d) { load_money(name, intl, d); }
bool __named_time_data(const char* name, __time_data<char>& d) { return load_time(name, d); }
bool __named_time_data(const char* name, __time_data<wchar_t>& d) { return load_time(name, d); }
std::size_t __named_strftime(const __named_locale* h, char* __buf, std::size_t __cap, const std::tm* t, char format,
                           char __modifier) {
  return put_time(h->__loc, __buf, __cap, t, format, __modifier);
}
std::size_t __named_strftime(const __named_locale* h, wchar_t* __buf, std::size_t __cap, const std::tm* t, char format,
                           char __modifier) {
  return put_time(h->__loc, __buf, __cap, t, format, __modifier);
}

bool __named_exists(const char* name, int c) {
  const locale_t __l = ::newlocale(c_masks[c], name, static_cast<locale_t>(0));
  if (__l == static_cast<locale_t>(0))
    return false;
  ::freelocale(__l);
  return true;
}

std::string __named_codeset(const char* name) {
  const locale_t __l = ::newlocale(LC_CTYPE_MASK, name, static_cast<locale_t>(0));
  if (__l == static_cast<locale_t>(0))
    return std::string();
  std::string r;
  if constexpr (__cfg::exceptions) {
    try {
      r = ::nl_langinfo_l(CODESET, __l);
    } catch (...) {
      ::freelocale(__l);
      throw;
    }
  } else {
    r = ::nl_langinfo_l(CODESET, __l);
  }
  ::freelocale(__l);
  return r;
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- ctype_byname<wchar_t> -----------------------------------------------------------------------

ctype_byname<wchar_t>::~ctype_byname() { ::__ycxx::__detail::__named_release(__named_); }

namespace {
ctype_base::mask wide_mask(locale_t __loc, wchar_t c) noexcept {
  const wint_t __w = static_cast<wint_t>(c);
  ctype_base::mask m = 0;
  if (iswspace_l(__w, __loc))
    m |= ctype_base::space;
  if (iswprint_l(__w, __loc))
    m |= ctype_base::print;
  if (iswcntrl_l(__w, __loc))
    m |= ctype_base::cntrl;
  if (iswupper_l(__w, __loc))
    m |= ctype_base::upper;
  if (iswlower_l(__w, __loc))
    m |= ctype_base::lower;
  if (iswalpha_l(__w, __loc))
    m |= ctype_base::alpha;
  if (iswdigit_l(__w, __loc))
    m |= ctype_base::digit;
  if (iswpunct_l(__w, __loc))
    m |= ctype_base::punct;
  if (iswxdigit_l(__w, __loc))
    m |= ctype_base::xdigit;
  if (iswblank_l(__w, __loc))
    m |= ctype_base::blank;
  return m;
}
} // namespace

bool ctype_byname<wchar_t>::do_is(mask m, wchar_t c) const {
  if (__named_ == nullptr)
    return ctype<wchar_t>::do_is(m, c);
  return (wide_mask(__named_->__loc, c) & m) != 0;
}
const wchar_t* ctype_byname<wchar_t>::do_is(const wchar_t* __low, const wchar_t* __high, mask* vec) const {
  if (__named_ == nullptr)
    return ctype<wchar_t>::do_is(__low, __high, vec);
  for (; __low != __high; ++__low, ++vec)
    *vec = wide_mask(__named_->__loc, *__low);
  return __high;
}
wchar_t ctype_byname<wchar_t>::do_toupper(wchar_t c) const {
  if (__named_ == nullptr)
    return ctype<wchar_t>::do_toupper(c);
  return static_cast<wchar_t>(towupper_l(static_cast<wint_t>(c), __named_->__loc));
}
const wchar_t* ctype_byname<wchar_t>::do_toupper(wchar_t* __low, const wchar_t* __high) const {
  for (; __low != __high; ++__low)
    *__low = do_toupper(*__low);
  return __high;
}
wchar_t ctype_byname<wchar_t>::do_tolower(wchar_t c) const {
  if (__named_ == nullptr)
    return ctype<wchar_t>::do_tolower(c);
  return static_cast<wchar_t>(towlower_l(static_cast<wint_t>(c), __named_->__loc));
}
const wchar_t* ctype_byname<wchar_t>::do_tolower(wchar_t* __low, const wchar_t* __high) const {
  for (; __low != __high; ++__low)
    *__low = do_tolower(*__low);
  return __high;
}
wchar_t ctype_byname<wchar_t>::do_widen(char c) const {
  if (__named_ == nullptr)
    return ctype<wchar_t>::do_widen(c);
  return __named_->widen[static_cast<unsigned char>(c)];
}
const char* ctype_byname<wchar_t>::do_widen(const char* __low, const char* __high, wchar_t* __dest) const {
  for (; __low != __high; ++__low, ++__dest)
    *__dest = do_widen(*__low);
  return __high;
}
char ctype_byname<wchar_t>::do_narrow(wchar_t c, char __dfault) const {
  if (__named_ == nullptr)
    return ctype<wchar_t>::do_narrow(c, __dfault);
  // wctob: the byte whose btowc is c
  int __lo = 0, __hi = __named_->nnarrow;
  while (__lo < __hi) {
    const int __mid = __lo + (__hi - __lo) / 2;
    if (__named_->narrow[__mid].__w < c)
      __lo = __mid + 1;
    else
      __hi = __mid;
  }
  return __lo < __named_->nnarrow && __named_->narrow[__lo].__w == c ? static_cast<char>(__named_->narrow[__lo].c) : __dfault;
}
const wchar_t* ctype_byname<wchar_t>::do_narrow(const wchar_t* __low, const wchar_t* __high, char __dfault,
                                                char* __dest) const {
  for (; __low != __high; ++__low, ++__dest)
    *__dest = do_narrow(*__low, __dfault);
  return __high;
}

// ---- codecvt_byname<wchar_t, char, mbstate_t> ------------------------------------------------------
// One character at a time through a copy of the state, so a character that does not fit, or an
// incomplete sequence at the end of the input, leaves the state as it was before it.

codecvt_byname<wchar_t, char, mbstate_t>::~codecvt_byname() { ::__ycxx::__detail::__named_release(__named_); }

codecvt_base::result codecvt_byname<wchar_t, char, mbstate_t>::do_out(mbstate_t& state, const wchar_t* from,
                                                                      const wchar_t* __from_end,
                                                                      const wchar_t*& __from_next, char* to,
                                                                      char* __to_end, char*& __to_next) const {
  if (__named_ == nullptr)
    return codecvt::do_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
  thread_locale in(__named_->__loc);
  mbstate_t* __st = &state;
  result r = ok;
  for (; from != __from_end; ++from) {
    char __buf[MB_LEN_MAX];
    ::mbstate_t __tmp = *__st;
    const std::size_t n = ::wcrtomb(__buf, *from, &__tmp);
    if (n == mb_error) {
      r = error;
      break;
    }
    if (n > static_cast<std::size_t>(__to_end - to)) {
      r = partial;
      break;
    }
    std::memcpy(to, __buf, n);
    to += n;
    *__st = __tmp;
  }
  __from_next = from;
  __to_next = to;
  return r;
}

codecvt_base::result codecvt_byname<wchar_t, char, mbstate_t>::do_in(mbstate_t& state, const char* from,
                                                                     const char* __from_end, const char*& __from_next,
                                                                     wchar_t* to, wchar_t* __to_end,
                                                                     wchar_t*& __to_next) const {
  if (__named_ == nullptr)
    return codecvt::do_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
  thread_locale in(__named_->__loc);
  mbstate_t* __st = &state;
  result r = ok;
  while (from != __from_end) {
    if (to == __to_end) {
      r = partial;
      break;
    }
    ::mbstate_t __tmp = *__st;
    wchar_t wc;
    std::size_t n = ::mbrtowc(&wc, from, static_cast<std::size_t>(__from_end - from), &__tmp);
    if (n == mb_error) {
      r = error;
      break;
    }
    if (n == mb_incomplete) {
      r = partial;
      break;
    }
    if (n == 0) // the null character
      n = 1;
    *to++ = wc;
    from += n;
    *__st = __tmp;
  }
  __from_next = from;
  __to_next = to;
  return r;
}

codecvt_base::result codecvt_byname<wchar_t, char, mbstate_t>::do_unshift(mbstate_t& state, char* to, char* __to_end,
                                                                          char*& __to_next) const {
  if (__named_ == nullptr)
    return codecvt::do_unshift(state, to, __to_end, __to_next);
  __to_next = to;
  thread_locale in(__named_->__loc);
  mbstate_t* __st = &state;
  char __buf[MB_LEN_MAX];
  ::mbstate_t __tmp = *__st;
  std::size_t n = ::wcrtomb(__buf, L'\0', &__tmp); // the shift sequence, then the null character
  if (n == mb_error)
    return error;
  if (--n == 0)
    return noconv;
  if (n > static_cast<std::size_t>(__to_end - to))
    return partial;
  std::memcpy(to, __buf, n);
  __to_next = to + n;
  *__st = __tmp;
  return ok;
}

// A state-dependent encoding (-1) is not detected: the C library's only probe for it, mbtowc
// with a null string, resets an internal state shared by every thread.
int codecvt_byname<wchar_t, char, mbstate_t>::do_encoding() const noexcept {
  if (__named_ == nullptr)
    return codecvt::do_encoding();
  return __named_->mb_max == 1 ? 1 : 0;
}
bool codecvt_byname<wchar_t, char, mbstate_t>::do_always_noconv() const noexcept { return false; }

int codecvt_byname<wchar_t, char, mbstate_t>::do_length(mbstate_t& state, const char* from, const char* end,
                                                        size_t max) const {
  if (__named_ == nullptr)
    return codecvt::do_length(state, from, end, max);
  thread_locale in(__named_->__loc);
  mbstate_t* __st = &state;
  const char* p = from;
  for (; p != end && max != 0; --max) {
    ::mbstate_t __tmp = *__st;
    wchar_t wc;
    std::size_t n = ::mbrtowc(&wc, p, static_cast<std::size_t>(end - p), &__tmp);
    if (n == mb_error || n == mb_incomplete)
      break;
    if (n == 0)
      n = 1;
    p += n;
    *__st = __tmp;
  }
  return static_cast<int>(p - from);
}

int codecvt_byname<wchar_t, char, mbstate_t>::do_max_length() const noexcept {
  if (__named_ == nullptr)
    return codecvt::do_max_length();
  return __named_->mb_max;
}

// ---- collate_byname --------------------------------------------------------------------------------

collate_byname<char>::~collate_byname() { ::__ycxx::__detail::__named_release(__named_); }
int collate_byname<char>::do_compare(const char* __low1, const char* __high1, const char* __low2, const char* __high2) const {
  if (__named_ == nullptr)
    return collate::do_compare(__low1, __high1, __low2, __high2);
  const locale_t __loc = __named_->__loc;
  return compare_runs(__low1, __high1, __low2, __high2, [__loc](const char* a, const char* b) { return ::strcoll_l(a, b, __loc); });
}
string collate_byname<char>::do_transform(const char* __low, const char* __high) const {
  if (__named_ == nullptr)
    return collate::do_transform(__low, __high);
  const locale_t __loc = __named_->__loc;
  return transform_runs(__low, __high,
                        [__loc](char* to, const char* s, size_t n) { return ::strxfrm_l(to, s, n, __loc); });
}
long collate_byname<char>::do_hash(const char* __low, const char* __high) const {
  if (__named_ == nullptr)
    return collate::do_hash(__low, __high);
  return __hash_of(do_transform(__low, __high));
}

collate_byname<wchar_t>::~collate_byname() { ::__ycxx::__detail::__named_release(__named_); }
int collate_byname<wchar_t>::do_compare(const wchar_t* __low1, const wchar_t* __high1, const wchar_t* __low2,
                                        const wchar_t* __high2) const {
  if (__named_ == nullptr)
    return collate::do_compare(__low1, __high1, __low2, __high2);
  const locale_t __loc = __named_->__loc;
  return compare_runs(__low1, __high1, __low2, __high2,
                      [__loc](const wchar_t* a, const wchar_t* b) { return ::wcscoll_l(a, b, __loc); });
}
wstring collate_byname<wchar_t>::do_transform(const wchar_t* __low, const wchar_t* __high) const {
  if (__named_ == nullptr)
    return collate::do_transform(__low, __high);
  const locale_t __loc = __named_->__loc;
  return transform_runs(__low, __high,
                        [__loc](wchar_t* to, const wchar_t* s, size_t n) { return ::wcsxfrm_l(to, s, n, __loc); });
}
long collate_byname<wchar_t>::do_hash(const wchar_t* __low, const wchar_t* __high) const {
  if (__named_ == nullptr)
    return collate::do_hash(__low, __high);
  return __hash_of(do_transform(__low, __high));
}

// ---- messages_byname ---------------------------------------------------------------------------------

messages_byname<char>::~messages_byname() { ::__ycxx::__detail::__named_release(__named_); }
messages_base::catalog messages_byname<char>::do_open(const string& __fn, const locale& __loc) const {
  if (__named_ == nullptr)
    return messages::do_open(__fn, __loc);
  return open_catalog(__named_, __fn);
}
string messages_byname<char>::do_get(catalog c, int set, int __msgid, const string& __dfault) const {
  if (__named_ == nullptr)
    return messages::do_get(c, set, __msgid, __dfault);
  const nl_catd d = catalog_get(c);
  if (d == no_catalog)
    return __dfault;
  const char* s = ::catgets(d, set, __msgid, nullptr);
  return s != nullptr ? string(s) : __dfault;
}
void messages_byname<char>::do_close(catalog c) const {
  if (__named_ == nullptr)
    return messages::do_close(c);
  const nl_catd d = catalog_remove(c);
  if (d != no_catalog)
    ::catclose(d);
}

messages_byname<wchar_t>::~messages_byname() { ::__ycxx::__detail::__named_release(__named_); }
messages_base::catalog messages_byname<wchar_t>::do_open(const string& __fn, const locale& __loc) const {
  if (__named_ == nullptr)
    return messages::do_open(__fn, __loc);
  return open_catalog(__named_, __fn);
}
wstring messages_byname<wchar_t>::do_get(catalog c, int set, int __msgid, const wstring& __dfault) const {
  if (__named_ == nullptr)
    return messages::do_get(c, set, __msgid, __dfault);
  const nl_catd d = catalog_get(c);
  if (d == no_catalog)
    return __dfault;
  const char* s = ::catgets(d, set, __msgid, nullptr);
  return s != nullptr ? to_wide(__named_->__loc, s) : __dfault;
}
void messages_byname<wchar_t>::do_close(catalog c) const {
  if (__named_ == nullptr)
    return messages::do_close(c);
  const nl_catd d = catalog_remove(c);
  if (d != no_catalog)
    ::catclose(d);
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// regex_traits::transform_primary ([re.traits]/7): the primary sort key when the facet is exactly
// a collate_byname and the form of its keys is known. glibc's strxfrm_l/wcsxfrm_l key of a locale
// with collation rules is the weights of each level in turn, each level ended by the value 1
// (glibc's string/strxfrm_l.c); the primary key is the weights before the first 1. A locale without rules ("C", or every
// locale of musl) gives a copy of the string, which has no separator: every character is then its
// own equivalence class, and the full key is the primary one too, which the caller uses when this
// returns false. Darwin's key form is not documented: false there as well.
template <class __charT>
static bool primary_key(const std::collate<__charT>& __f, const __charT* __low, const __charT* __high,
                        std::basic_string<__charT>& out) {
  if constexpr (__cfg::__darwin)
    return false;
  if (typeid(__f) != typeid(std::collate_byname<__charT>))
    return false;
  const __charT a[1] = {__charT('a')};
  if (__f.transform(a, a + 1).find(__charT(1)) == std::basic_string<__charT>::npos)
    return false;
  out = __f.transform(__low, __high);
  const std::size_t __end = out.find(__charT(1));
  if (__end != std::basic_string<__charT>::npos)
    out.resize(__end);
  return true;
}

bool __regex_primary_key(const std::collate<char>& __f, const char* __low, const char* __high, std::string& out) {
  return primary_key(__f, __low, __high, out);
}
bool __regex_primary_key(const std::collate<wchar_t>& __f, const wchar_t* __low, const wchar_t* __high,
                         std::wstring& out) {
  return primary_key(__f, __low, __high, out);
}

}} // namespace __ycxx::__detail
