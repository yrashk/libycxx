// libycxx hosted runtime: named locales on the C library's locales (DECISIONS §7).
//
// A name other than "C", "POSIX" and "C.UTF-8" is valid for a category when the C library's
// newlocale accepts it for that category. Each (name, category) pair the program uses is one
// named_locale: a locale_t holding that category and LC_CTYPE of the name (so strings can be
// converted in the name's own encoding), shared by every facet built from it, reference-counted
// (the facets hold the references), freed with the last of them. What the facets read from it is
// computed once: the ctype<char> table and case mappings when the LC_CTYPE entry is opened, the
// numpunct, moneypunct and time_get data when such a facet is constructed. Nothing here changes
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
#include <cstring>
#include <string>
#include <ycxx/hosted/memory_resource.hpp> // ycxx::detail::pal_lock
#include "locale_named.hpp"

#include <ctype.h>
#include <langinfo.h>
#include <locale.h>
#include <nl_types.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#if YCXX_TARGET_DARWIN
// Darwin declares the _l functions in <xlocale.h>, for the headers included before it.
#  include <xlocale.h>
#endif

namespace {

constexpr int ctype_index = 1;
constexpr int c_masks[ycxx::detail::locale_ncategories] = {LC_COLLATE_MASK, LC_CTYPE_MASK, LC_MONETARY_MASK,
                                                           LC_NUMERIC_MASK, LC_TIME_MASK, LC_MESSAGES_MASK};

ycxx::detail::pal_lock cache_lock; // the list of open locales and their counts
ycxx::detail::pal_lock lconv_lock; // localeconv's static object

struct lock_guard {
  explicit lock_guard(ycxx::detail::pal_lock& l) noexcept : l_(l) { l_.lock(); }
  ~lock_guard() { l_.unlock(); }
  lock_guard(const lock_guard&) = delete;
  ycxx::detail::pal_lock& l_;
};

// The calling thread's locale is loc while this object lives.
struct thread_locale {
  explicit thread_locale(locale_t loc) noexcept : old_(::uselocale(loc)) {}
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

namespace ycxx::detail {

struct named_locale {
  std::size_t refs; // under cache_lock
  named_locale* next;
  int category; // index
  std::string name;
  locale_t loc;
  // LC_CTYPE only: ctype<char>'s table and case mappings, ctype<wchar_t>'s widen (btowc, WEOF for
  // a byte that is not a character by itself) and narrow (the pairs of widen, by wide value)
  std::ctype_base::mask table[256];
  unsigned char upper[256], lower[256];
  wchar_t widen[256];
  struct narrow_pair {
    wchar_t w;
    unsigned char c;
  } narrow[256];
  int nnarrow;
  int mb_max; // MB_CUR_MAX
};

} // namespace ycxx::detail

namespace {

using ycxx::detail::named_locale;

named_locale* open_list = nullptr; // under cache_lock

[[noreturn]] void bad_name(const char* what, const char* name) {
  std::string msg(what);
  msg += ": unsupported locale name \"";
  if (name != nullptr)
    msg += name;
  msg += '"';
  ::ycxx::detail::throw_runtime_error(msg.c_str());
}

// newlocale for category c (an index) of name, with the name's LC_CTYPE where it has one.
locale_t new_c_locale(const char* name, int c) noexcept {
  locale_t l = ::newlocale(c_masks[c] | LC_CTYPE_MASK, name, static_cast<locale_t>(0));
  if (l == static_cast<locale_t>(0))
    l = ::newlocale(c_masks[c], name, static_cast<locale_t>(0));
  return l;
}

void fill_ctype(named_locale& h) {
  using B = std::ctype_base;
  for (int c = 0; c < 256; ++c) {
    B::mask m = 0;
    if (isspace_l(c, h.loc))
      m |= B::space;
    if (isprint_l(c, h.loc))
      m |= B::print;
    if (iscntrl_l(c, h.loc))
      m |= B::cntrl;
    if (isupper_l(c, h.loc))
      m |= B::upper;
    if (islower_l(c, h.loc))
      m |= B::lower;
    if (isalpha_l(c, h.loc))
      m |= B::alpha;
    if (isdigit_l(c, h.loc))
      m |= B::digit;
    if (ispunct_l(c, h.loc))
      m |= B::punct;
    if (isxdigit_l(c, h.loc))
      m |= B::xdigit;
    if (isblank_l(c, h.loc))
      m |= B::blank;
    h.table[c] = m;
    h.upper[c] = static_cast<unsigned char>(toupper_l(c, h.loc));
    h.lower[c] = static_cast<unsigned char>(tolower_l(c, h.loc));
  }
  {
    thread_locale in(h.loc);
    for (int c = 0; c < 256; ++c)
      h.widen[c] = static_cast<wchar_t>(::btowc(c));
    h.mb_max = static_cast<int>(MB_CUR_MAX);
  }
  h.nnarrow = 0;
  for (int c = 0; c < 256; ++c)
    if (h.widen[c] != static_cast<wchar_t>(WEOF))
      h.narrow[h.nnarrow++] = {h.widen[c], static_cast<unsigned char>(c)};
  // by wide value; the smallest byte first among equal values (insertion sort: 256 entries once)
  for (int i = 1; i < h.nnarrow; ++i)
    for (int j = i; j > 0 && h.narrow[j].w < h.narrow[j - 1].w; --j) {
      const auto t = h.narrow[j];
      h.narrow[j] = h.narrow[j - 1];
      h.narrow[j - 1] = t;
    }
}

// Releases a reference at the end of a scope.
struct named_ref {
  named_locale* h;
  ~named_ref() { ycxx::detail::named_release(h); }
};

// ---- localeconv --------------------------------------------------------------------------------

struct lconv_copy {
  std::string decimal_point, thousands_sep, grouping, int_curr_symbol, currency_symbol, mon_decimal_point,
      mon_thousands_sep, mon_grouping, positive_sign, negative_sign;
  char int_frac_digits, frac_digits, p_cs_precedes, p_sep_by_space, n_cs_precedes, n_sep_by_space, p_sign_posn,
      n_sign_posn, int_p_cs_precedes, int_p_sep_by_space, int_n_cs_precedes, int_n_sep_by_space, int_p_sign_posn,
      int_n_sign_posn;
};

void copy_lconv(const struct lconv& l, lconv_copy& o) {
  o.decimal_point = l.decimal_point;
  o.thousands_sep = l.thousands_sep;
  o.grouping = l.grouping;
  o.int_curr_symbol = l.int_curr_symbol;
  o.currency_symbol = l.currency_symbol;
  o.mon_decimal_point = l.mon_decimal_point;
  o.mon_thousands_sep = l.mon_thousands_sep;
  o.mon_grouping = l.mon_grouping;
  o.positive_sign = l.positive_sign;
  o.negative_sign = l.negative_sign;
  o.int_frac_digits = l.int_frac_digits;
  o.frac_digits = l.frac_digits;
  o.p_cs_precedes = l.p_cs_precedes;
  o.p_sep_by_space = l.p_sep_by_space;
  o.n_cs_precedes = l.n_cs_precedes;
  o.n_sep_by_space = l.n_sep_by_space;
  o.p_sign_posn = l.p_sign_posn;
  o.n_sign_posn = l.n_sign_posn;
  o.int_p_cs_precedes = l.int_p_cs_precedes;
  o.int_p_sep_by_space = l.int_p_sep_by_space;
  o.int_n_cs_precedes = l.int_n_cs_precedes;
  o.int_n_sep_by_space = l.int_n_sep_by_space;
  o.int_p_sign_posn = l.int_p_sign_posn;
  o.int_n_sign_posn = l.int_n_sign_posn;
}

// Whether the C library has localeconv_l (Darwin; found by argument-dependent lookup on locale_t,
// so no preprocessor test is needed).
template <class L>
concept has_localeconv_l = requires(L l) { localeconv_l(l); };

template <class L>
void read_lconv(L loc, lconv_copy& o) {
  if constexpr (has_localeconv_l<L>) {
    copy_lconv(*localeconv_l(loc), o);
  } else {
    lock_guard g(lconv_lock);
    thread_locale in(loc);
    copy_lconv(*::localeconv(), o);
  }
}

// ---- strings in the locale's encoding ------------------------------------------------------------

// s converted to wide characters by mbrtowc in loc (stops at an invalid sequence).
std::wstring to_wide(locale_t loc, const char* s) {
  std::wstring r;
  thread_locale in(loc);
  ::mbstate_t st{};
  const char* end = s + std::strlen(s);
  while (s != end) {
    wchar_t wc;
    std::size_t n = ::mbrtowc(&wc, s, static_cast<std::size_t>(end - s), &st);
    if (n == mb_error || n == mb_incomplete)
      break;
    if (n == 0)
      n = 1;
    r.push_back(wc);
    s += n;
  }
  return r;
}

bool space_like(locale_t loc, wchar_t wc) noexcept {
  // the no-break spaces, which iswspace does not count, separate digit groups in many locales
  return wc == 0xA0 || wc == 0x2007 || wc == 0x202F || iswspace_l(static_cast<wint_t>(wc), loc);
}

// A separator as one char: itself when it is one byte; ' ' for a space character of more bytes
// (fr_FR.UTF-8's U+202F); dflt when empty or any other character of more bytes.
char narrow_sep(locale_t loc, const std::string& s, char dflt) {
  if (s.size() == 1)
    return s[0];
  if (s.empty())
    return dflt;
  const std::wstring w = to_wide(loc, s.c_str());
  return w.size() == 1 && space_like(loc, w[0]) ? ' ' : dflt;
}
wchar_t wide_sep(locale_t loc, const std::string& s, wchar_t dflt) {
  if (s.empty())
    return dflt;
  const std::wstring w = to_wide(loc, s.c_str());
  return w.empty() ? dflt : w[0];
}

std::string convert(locale_t, const char* s, char) { return s; }
std::wstring convert(locale_t loc, const char* s, wchar_t) { return to_wide(loc, s); }

template <class charT>
charT separator(locale_t loc, const std::string& s, charT dflt) {
  if constexpr (std::is_same_v<charT, char>)
    return narrow_sep(loc, s, dflt);
  else
    return wide_sep(loc, s, dflt);
}

// ---- numpunct, moneypunct ---------------------------------------------------------------------

template <class charT>
void load_numpunct(const char* name, charT& point, charT& sep, std::string& grouping) {
  named_ref h{ycxx::detail::named_open(name, std::locale::numeric, "std::numpunct_byname")};
  if (h.h == nullptr)
    return;
  lconv_copy l;
  read_lconv(h.h->loc, l);
  point = separator<charT>(h.h->loc, l.decimal_point, charT('.'));
  sep = separator<charT>(h.h->loc, l.thousands_sep, charT(','));
  grouping = l.thousands_sep.empty() ? std::string() : l.grouping;
}

// The pattern of POSIX's cs_precedes, sep_by_space and sign_posn ([locale.moneypunct.general]/3:
// none never first, space neither first nor last). A separating space is a space field; where
// the format has no space, the none field marks where internal padding goes.
std::money_base::pattern money_pattern(char cs_precedes, char sep_by_space, char sign_posn,
                                       std::money_base::pattern dflt) {
  using M = std::money_base;
  if (cs_precedes == CHAR_MAX || sep_by_space == CHAR_MAX || sign_posn == CHAR_MAX || sep_by_space < 0 ||
      sep_by_space > 2 || sign_posn < 0 || sign_posn > 4)
    return dflt;
  const char S = M::symbol, G = M::sign, V = M::value, gap = sep_by_space == 0 ? M::none : M::space;
  const char sp = M::space;
  const bool two = sep_by_space == 2; // a space between symbol and sign when adjacent, else sign and value
  if (cs_precedes) {
    switch (sign_posn) {
    case 0: // parentheses around both: the sign string "()" before both
    case 1:
    case 3: // sign symbol value
      return two ? M::pattern{{G, sp, S, V}} : M::pattern{{G, S, gap, V}};
    case 2: // symbol value sign
      return two ? M::pattern{{S, V, sp, G}} : M::pattern{{S, gap, V, G}};
    default: // 4: symbol sign value
      return two ? M::pattern{{S, sp, G, V}} : M::pattern{{S, G, gap, V}};
    }
  }
  switch (sign_posn) {
  case 0:
  case 1: // sign value symbol
    return two ? M::pattern{{G, sp, V, S}} : M::pattern{{G, V, gap, S}};
  case 3: // value sign symbol
    return two ? M::pattern{{V, G, sp, S}} : M::pattern{{V, gap, G, S}};
  default: // 2, 4: value symbol sign
    return two ? M::pattern{{V, S, sp, G}} : M::pattern{{V, gap, S, G}};
  }
}

template <class charT>
void load_money(const char* name, bool intl, ycxx::detail::money_data<charT>& d) {
  named_ref h{ycxx::detail::named_open(name, std::locale::monetary, "std::moneypunct_byname")};
  if (h.h == nullptr)
    return;
  lconv_copy l;
  read_lconv(h.h->loc, l);
  const locale_t loc = h.h->loc;
  d.point = separator<charT>(loc, l.mon_decimal_point, d.point);
  d.sep = separator<charT>(loc, l.mon_thousands_sep, d.sep);
  d.grouping = l.mon_thousands_sep.empty() ? std::string() : l.mon_grouping;
  d.symbol = convert(loc, intl ? l.int_curr_symbol.c_str() : l.currency_symbol.c_str(), charT());
  d.positive = convert(loc, l.positive_sign.c_str(), charT());
  d.negative = convert(loc, l.negative_sign.c_str(), charT());
  const char frac = intl ? l.int_frac_digits : l.frac_digits;
  d.frac_digits = frac == CHAR_MAX || frac < 0 ? 0 : frac;
  const char pcs = intl ? l.int_p_cs_precedes : l.p_cs_precedes, psep = intl ? l.int_p_sep_by_space : l.p_sep_by_space,
             ppos = intl ? l.int_p_sign_posn : l.p_sign_posn, ncs = intl ? l.int_n_cs_precedes : l.n_cs_precedes,
             nsep = intl ? l.int_n_sep_by_space : l.n_sep_by_space, npos = intl ? l.int_n_sign_posn : l.n_sign_posn;
  d.pos = money_pattern(pcs, psep, ppos, d.pos);
  d.neg = money_pattern(ncs, nsep, npos, d.neg);
  if (ppos == 0)
    d.positive = convert(loc, "()", charT());
  if (npos == 0)
    d.negative = convert(loc, "()", charT());
}

// ---- time ----------------------------------------------------------------------------------------

std::time_base::dateorder order_of(const char* fmt) noexcept {
  char seen[3];
  int n = 0;
  for (const char* p = fmt; *p && n < 3; ++p) {
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
      seen[n++] = k;
  }
  if (n != 3)
    return std::time_base::no_order;
  if (seen[0] == 'd' && seen[1] == 'm' && seen[2] == 'y')
    return std::time_base::dmy;
  if (seen[0] == 'm' && seen[1] == 'd' && seen[2] == 'y')
    return std::time_base::mdy;
  if (seen[0] == 'y' && seen[1] == 'm' && seen[2] == 'd')
    return std::time_base::ymd;
  if (seen[0] == 'y' && seen[1] == 'd' && seen[2] == 'm')
    return std::time_base::ydm;
  return std::time_base::no_order;
}

template <class charT>
bool load_time(const char* name, ycxx::detail::time_data<charT>& d) {
  named_ref h{ycxx::detail::named_open(name, std::locale::time, "std::time_get_byname")};
  if (h.h == nullptr)
    return false;
  const locale_t loc = h.h->loc;
  static constexpr nl_item items[40] = {
      DAY_1,  DAY_2,  DAY_3,  DAY_4,  DAY_5,   DAY_6,   DAY_7,   ABDAY_1, ABDAY_2, ABDAY_3,
      ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7, MON_1,  MON_2,   MON_3,   MON_4,   MON_5,   MON_6,
      MON_7,  MON_8,  MON_9,  MON_10, MON_11,  MON_12,  ABMON_1, ABMON_2, ABMON_3, ABMON_4,
      ABMON_5, ABMON_6, ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12, AM_STR, PM_STR};
  for (int i = 0; i < 40; ++i)
    d.names[i] = convert(loc, ::nl_langinfo_l(items[i], loc), charT());
  d.d_t_fmt = convert(loc, ::nl_langinfo_l(D_T_FMT, loc), charT());
  const char* x = ::nl_langinfo_l(D_FMT, loc);
  d.d_fmt = convert(loc, x, charT());
  d.t_fmt = convert(loc, ::nl_langinfo_l(T_FMT, loc), charT());
  d.t_fmt_ampm = convert(loc, ::nl_langinfo_l(T_FMT_AMPM, loc), charT());
  d.order = order_of(x);
  return true;
}

// strftime_l / wcsftime_l of one conversion; the length of the result (once more in 1024 if it
// does not fit in cap:
// both return 0 both for an empty result and for one that does not fit; one conversion of a C
// library locale is far shorter than 1024 characters, so 0 there means empty).
template <class charT>
std::size_t put_time(locale_t loc, charT* buf, std::size_t cap, const std::tm* t, char format, char modifier) {
  charT fmt[4] = {charT('%')};
  int k = 1;
  if (modifier != 0)
    fmt[k++] = charT(modifier);
  fmt[k++] = charT(static_cast<unsigned char>(format));
  fmt[k] = charT();
  auto call = [&](charT* to, std::size_t n) -> std::size_t {
    if constexpr (std::is_same_v<charT, char>)
      return ::strftime_l(to, n, fmt, t, loc);
    else
      return ::wcsftime_l(to, n, fmt, t, loc);
  };
  if (cap > 1) {
    const std::size_t n = call(buf, cap);
    if (n != 0)
      return n;
  }
  if (cap >= 1024)
    return 0;
  charT big[1024];
  const std::size_t n = call(big, 1024);
  for (std::size_t i = 0; i < n && i < cap; ++i)
    buf[i] = big[i];
  return n;
}

// ---- collate ------------------------------------------------------------------------------------

// The runs of [low, high) between embedded null characters, compared run by run.
template <class charT, class Coll>
int compare_runs(const charT* low1, const charT* high1, const charT* low2, const charT* high2, Coll coll) {
  const std::basic_string<charT> a(low1, high1), b(low2, high2);
  const charT *p = a.c_str(), *pe = p + a.size(), *q = b.c_str(), *qe = q + b.size();
  for (;;) {
    const int r = coll(p, q);
    if (r != 0)
      return r < 0 ? -1 : 1;
    p += std::char_traits<charT>::length(p);
    q += std::char_traits<charT>::length(q);
    if (p == pe || q == qe)
      return p == pe ? (q == qe ? 0 : -1) : 1;
    ++p;
    ++q;
  }
}

template <class charT, class Xfrm>
std::basic_string<charT> transform_runs(const charT* low, const charT* high, Xfrm xfrm) {
  const std::basic_string<charT> a(low, high);
  const charT *p = a.c_str(), *pe = p + a.size();
  std::basic_string<charT> r;
  for (;;) {
    std::size_t have = r.size();
    std::size_t n = 2 * std::char_traits<charT>::length(p) + 16;
    for (;;) {
      r.resize(have + n);
      const std::size_t m = xfrm(r.data() + have, p, n);
      if (m < n) {
        r.resize(have + m);
        break;
      }
      n = m + 1;
    }
    p += std::char_traits<charT>::length(p);
    if (p == pe)
      return r;
    r.push_back(charT()); // keeps the runs apart, ordered before any key character
    ++p;
  }
}

template <class charT>
long hash_of(const std::basic_string<charT>& s) noexcept {
  unsigned long h = 14695981039346656037ul; // FNV-1a over the key
  for (charT c : s) {
    h ^= static_cast<unsigned long>(static_cast<std::make_unsigned_t<charT>>(c));
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
  lock_guard g(cache_lock);
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
  lock_guard g(cache_lock);
  return c >= 0 && c < catalogs.n ? catalogs.d[c] : no_catalog;
}
nl_catd catalog_remove(int c) {
  lock_guard g(cache_lock);
  if (c < 0 || c >= catalogs.n)
    return no_catalog;
  const nl_catd d = catalogs.d[c];
  catalogs.d[c] = no_catalog;
  return d;
}

int open_catalog(const named_locale* h, const std::string& fn) {
  nl_catd d;
  {
    thread_locale in(h->loc); // catopen's NL_CAT_LOCALE reads the thread's LC_MESSAGES
    d = ::catopen(fn.c_str(), NL_CAT_LOCALE);
  }
  if (d == no_catalog)
    return -1;
  const int k = catalog_add(d);
  if (k < 0)
    ::catclose(d);
  return k;
}

} // namespace

namespace ycxx::detail {

named_locale* named_open(const char* name, int cat, const char* what) {
  if (name == nullptr)
    bad_name(what, name);
  const int c = category_index(cat);
  std::string part;
  if (!locale_name_part(name, c, part))
    bad_name(what, name);
  if (classic_locale_name(part.c_str()) != nullptr)
    return nullptr;
  {
    lock_guard g(cache_lock);
    for (named_locale* p = open_list; p != nullptr; p = p->next)
      if (p->category == c && p->name == part) {
        ++p->refs;
        return p;
      }
  }
  const locale_t loc = new_c_locale(part.c_str(), c);
  if (loc == static_cast<locale_t>(0))
    bad_name(what, part.c_str());
  named_locale* h;
  if constexpr (cfg::exceptions) {
    try {
      h = new named_locale{1, nullptr, c, part, loc, {}, {}, {}, {}, {}, 0, 1};
    } catch (...) {
      ::freelocale(loc);
      throw;
    }
  } else {
    h = new named_locale{1, nullptr, c, part, loc, {}, {}, {}, {}, {}, 0, 1};
  }
  if (c == ctype_index)
    fill_ctype(*h); // does not throw
  // another thread may have opened the same one meanwhile: keep the first
  named_locale* mine = h;
  {
    lock_guard g(cache_lock);
    for (named_locale* p = open_list; p != nullptr; p = p->next)
      if (p->category == c && p->name == part) {
        ++p->refs;
        h = p;
        break;
      }
    if (h == mine) {
      h->next = open_list;
      open_list = h;
    }
  }
  if (h != mine) {
    ::freelocale(mine->loc);
    delete mine;
  }
  return h;
}

void named_release(named_locale* h) noexcept {
  if (h == nullptr)
    return;
  {
    lock_guard g(cache_lock);
    if (--h->refs != 0)
      return;
    for (named_locale** p = &open_list; *p != nullptr; p = &(*p)->next)
      if (*p == h) {
        *p = h->next;
        break;
      }
  }
  ::freelocale(h->loc);
  delete h;
}

const std::ctype_base::mask* named_ctype_table(const named_locale* h) noexcept { return h ? h->table : nullptr; }
const unsigned char* named_toupper_table(const named_locale* h) noexcept { return h ? h->upper : nullptr; }
const unsigned char* named_tolower_table(const named_locale* h) noexcept { return h ? h->lower : nullptr; }

void named_numpunct(const char* name, char& point, char& sep, std::string& grouping) {
  load_numpunct(name, point, sep, grouping);
}
void named_numpunct(const char* name, wchar_t& point, wchar_t& sep, std::string& grouping) {
  load_numpunct(name, point, sep, grouping);
}
void named_money_data(const char* name, bool intl, money_data<char>& d) { load_money(name, intl, d); }
void named_money_data(const char* name, bool intl, money_data<wchar_t>& d) { load_money(name, intl, d); }
bool named_time_data(const char* name, time_data<char>& d) { return load_time(name, d); }
bool named_time_data(const char* name, time_data<wchar_t>& d) { return load_time(name, d); }
std::size_t named_strftime(const named_locale* h, char* buf, std::size_t cap, const std::tm* t, char format,
                           char modifier) {
  return put_time(h->loc, buf, cap, t, format, modifier);
}
std::size_t named_strftime(const named_locale* h, wchar_t* buf, std::size_t cap, const std::tm* t, char format,
                           char modifier) {
  return put_time(h->loc, buf, cap, t, format, modifier);
}

bool named_exists(const char* name, int c) {
  const locale_t l = ::newlocale(c_masks[c], name, static_cast<locale_t>(0));
  if (l == static_cast<locale_t>(0))
    return false;
  ::freelocale(l);
  return true;
}

std::string named_codeset(const char* name) {
  const locale_t l = ::newlocale(LC_CTYPE_MASK, name, static_cast<locale_t>(0));
  if (l == static_cast<locale_t>(0))
    return std::string();
  std::string r;
  if constexpr (cfg::exceptions) {
    try {
      r = ::nl_langinfo_l(CODESET, l);
    } catch (...) {
      ::freelocale(l);
      throw;
    }
  } else {
    r = ::nl_langinfo_l(CODESET, l);
  }
  ::freelocale(l);
  return r;
}

} // namespace ycxx::detail

namespace std {

// ---- ctype_byname<wchar_t> -----------------------------------------------------------------------

ctype_byname<wchar_t>::~ctype_byname() { ::ycxx::detail::named_release(named_); }

namespace {
ctype_base::mask wide_mask(locale_t loc, wchar_t c) noexcept {
  const wint_t w = static_cast<wint_t>(c);
  ctype_base::mask m = 0;
  if (iswspace_l(w, loc))
    m |= ctype_base::space;
  if (iswprint_l(w, loc))
    m |= ctype_base::print;
  if (iswcntrl_l(w, loc))
    m |= ctype_base::cntrl;
  if (iswupper_l(w, loc))
    m |= ctype_base::upper;
  if (iswlower_l(w, loc))
    m |= ctype_base::lower;
  if (iswalpha_l(w, loc))
    m |= ctype_base::alpha;
  if (iswdigit_l(w, loc))
    m |= ctype_base::digit;
  if (iswpunct_l(w, loc))
    m |= ctype_base::punct;
  if (iswxdigit_l(w, loc))
    m |= ctype_base::xdigit;
  if (iswblank_l(w, loc))
    m |= ctype_base::blank;
  return m;
}
} // namespace

bool ctype_byname<wchar_t>::do_is(mask m, wchar_t c) const {
  if (named_ == nullptr)
    return ctype<wchar_t>::do_is(m, c);
  return (wide_mask(named_->loc, c) & m) != 0;
}
const wchar_t* ctype_byname<wchar_t>::do_is(const wchar_t* low, const wchar_t* high, mask* vec) const {
  if (named_ == nullptr)
    return ctype<wchar_t>::do_is(low, high, vec);
  for (; low != high; ++low, ++vec)
    *vec = wide_mask(named_->loc, *low);
  return high;
}
wchar_t ctype_byname<wchar_t>::do_toupper(wchar_t c) const {
  if (named_ == nullptr)
    return ctype<wchar_t>::do_toupper(c);
  return static_cast<wchar_t>(towupper_l(static_cast<wint_t>(c), named_->loc));
}
const wchar_t* ctype_byname<wchar_t>::do_toupper(wchar_t* low, const wchar_t* high) const {
  for (; low != high; ++low)
    *low = do_toupper(*low);
  return high;
}
wchar_t ctype_byname<wchar_t>::do_tolower(wchar_t c) const {
  if (named_ == nullptr)
    return ctype<wchar_t>::do_tolower(c);
  return static_cast<wchar_t>(towlower_l(static_cast<wint_t>(c), named_->loc));
}
const wchar_t* ctype_byname<wchar_t>::do_tolower(wchar_t* low, const wchar_t* high) const {
  for (; low != high; ++low)
    *low = do_tolower(*low);
  return high;
}
wchar_t ctype_byname<wchar_t>::do_widen(char c) const {
  if (named_ == nullptr)
    return ctype<wchar_t>::do_widen(c);
  return named_->widen[static_cast<unsigned char>(c)];
}
const char* ctype_byname<wchar_t>::do_widen(const char* low, const char* high, wchar_t* dest) const {
  for (; low != high; ++low, ++dest)
    *dest = do_widen(*low);
  return high;
}
char ctype_byname<wchar_t>::do_narrow(wchar_t c, char dfault) const {
  if (named_ == nullptr)
    return ctype<wchar_t>::do_narrow(c, dfault);
  // wctob: the byte whose btowc is c
  int lo = 0, hi = named_->nnarrow;
  while (lo < hi) {
    const int mid = lo + (hi - lo) / 2;
    if (named_->narrow[mid].w < c)
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo < named_->nnarrow && named_->narrow[lo].w == c ? static_cast<char>(named_->narrow[lo].c) : dfault;
}
const wchar_t* ctype_byname<wchar_t>::do_narrow(const wchar_t* low, const wchar_t* high, char dfault,
                                                char* dest) const {
  for (; low != high; ++low, ++dest)
    *dest = do_narrow(*low, dfault);
  return high;
}

// ---- codecvt_byname<wchar_t, char, mbstate_t> ------------------------------------------------------
// One character at a time through a copy of the state, so a character that does not fit, or an
// incomplete sequence at the end of the input, leaves the state as it was before it.

codecvt_byname<wchar_t, char, mbstate_t>::~codecvt_byname() { ::ycxx::detail::named_release(named_); }

codecvt_base::result codecvt_byname<wchar_t, char, mbstate_t>::do_out(mbstate_t& state, const wchar_t* from,
                                                                      const wchar_t* from_end,
                                                                      const wchar_t*& from_next, char* to,
                                                                      char* to_end, char*& to_next) const {
  if (named_ == nullptr)
    return codecvt::do_out(state, from, from_end, from_next, to, to_end, to_next);
  thread_locale in(named_->loc);
  mbstate_t* st = &state;
  result r = ok;
  for (; from != from_end; ++from) {
    char buf[MB_LEN_MAX];
    ::mbstate_t tmp = *st;
    const std::size_t n = ::wcrtomb(buf, *from, &tmp);
    if (n == mb_error) {
      r = error;
      break;
    }
    if (n > static_cast<std::size_t>(to_end - to)) {
      r = partial;
      break;
    }
    std::memcpy(to, buf, n);
    to += n;
    *st = tmp;
  }
  from_next = from;
  to_next = to;
  return r;
}

codecvt_base::result codecvt_byname<wchar_t, char, mbstate_t>::do_in(mbstate_t& state, const char* from,
                                                                     const char* from_end, const char*& from_next,
                                                                     wchar_t* to, wchar_t* to_end,
                                                                     wchar_t*& to_next) const {
  if (named_ == nullptr)
    return codecvt::do_in(state, from, from_end, from_next, to, to_end, to_next);
  thread_locale in(named_->loc);
  mbstate_t* st = &state;
  result r = ok;
  while (from != from_end) {
    if (to == to_end) {
      r = partial;
      break;
    }
    ::mbstate_t tmp = *st;
    wchar_t wc;
    std::size_t n = ::mbrtowc(&wc, from, static_cast<std::size_t>(from_end - from), &tmp);
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
    *st = tmp;
  }
  from_next = from;
  to_next = to;
  return r;
}

codecvt_base::result codecvt_byname<wchar_t, char, mbstate_t>::do_unshift(mbstate_t& state, char* to, char* to_end,
                                                                          char*& to_next) const {
  if (named_ == nullptr)
    return codecvt::do_unshift(state, to, to_end, to_next);
  to_next = to;
  thread_locale in(named_->loc);
  mbstate_t* st = &state;
  char buf[MB_LEN_MAX];
  ::mbstate_t tmp = *st;
  std::size_t n = ::wcrtomb(buf, L'\0', &tmp); // the shift sequence, then the null character
  if (n == mb_error)
    return error;
  if (--n == 0)
    return noconv;
  if (n > static_cast<std::size_t>(to_end - to))
    return partial;
  std::memcpy(to, buf, n);
  to_next = to + n;
  *st = tmp;
  return ok;
}

// A state-dependent encoding (-1) is not detected: the C library's only probe for it, mbtowc
// with a null string, resets an internal state shared by every thread.
int codecvt_byname<wchar_t, char, mbstate_t>::do_encoding() const noexcept {
  if (named_ == nullptr)
    return codecvt::do_encoding();
  return named_->mb_max == 1 ? 1 : 0;
}
bool codecvt_byname<wchar_t, char, mbstate_t>::do_always_noconv() const noexcept { return false; }

int codecvt_byname<wchar_t, char, mbstate_t>::do_length(mbstate_t& state, const char* from, const char* end,
                                                        size_t max) const {
  if (named_ == nullptr)
    return codecvt::do_length(state, from, end, max);
  thread_locale in(named_->loc);
  mbstate_t* st = &state;
  const char* p = from;
  for (; p != end && max != 0; --max) {
    ::mbstate_t tmp = *st;
    wchar_t wc;
    std::size_t n = ::mbrtowc(&wc, p, static_cast<std::size_t>(end - p), &tmp);
    if (n == mb_error || n == mb_incomplete)
      break;
    if (n == 0)
      n = 1;
    p += n;
    *st = tmp;
  }
  return static_cast<int>(p - from);
}

int codecvt_byname<wchar_t, char, mbstate_t>::do_max_length() const noexcept {
  if (named_ == nullptr)
    return codecvt::do_max_length();
  return named_->mb_max;
}

// ---- collate_byname --------------------------------------------------------------------------------

collate_byname<char>::~collate_byname() { ::ycxx::detail::named_release(named_); }
int collate_byname<char>::do_compare(const char* low1, const char* high1, const char* low2, const char* high2) const {
  if (named_ == nullptr)
    return collate::do_compare(low1, high1, low2, high2);
  const locale_t loc = named_->loc;
  return compare_runs(low1, high1, low2, high2, [loc](const char* a, const char* b) { return ::strcoll_l(a, b, loc); });
}
string collate_byname<char>::do_transform(const char* low, const char* high) const {
  if (named_ == nullptr)
    return collate::do_transform(low, high);
  const locale_t loc = named_->loc;
  return transform_runs(low, high,
                        [loc](char* to, const char* s, size_t n) { return ::strxfrm_l(to, s, n, loc); });
}
long collate_byname<char>::do_hash(const char* low, const char* high) const {
  if (named_ == nullptr)
    return collate::do_hash(low, high);
  return hash_of(do_transform(low, high));
}

collate_byname<wchar_t>::~collate_byname() { ::ycxx::detail::named_release(named_); }
int collate_byname<wchar_t>::do_compare(const wchar_t* low1, const wchar_t* high1, const wchar_t* low2,
                                        const wchar_t* high2) const {
  if (named_ == nullptr)
    return collate::do_compare(low1, high1, low2, high2);
  const locale_t loc = named_->loc;
  return compare_runs(low1, high1, low2, high2,
                      [loc](const wchar_t* a, const wchar_t* b) { return ::wcscoll_l(a, b, loc); });
}
wstring collate_byname<wchar_t>::do_transform(const wchar_t* low, const wchar_t* high) const {
  if (named_ == nullptr)
    return collate::do_transform(low, high);
  const locale_t loc = named_->loc;
  return transform_runs(low, high,
                        [loc](wchar_t* to, const wchar_t* s, size_t n) { return ::wcsxfrm_l(to, s, n, loc); });
}
long collate_byname<wchar_t>::do_hash(const wchar_t* low, const wchar_t* high) const {
  if (named_ == nullptr)
    return collate::do_hash(low, high);
  return hash_of(do_transform(low, high));
}

// ---- messages_byname ---------------------------------------------------------------------------------

messages_byname<char>::~messages_byname() { ::ycxx::detail::named_release(named_); }
messages_base::catalog messages_byname<char>::do_open(const string& fn, const locale& loc) const {
  if (named_ == nullptr)
    return messages::do_open(fn, loc);
  return open_catalog(named_, fn);
}
string messages_byname<char>::do_get(catalog c, int set, int msgid, const string& dfault) const {
  if (named_ == nullptr)
    return messages::do_get(c, set, msgid, dfault);
  const nl_catd d = catalog_get(c);
  if (d == no_catalog)
    return dfault;
  const char* s = ::catgets(d, set, msgid, nullptr);
  return s != nullptr ? string(s) : dfault;
}
void messages_byname<char>::do_close(catalog c) const {
  if (named_ == nullptr)
    return messages::do_close(c);
  const nl_catd d = catalog_remove(c);
  if (d != no_catalog)
    ::catclose(d);
}

messages_byname<wchar_t>::~messages_byname() { ::ycxx::detail::named_release(named_); }
messages_base::catalog messages_byname<wchar_t>::do_open(const string& fn, const locale& loc) const {
  if (named_ == nullptr)
    return messages::do_open(fn, loc);
  return open_catalog(named_, fn);
}
wstring messages_byname<wchar_t>::do_get(catalog c, int set, int msgid, const wstring& dfault) const {
  if (named_ == nullptr)
    return messages::do_get(c, set, msgid, dfault);
  const nl_catd d = catalog_get(c);
  if (d == no_catalog)
    return dfault;
  const char* s = ::catgets(d, set, msgid, nullptr);
  return s != nullptr ? to_wide(named_->loc, s) : dfault;
}
void messages_byname<wchar_t>::do_close(catalog c) const {
  if (named_ == nullptr)
    return messages::do_close(c);
  const nl_catd d = catalog_remove(c);
  if (d != no_catalog)
    ::catclose(d);
}

} // namespace std
