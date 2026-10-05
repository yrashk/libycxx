// libycxx hosted: the locale machinery and the facets the iostreams need everywhere
// ([locales], [category.ctype], [category.numeric] numpunct, [category.collate]).
//
// locale is a pointer to a reference-counted ycxx::detail::locale_impl (opaque here; defined in
// the hosted runtime, src/hosted/locale.cpp): an array of facet pointers indexed by
// locale::id, and the locale's name. A facet carries an atomic reference count: the count passed
// to its constructor, plus one per locale holding it, so a facet constructed with refs == 0 is
// deleted with the last such locale, and one constructed with refs != 0 never is
// ([locale.facet]/3). locale::id is constant-initialized to "no index"; the index is assigned
// on first use from a global counter ([locale.id] Note 1), so facets work during static
// initialization.
//
// The classic locale holds every facet of [locale.category] Table 91 for char and wchar_t, plus
// the Annex D codecvt<char16_t/char32_t, char8_t> and codecvt<char16_t/char32_t, char>
// ([depr.locale.category], declared [[deprecated]]); it is built on first use and never
// destroyed. Named locales: "C", "POSIX" (named "C") and "C.UTF-8" / "C.utf8" have the classic
// semantics; "" names the environment's locale (LC_ALL, LC_<category>, LANG), which is one of
// those or, for any other name, the classic locale named "C" (the environment's own conventions
// are not supported). Other names throw runtime_error, as do the _byname facets.
//
// Classic semantics chosen where the draft leaves them implementation-defined: ctype<charT>
// for character types other than char classifies the ASCII range only; widen/narrow map the
// values 0-255 one-to-one; codecvt<wchar_t, char, mbstate_t> converts UTF-32 (wchar_t) to and
// from UTF-8.
#pragma once

#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/char_traits.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/stream_iterators.hpp>
#include <ycxx/core/typeinfo.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
struct locale_impl; // src/hosted/locale.cpp
struct locale_access;
// Selects locale's private constructor from a locale_impl*: without it, a null pointer constant
// would also convert to that constructor's parameter and make locale(nullptr) ambiguous.
struct locale_impl_tag {};
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

class locale;
struct text_encoding; // <text_encoding>; <locale> includes it
template <class Facet>
const Facet& use_facet(const locale&);
template <class Facet>
bool has_facet(const locale&) noexcept;

// [locale]
class locale {
public:
  class facet;
  class id;
  using category = int;
  static constexpr category none = 0, collate = 0x010, ctype = 0x020, monetary = 0x040, numeric = 0x080,
                            time = 0x100, messages = 0x200,
                            all = collate | ctype | monetary | numeric | time | messages;

  locale() noexcept;
  locale(const locale& other) noexcept;
  explicit locale(const char* std_name);
  explicit locale(const string& std_name) : locale(std_name.c_str()) {}
  locale(const locale& other, const char* std_name, category cats);
  locale(const locale& other, const string& std_name, category cats) : locale(other, std_name.c_str(), cats) {}
  template <class Facet>
  locale(const locale& other, Facet* f);
  locale(const locale& other, const locale& one, category cats);
  ~locale();
  const locale& operator=(const locale& other) noexcept;

  template <class Facet>
  locale combine(const locale& other) const;
  string name() const;
  text_encoding encoding() const; // src/hosted/text_encoding.cpp
  bool operator==(const locale& other) const;
  template <class charT, class traits, class Allocator>
  bool operator()(const basic_string<charT, traits, Allocator>& s1, const basic_string<charT, traits, Allocator>& s2) const;

  static locale global(const locale& loc);
  static const locale& classic();

private:
  friend ycxx::detail::locale_access;
  template <class Facet>
  friend const Facet& use_facet(const locale&);
  template <class Facet>
  friend bool has_facet(const locale&) noexcept;

  locale(ycxx::detail::locale_impl_tag, ycxx::detail::locale_impl* impl) noexcept : impl_(impl) {}
  // A copy of other with f installed under index i (null f: a copy of other).
  locale(const locale& other, const facet* f, const id& i);
  // The facet under index i, or null.
  const facet* find(const id& i) const noexcept;

  ycxx::detail::locale_impl* impl_;
};

// [locale.facet]
class locale::facet {
protected:
  explicit facet(size_t refs = 0) noexcept : refs_(refs) {}
  virtual ~facet();
  facet(const facet&) = delete;
  void operator=(const facet&) = delete;

private:
  friend ycxx::detail::locale_access;
  mutable size_t refs_; // refs, plus one per locale holding the facet (atomic)
};

// [locale.id]
class locale::id {
public:
  constexpr id() noexcept {}
  void operator=(const id&) = delete;
  id(const id&) = delete;

private:
  friend locale;
  friend ycxx::detail::locale_access;
  // The index, assigned on first use (0: not yet assigned).
  size_t index() const noexcept {
    size_t i = __atomic_load_n(&index_, __ATOMIC_ACQUIRE);
    return i != 0 ? i : assign();
  }
  size_t assign() const noexcept;
  mutable size_t index_ = 0;
};

template <class Facet>
locale::locale(const locale& other, Facet* f) : locale(other, f, Facet::id) {}

template <class Facet>
locale locale::combine(const locale& other) const {
  const facet* f = other.find(Facet::id);
  if (f == nullptr)
    ::ycxx::detail::throw_runtime_error("std::locale::combine: the facet is not present in the other locale");
  return locale(*this, f, Facet::id);
}

// [locale.global.templates]
template <class Facet>
const Facet& use_facet(const locale& loc) {
  const locale::facet* f = loc.find(Facet::id);
  if (f == nullptr)
    ::ycxx::detail::raise_with(ycxx_error_bad_cast, "std::use_facet: the facet is not present in the locale",
                               [] { return bad_cast(); });
  return static_cast<const Facet&>(*f);
}
template <class Facet>
bool has_facet(const locale& loc) noexcept {
  return loc.find(Facet::id) != nullptr;
}

// ---- [locale.syn]: the default template arguments, declared once --------------------------------
template <class charT, class InputIterator = istreambuf_iterator<charT>>
class num_get;
template <class charT, class OutputIterator = ostreambuf_iterator<charT>>
class num_put;
template <class charT, class InputIterator = istreambuf_iterator<charT>>
class time_get;
template <class charT, class InputIterator = istreambuf_iterator<charT>>
class time_get_byname;
template <class charT, class OutputIterator = ostreambuf_iterator<charT>>
class time_put;
template <class charT, class OutputIterator = ostreambuf_iterator<charT>>
class time_put_byname;
template <class charT, class InputIterator = istreambuf_iterator<charT>>
class money_get;
template <class charT, class OutputIterator = ostreambuf_iterator<charT>>
class money_put;
template <class charT, bool Intl = false>
class moneypunct;
template <class charT, bool Intl = false>
class moneypunct_byname;

// ---- [category.ctype] -------------------------------------------------------------------------
class ctype_base {
public:
  using mask = unsigned short;
  static constexpr mask space = 1 << 0;
  static constexpr mask print = 1 << 1;
  static constexpr mask cntrl = 1 << 2;
  static constexpr mask upper = 1 << 3;
  static constexpr mask lower = 1 << 4;
  static constexpr mask alpha = 1 << 5;
  static constexpr mask digit = 1 << 6;
  static constexpr mask punct = 1 << 7;
  static constexpr mask xdigit = 1 << 8;
  static constexpr mask blank = 1 << 9;
  static constexpr mask alnum = alpha | digit;
  static constexpr mask graph = alnum | punct;
};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// The "C" classification of the 128 ASCII characters.
consteval auto make_ascii_masks() {
  using B = std::ctype_base;
  struct table {
    B::mask m[128];
  } t{};
  for (int c = 0; c < 128; ++c) {
    B::mask m = 0;
    if (c < 32 || c == 127)
      m |= B::cntrl;
    if (c == ' ' || (c >= '\t' && c <= '\r'))
      m |= B::space;
    if (c == ' ' || c == '\t')
      m |= B::blank;
    if (c >= 32 && c < 127)
      m |= B::print;
    if (c >= 'A' && c <= 'Z')
      m |= B::upper | B::alpha;
    if (c >= 'a' && c <= 'z')
      m |= B::lower | B::alpha;
    if (c >= '0' && c <= '9')
      m |= B::digit;
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
      m |= B::xdigit;
    if (c > 32 && c < 127 && !(m & (B::alpha | B::digit)))
      m |= B::punct;
    t.m[c] = m;
  }
  return t;
}
inline constexpr auto ascii_masks = ::ycxx::detail::make_ascii_masks();

template <class charT>
constexpr std::ctype_base::mask classic_mask(charT c) noexcept {
  using U = std::make_unsigned_t<charT>;
  const U u = static_cast<U>(c);
  return u < 128 ? ascii_masks.m[u] : std::ctype_base::mask(0);
}
template <class charT>
constexpr charT ascii_toupper(charT c) noexcept {
  return c >= charT('a') && c <= charT('z') ? charT(c - charT('a') + charT('A')) : c;
}
template <class charT>
constexpr charT ascii_tolower(charT c) noexcept {
  return c >= charT('A') && c <= charT('Z') ? charT(c - charT('A') + charT('a')) : c;
}

// A string of the library's ASCII literal widened to charT (numpunct names and the like).
template <class charT>
std::basic_string<charT> widen_ascii(const char* s) {
  std::basic_string<charT> r(__builtin_strlen(s), charT());
  for (std::size_t i = 0; i < r.size(); ++i)
    r[i] = static_cast<charT>(s[i]);
  return r;
}

// Accepts every valid locale name (see the file comment); throws runtime_error for any other
// name, null included. Used by the _byname facets of character types other than char and
// wchar_t, which have the classic semantics. Defined in the hosted runtime.
void check_locale_name(const char* name, const char* what);

// ---- named locales (src/hosted/locale_named.cpp) ------------------------------------------------
// One category of a C library locale: a locale_t for one name, shared by every facet built from
// that name and category, reference-counted, immutable once opened (it also holds what the
// facets read from it, computed when it is opened).
struct named_locale;
struct named_tag {};
// The C library's locale `name` for category cat (one std::locale::category bit). name may be ""
// (the environment's, [locale.cons]/4) or a composite name (its part for cat). Returns null for
// the names with the classic semantics ("C", "POSIX", "C.UTF-8"); throws runtime_error, naming
// `what`, for a null name or one the C library does not have.
named_locale* named_open(const char* name, int cat, const char* what);
void named_release(named_locale* h) noexcept;

// ctype<char>: the 256 classifications and case mappings (null for the classic semantics).
const std::ctype_base::mask* named_ctype_table(const named_locale* h) noexcept;
const unsigned char* named_toupper_table(const named_locale* h) noexcept;
const unsigned char* named_tolower_table(const named_locale* h) noexcept;

// numpunct: the decimal point, the thousands separator and the grouping of LC_NUMERIC. A
// separator that is not one char in the locale's encoding is replaced (narrow facets: ' ' for a
// space character, else the classic value); no separator (an empty string) gives the classic
// value and no grouping.
void named_numpunct(const char* name, char& point, char& sep, std::string& grouping);
void named_numpunct(const char* name, wchar_t& point, wchar_t& sep, std::string& grouping);

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [locale.ctype]
template <class charT>
class ctype : public locale::facet, public ctype_base {
public:
  using char_type = charT;

  explicit ctype(size_t refs = 0) : locale::facet(refs) {}

  bool is(mask m, charT c) const { return do_is(m, c); }
  const charT* is(const charT* low, const charT* high, mask* vec) const { return do_is(low, high, vec); }
  const charT* scan_is(mask m, const charT* low, const charT* high) const { return do_scan_is(m, low, high); }
  const charT* scan_not(mask m, const charT* low, const charT* high) const { return do_scan_not(m, low, high); }
  charT toupper(charT c) const { return do_toupper(c); }
  const charT* toupper(charT* low, const charT* high) const { return do_toupper(low, high); }
  charT tolower(charT c) const { return do_tolower(c); }
  const charT* tolower(charT* low, const charT* high) const { return do_tolower(low, high); }
  charT widen(char c) const { return do_widen(c); }
  const char* widen(const char* low, const char* high, charT* to) const { return do_widen(low, high, to); }
  char narrow(charT c, char dfault) const { return do_narrow(c, dfault); }
  const charT* narrow(const charT* low, const charT* high, char dfault, char* to) const {
    return do_narrow(low, high, dfault, to);
  }

  static locale::id id;

protected:
  ~ctype() override {}
  virtual bool do_is(mask m, charT c) const { return (::ycxx::detail::classic_mask(c) & m) != 0; }
  virtual const charT* do_is(const charT* low, const charT* high, mask* vec) const {
    for (; low != high; ++low, ++vec)
      *vec = ::ycxx::detail::classic_mask(*low);
    return high;
  }
  virtual const charT* do_scan_is(mask m, const charT* low, const charT* high) const {
    while (low != high && !is(m, *low))
      ++low;
    return low;
  }
  virtual const charT* do_scan_not(mask m, const charT* low, const charT* high) const {
    while (low != high && is(m, *low))
      ++low;
    return low;
  }
  virtual charT do_toupper(charT c) const { return ::ycxx::detail::ascii_toupper(c); }
  virtual const charT* do_toupper(charT* low, const charT* high) const {
    for (; low != high; ++low)
      *low = ::ycxx::detail::ascii_toupper(*low);
    return high;
  }
  virtual charT do_tolower(charT c) const { return ::ycxx::detail::ascii_tolower(c); }
  virtual const charT* do_tolower(charT* low, const charT* high) const {
    for (; low != high; ++low)
      *low = ::ycxx::detail::ascii_tolower(*low);
    return high;
  }
  virtual charT do_widen(char c) const { return static_cast<charT>(static_cast<unsigned char>(c)); }
  virtual const char* do_widen(const char* low, const char* high, charT* dest) const {
    for (; low != high; ++low, ++dest)
      *dest = do_widen(*low);
    return high;
  }
  virtual char do_narrow(charT c, char dfault) const {
    using U = make_unsigned_t<charT>;
    return static_cast<U>(c) < 256 ? static_cast<char>(static_cast<unsigned char>(c)) : dfault;
  }
  virtual const charT* do_narrow(const charT* low, const charT* high, char dfault, char* dest) const {
    for (; low != high; ++low, ++dest)
      *dest = do_narrow(*low, dfault);
    return high;
  }
};
template <class charT>
locale::id ctype<charT>::id;

// [facet.ctype.special]
template <>
class ctype<char> : public locale::facet, public ctype_base {
public:
  using char_type = char;

  explicit ctype(const mask* tbl = nullptr, bool del = false, size_t refs = 0)
      : locale::facet(refs), table_(tbl ? tbl : classic_table()), del_(tbl != nullptr && del) {}

  bool is(mask m, char c) const { return (table_[static_cast<unsigned char>(c)] & m) != 0; }
  const char* is(const char* low, const char* high, mask* vec) const {
    for (; low != high; ++low, ++vec)
      *vec = table_[static_cast<unsigned char>(*low)];
    return high;
  }
  const char* scan_is(mask m, const char* low, const char* high) const {
    while (low != high && !(table_[static_cast<unsigned char>(*low)] & m))
      ++low;
    return low;
  }
  const char* scan_not(mask m, const char* low, const char* high) const {
    while (low != high && (table_[static_cast<unsigned char>(*low)] & m))
      ++low;
    return low;
  }
  char toupper(char c) const { return do_toupper(c); }
  const char* toupper(char* low, const char* high) const { return do_toupper(low, high); }
  char tolower(char c) const { return do_tolower(c); }
  const char* tolower(char* low, const char* high) const { return do_tolower(low, high); }
  char widen(char c) const { return do_widen(c); }
  const char* widen(const char* low, const char* high, char* to) const { return do_widen(low, high, to); }
  char narrow(char c, char dfault) const { return do_narrow(c, dfault); }
  const char* narrow(const char* low, const char* high, char dfault, char* to) const {
    return do_narrow(low, high, dfault, to);
  }

  static locale::id id;
  static constexpr size_t table_size = 256;
  const mask* table() const noexcept { return table_; }
  static const mask* classic_table() noexcept;

protected:
  ~ctype() override;
  virtual char do_toupper(char c) const { return ::ycxx::detail::ascii_toupper(c); }
  virtual const char* do_toupper(char* low, const char* high) const {
    for (; low != high; ++low)
      *low = ::ycxx::detail::ascii_toupper(*low);
    return high;
  }
  virtual char do_tolower(char c) const { return ::ycxx::detail::ascii_tolower(c); }
  virtual const char* do_tolower(char* low, const char* high) const {
    for (; low != high; ++low)
      *low = ::ycxx::detail::ascii_tolower(*low);
    return high;
  }
  virtual char do_widen(char c) const { return c; }
  virtual const char* do_widen(const char* low, const char* high, char* dest) const {
    if (low != high)
      __builtin_memmove(dest, low, static_cast<size_t>(high - low));
    return high;
  }
  virtual char do_narrow(char c, char) const { return c; }
  virtual const char* do_narrow(const char* low, const char* high, char, char* dest) const {
    if (low != high)
      __builtin_memmove(dest, low, static_cast<size_t>(high - low));
    return high;
  }

private:
  const mask* table_;
  bool del_;
};

// [locale.ctype.byname]: for char and wchar_t, the C library's LC_CTYPE of the name; for other
// character types, the classic semantics (the name is checked).
template <class charT>
class ctype_byname : public ctype<charT> {
public:
  using mask = typename ctype<charT>::mask;
  explicit ctype_byname(const char* name, size_t refs = 0) : ctype<charT>(refs) {
    ::ycxx::detail::check_locale_name(name, "std::ctype_byname");
  }
  explicit ctype_byname(const string& name, size_t refs = 0) : ctype_byname(name.c_str(), refs) {}

protected:
  ~ctype_byname() override {}
};
// The table of is*_l and the case mappings of toupper_l/tolower_l, computed when the locale is
// opened.
template <>
class ctype_byname<char> : public ctype<char> {
public:
  explicit ctype_byname(const char* name, size_t refs = 0)
      : ctype_byname(ycxx::detail::named_tag(), ::ycxx::detail::named_open(name, locale::ctype, "std::ctype_byname"),
                     refs) {}
  explicit ctype_byname(const string& name, size_t refs = 0) : ctype_byname(name.c_str(), refs) {}

protected:
  ~ctype_byname() override { ::ycxx::detail::named_release(named_); }
  char do_toupper(char c) const override {
    return upper_ != nullptr ? static_cast<char>(upper_[static_cast<unsigned char>(c)])
                             : ::ycxx::detail::ascii_toupper(c);
  }
  const char* do_toupper(char* low, const char* high) const override {
    for (; low != high; ++low)
      *low = do_toupper(*low);
    return high;
  }
  char do_tolower(char c) const override {
    return lower_ != nullptr ? static_cast<char>(lower_[static_cast<unsigned char>(c)])
                             : ::ycxx::detail::ascii_tolower(c);
  }
  const char* do_tolower(char* low, const char* high) const override {
    for (; low != high; ++low)
      *low = do_tolower(*low);
    return high;
  }

private:
  ctype_byname(ycxx::detail::named_tag, ycxx::detail::named_locale* h, size_t refs) noexcept
      : ctype<char>(::ycxx::detail::named_ctype_table(h), false, refs), named_(h),
        upper_(::ycxx::detail::named_toupper_table(h)), lower_(::ycxx::detail::named_tolower_table(h)) {}
  ycxx::detail::named_locale* named_;
  const unsigned char* upper_;
  const unsigned char* lower_;
};
// Classification and case mapping by iswctype_l and towupper_l/towlower_l, widen/narrow by
// btowc/wctob in the locale (src/hosted/locale_named.cpp).
template <>
class ctype_byname<wchar_t> : public ctype<wchar_t> {
public:
  explicit ctype_byname(const char* name, size_t refs = 0)
      : ctype<wchar_t>(refs), named_(::ycxx::detail::named_open(name, locale::ctype, "std::ctype_byname")) {}
  explicit ctype_byname(const string& name, size_t refs = 0) : ctype_byname(name.c_str(), refs) {}

protected:
  ~ctype_byname() override;
  bool do_is(mask m, wchar_t c) const override;
  const wchar_t* do_is(const wchar_t* low, const wchar_t* high, mask* vec) const override;
  wchar_t do_toupper(wchar_t c) const override;
  const wchar_t* do_toupper(wchar_t* low, const wchar_t* high) const override;
  wchar_t do_tolower(wchar_t c) const override;
  const wchar_t* do_tolower(wchar_t* low, const wchar_t* high) const override;
  wchar_t do_widen(char c) const override;
  const char* do_widen(const char* low, const char* high, wchar_t* dest) const override;
  char do_narrow(wchar_t c, char dfault) const override;
  const wchar_t* do_narrow(const wchar_t* low, const wchar_t* high, char dfault, char* dest) const override;

private:
  ycxx::detail::named_locale* named_;
};

// [locale.codecvt]
class codecvt_base {
public:
  enum result { ok, partial, error, noconv };
};

// The primary template: a degenerate (noconv) conversion, as codecvt<char, char, mbstate_t>
// ([locale.codecvt.general]/3); a program specializes it for its own state types.
template <class internT, class externT, class stateT>
class codecvt : public locale::facet, public codecvt_base {
public:
  using intern_type = internT;
  using extern_type = externT;
  using state_type = stateT;

  explicit codecvt(size_t refs = 0) : locale::facet(refs) {}

  result out(stateT& state, const internT* from, const internT* from_end, const internT*& from_next, externT* to,
             externT* to_end, externT*& to_next) const {
    return do_out(state, from, from_end, from_next, to, to_end, to_next);
  }
  result unshift(stateT& state, externT* to, externT* to_end, externT*& to_next) const {
    return do_unshift(state, to, to_end, to_next);
  }
  result in(stateT& state, const externT* from, const externT* from_end, const externT*& from_next, internT* to,
            internT* to_end, internT*& to_next) const {
    return do_in(state, from, from_end, from_next, to, to_end, to_next);
  }
  int encoding() const noexcept { return do_encoding(); }
  bool always_noconv() const noexcept { return do_always_noconv(); }
  int length(stateT& state, const externT* from, const externT* end, size_t max) const {
    return do_length(state, from, end, max);
  }
  int max_length() const noexcept { return do_max_length(); }

  static locale::id id;

protected:
  ~codecvt() override {}
  virtual result do_out(stateT&, const internT* from, const internT*, const internT*& from_next, externT* to,
                        externT*, externT*& to_next) const {
    from_next = from;
    to_next = to;
    return noconv;
  }
  virtual result do_in(stateT&, const externT* from, const externT*, const externT*& from_next, internT* to,
                       internT*, internT*& to_next) const {
    from_next = from;
    to_next = to;
    return noconv;
  }
  virtual result do_unshift(stateT&, externT* to, externT*, externT*& to_next) const {
    to_next = to;
    return noconv;
  }
  virtual int do_encoding() const noexcept { return 1; }
  virtual bool do_always_noconv() const noexcept { return true; }
  virtual int do_length(stateT&, const externT* from, const externT* end, size_t max) const {
    const size_t n = static_cast<size_t>(end - from);
    return static_cast<int>(n < max ? n : max);
  }
  virtual int do_max_length() const noexcept { return 1; }
};
template <class internT, class externT, class stateT>
locale::id codecvt<internT, externT, stateT>::id;

// The four required specializations on mbstate_t share this shape; their members are defined in
// the hosted runtime.
template <>
class codecvt<char, char, mbstate_t> : public locale::facet, public codecvt_base {
public:
  using intern_type = char;
  using extern_type = char;
  using state_type = mbstate_t;

  explicit codecvt(size_t refs = 0) : locale::facet(refs) {}
  result out(mbstate_t& state, const char* from, const char* from_end, const char*& from_next, char* to,
             char* to_end, char*& to_next) const {
    return do_out(state, from, from_end, from_next, to, to_end, to_next);
  }
  result unshift(mbstate_t& state, char* to, char* to_end, char*& to_next) const {
    return do_unshift(state, to, to_end, to_next);
  }
  result in(mbstate_t& state, const char* from, const char* from_end, const char*& from_next, char* to, char* to_end,
            char*& to_next) const {
    return do_in(state, from, from_end, from_next, to, to_end, to_next);
  }
  int encoding() const noexcept { return do_encoding(); }
  bool always_noconv() const noexcept { return do_always_noconv(); }
  int length(mbstate_t& state, const char* from, const char* end, size_t max) const {
    return do_length(state, from, end, max);
  }
  int max_length() const noexcept { return do_max_length(); }

  static locale::id id;

protected:
  ~codecvt() override;
  virtual result do_out(mbstate_t& state, const char* from, const char* from_end, const char*& from_next, char* to,
                        char* to_end, char*& to_next) const;
  virtual result do_in(mbstate_t& state, const char* from, const char* from_end, const char*& from_next, char* to,
                       char* to_end, char*& to_next) const;
  virtual result do_unshift(mbstate_t& state, char* to, char* to_end, char*& to_next) const;
  virtual int do_encoding() const noexcept;
  virtual bool do_always_noconv() const noexcept;
  virtual int do_length(mbstate_t&, const char* from, const char* end, size_t max) const;
  virtual int do_max_length() const noexcept;
};

template <>
class codecvt<wchar_t, char, mbstate_t> : public locale::facet, public codecvt_base {
public:
  using intern_type = wchar_t;
  using extern_type = char;
  using state_type = mbstate_t;

  explicit codecvt(size_t refs = 0) : locale::facet(refs) {}
  result out(mbstate_t& state, const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next, char* to,
             char* to_end, char*& to_next) const {
    return do_out(state, from, from_end, from_next, to, to_end, to_next);
  }
  result unshift(mbstate_t& state, char* to, char* to_end, char*& to_next) const {
    return do_unshift(state, to, to_end, to_next);
  }
  result in(mbstate_t& state, const char* from, const char* from_end, const char*& from_next, wchar_t* to,
            wchar_t* to_end, wchar_t*& to_next) const {
    return do_in(state, from, from_end, from_next, to, to_end, to_next);
  }
  int encoding() const noexcept { return do_encoding(); }
  bool always_noconv() const noexcept { return do_always_noconv(); }
  int length(mbstate_t& state, const char* from, const char* end, size_t max) const {
    return do_length(state, from, end, max);
  }
  int max_length() const noexcept { return do_max_length(); }

  static locale::id id;

protected:
  ~codecvt() override;
  virtual result do_out(mbstate_t& state, const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next,
                        char* to, char* to_end, char*& to_next) const;
  virtual result do_in(mbstate_t& state, const char* from, const char* from_end, const char*& from_next, wchar_t* to,
                       wchar_t* to_end, wchar_t*& to_next) const;
  virtual result do_unshift(mbstate_t& state, char* to, char* to_end, char*& to_next) const;
  virtual int do_encoding() const noexcept;
  virtual bool do_always_noconv() const noexcept;
  virtual int do_length(mbstate_t&, const char* from, const char* end, size_t max) const;
  virtual int do_max_length() const noexcept;
};

template <>
class [[deprecated("codecvt<char16_t, char8_t, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt<char16_t, char8_t, mbstate_t> : public locale::facet, public codecvt_base {
public:
  using intern_type = char16_t;
  using extern_type = char8_t;
  using state_type = mbstate_t;

  explicit codecvt(size_t refs = 0) : locale::facet(refs) {}
  result out(mbstate_t& state, const char16_t* from, const char16_t* from_end, const char16_t*& from_next,
             char8_t* to, char8_t* to_end, char8_t*& to_next) const {
    return do_out(state, from, from_end, from_next, to, to_end, to_next);
  }
  result unshift(mbstate_t& state, char8_t* to, char8_t* to_end, char8_t*& to_next) const {
    return do_unshift(state, to, to_end, to_next);
  }
  result in(mbstate_t& state, const char8_t* from, const char8_t* from_end, const char8_t*& from_next, char16_t* to,
            char16_t* to_end, char16_t*& to_next) const {
    return do_in(state, from, from_end, from_next, to, to_end, to_next);
  }
  int encoding() const noexcept { return do_encoding(); }
  bool always_noconv() const noexcept { return do_always_noconv(); }
  int length(mbstate_t& state, const char8_t* from, const char8_t* end, size_t max) const {
    return do_length(state, from, end, max);
  }
  int max_length() const noexcept { return do_max_length(); }

  static locale::id id;

protected:
  ~codecvt() override;
  virtual result do_out(mbstate_t& state, const char16_t* from, const char16_t* from_end, const char16_t*& from_next,
                        char8_t* to, char8_t* to_end, char8_t*& to_next) const;
  virtual result do_in(mbstate_t& state, const char8_t* from, const char8_t* from_end, const char8_t*& from_next,
                       char16_t* to, char16_t* to_end, char16_t*& to_next) const;
  virtual result do_unshift(mbstate_t& state, char8_t* to, char8_t* to_end, char8_t*& to_next) const;
  virtual int do_encoding() const noexcept;
  virtual bool do_always_noconv() const noexcept;
  virtual int do_length(mbstate_t&, const char8_t* from, const char8_t* end, size_t max) const;
  virtual int do_max_length() const noexcept;
};

template <>
class [[deprecated("codecvt<char32_t, char8_t, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt<char32_t, char8_t, mbstate_t> : public locale::facet, public codecvt_base {
public:
  using intern_type = char32_t;
  using extern_type = char8_t;
  using state_type = mbstate_t;

  explicit codecvt(size_t refs = 0) : locale::facet(refs) {}
  result out(mbstate_t& state, const char32_t* from, const char32_t* from_end, const char32_t*& from_next,
             char8_t* to, char8_t* to_end, char8_t*& to_next) const {
    return do_out(state, from, from_end, from_next, to, to_end, to_next);
  }
  result unshift(mbstate_t& state, char8_t* to, char8_t* to_end, char8_t*& to_next) const {
    return do_unshift(state, to, to_end, to_next);
  }
  result in(mbstate_t& state, const char8_t* from, const char8_t* from_end, const char8_t*& from_next, char32_t* to,
            char32_t* to_end, char32_t*& to_next) const {
    return do_in(state, from, from_end, from_next, to, to_end, to_next);
  }
  int encoding() const noexcept { return do_encoding(); }
  bool always_noconv() const noexcept { return do_always_noconv(); }
  int length(mbstate_t& state, const char8_t* from, const char8_t* end, size_t max) const {
    return do_length(state, from, end, max);
  }
  int max_length() const noexcept { return do_max_length(); }

  static locale::id id;

protected:
  ~codecvt() override;
  virtual result do_out(mbstate_t& state, const char32_t* from, const char32_t* from_end, const char32_t*& from_next,
                        char8_t* to, char8_t* to_end, char8_t*& to_next) const;
  virtual result do_in(mbstate_t& state, const char8_t* from, const char8_t* from_end, const char8_t*& from_next,
                       char32_t* to, char32_t* to_end, char32_t*& to_next) const;
  virtual result do_unshift(mbstate_t& state, char8_t* to, char8_t* to_end, char8_t*& to_next) const;
  virtual int do_encoding() const noexcept;
  virtual bool do_always_noconv() const noexcept;
  virtual int do_length(mbstate_t&, const char8_t* from, const char8_t* end, size_t max) const;
  virtual int do_max_length() const noexcept;
};

// [depr.locale.category]: the deprecated UTF-16 / UTF-32 <-> UTF-8 conversions with char as the
// UTF-8 code unit.
template <>
class [[deprecated("codecvt<char16_t, char, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt<char16_t, char, mbstate_t> : public locale::facet, public codecvt_base {
public:
  using intern_type = char16_t;
  using extern_type = char;
  using state_type = mbstate_t;

  explicit codecvt(size_t refs = 0) : locale::facet(refs) {}
  result out(mbstate_t& state, const char16_t* from, const char16_t* from_end, const char16_t*& from_next,
             char* to, char* to_end, char*& to_next) const {
    return do_out(state, from, from_end, from_next, to, to_end, to_next);
  }
  result unshift(mbstate_t& state, char* to, char* to_end, char*& to_next) const {
    return do_unshift(state, to, to_end, to_next);
  }
  result in(mbstate_t& state, const char* from, const char* from_end, const char*& from_next, char16_t* to,
            char16_t* to_end, char16_t*& to_next) const {
    return do_in(state, from, from_end, from_next, to, to_end, to_next);
  }
  int encoding() const noexcept { return do_encoding(); }
  bool always_noconv() const noexcept { return do_always_noconv(); }
  int length(mbstate_t& state, const char* from, const char* end, size_t max) const {
    return do_length(state, from, end, max);
  }
  int max_length() const noexcept { return do_max_length(); }

  static locale::id id;

protected:
  ~codecvt() override;
  virtual result do_out(mbstate_t& state, const char16_t* from, const char16_t* from_end, const char16_t*& from_next,
                        char* to, char* to_end, char*& to_next) const;
  virtual result do_in(mbstate_t& state, const char* from, const char* from_end, const char*& from_next,
                       char16_t* to, char16_t* to_end, char16_t*& to_next) const;
  virtual result do_unshift(mbstate_t& state, char* to, char* to_end, char*& to_next) const;
  virtual int do_encoding() const noexcept;
  virtual bool do_always_noconv() const noexcept;
  virtual int do_length(mbstate_t&, const char* from, const char* end, size_t max) const;
  virtual int do_max_length() const noexcept;
};

template <>
class [[deprecated("codecvt<char32_t, char, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt<char32_t, char, mbstate_t> : public locale::facet, public codecvt_base {
public:
  using intern_type = char32_t;
  using extern_type = char;
  using state_type = mbstate_t;

  explicit codecvt(size_t refs = 0) : locale::facet(refs) {}
  result out(mbstate_t& state, const char32_t* from, const char32_t* from_end, const char32_t*& from_next,
             char* to, char* to_end, char*& to_next) const {
    return do_out(state, from, from_end, from_next, to, to_end, to_next);
  }
  result unshift(mbstate_t& state, char* to, char* to_end, char*& to_next) const {
    return do_unshift(state, to, to_end, to_next);
  }
  result in(mbstate_t& state, const char* from, const char* from_end, const char*& from_next, char32_t* to,
            char32_t* to_end, char32_t*& to_next) const {
    return do_in(state, from, from_end, from_next, to, to_end, to_next);
  }
  int encoding() const noexcept { return do_encoding(); }
  bool always_noconv() const noexcept { return do_always_noconv(); }
  int length(mbstate_t& state, const char* from, const char* end, size_t max) const {
    return do_length(state, from, end, max);
  }
  int max_length() const noexcept { return do_max_length(); }

  static locale::id id;

protected:
  ~codecvt() override;
  virtual result do_out(mbstate_t& state, const char32_t* from, const char32_t* from_end, const char32_t*& from_next,
                        char* to, char* to_end, char*& to_next) const;
  virtual result do_in(mbstate_t& state, const char* from, const char* from_end, const char*& from_next,
                       char32_t* to, char32_t* to_end, char32_t*& to_next) const;
  virtual result do_unshift(mbstate_t& state, char* to, char* to_end, char*& to_next) const;
  virtual int do_encoding() const noexcept;
  virtual bool do_always_noconv() const noexcept;
  virtual int do_length(mbstate_t&, const char* from, const char* end, size_t max) const;
  virtual int do_max_length() const noexcept;
};

// [locale.codecvt.byname]
template <class internT, class externT, class stateT>
class codecvt_byname : public codecvt<internT, externT, stateT> {
public:
  explicit codecvt_byname(const char* name, size_t refs = 0) : codecvt<internT, externT, stateT>(refs) {
    ::ycxx::detail::check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t refs = 0) : codecvt_byname(name.c_str(), refs) {}

protected:
  ~codecvt_byname() override {}
};
// The C library's multibyte conversion of the name's LC_CTYPE (mbrtowc/wcrtomb in the locale; src/
// hosted/locale_named.cpp); the classic UTF-8 conversion for the names with classic semantics.
template <>
class codecvt_byname<wchar_t, char, mbstate_t> : public codecvt<wchar_t, char, mbstate_t> {
public:
  explicit codecvt_byname(const char* name, size_t refs = 0)
      : codecvt(refs), named_(::ycxx::detail::named_open(name, locale::ctype, "std::codecvt_byname")) {}
  explicit codecvt_byname(const string& name, size_t refs = 0) : codecvt_byname(name.c_str(), refs) {}

protected:
  ~codecvt_byname() override;
  result do_out(mbstate_t& state, const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next, char* to,
                char* to_end, char*& to_next) const override;
  result do_in(mbstate_t& state, const char* from, const char* from_end, const char*& from_next, wchar_t* to,
               wchar_t* to_end, wchar_t*& to_next) const override;
  result do_unshift(mbstate_t& state, char* to, char* to_end, char*& to_next) const override;
  int do_encoding() const noexcept override;
  bool do_always_noconv() const noexcept override;
  int do_length(mbstate_t& state, const char* from, const char* end, size_t max) const override;
  int do_max_length() const noexcept override;

private:
  ycxx::detail::named_locale* named_;
};
// [depr.locale.category]/2: the Annex D codecvt_byname facets.
template <>
class [[deprecated("codecvt_byname<char16_t, char, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt_byname<char16_t, char, mbstate_t> : public codecvt<char16_t, char, mbstate_t> {
public:
  explicit codecvt_byname(const char* name, size_t refs = 0) : codecvt(refs) {
    ::ycxx::detail::check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t refs = 0) : codecvt_byname(name.c_str(), refs) {}

protected:
  ~codecvt_byname() override {}
};
template <>
class [[deprecated("codecvt_byname<char32_t, char, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt_byname<char32_t, char, mbstate_t> : public codecvt<char32_t, char, mbstate_t> {
public:
  explicit codecvt_byname(const char* name, size_t refs = 0) : codecvt(refs) {
    ::ycxx::detail::check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t refs = 0) : codecvt_byname(name.c_str(), refs) {}

protected:
  ~codecvt_byname() override {}
};
template <>
class [[deprecated("codecvt_byname<char16_t, char8_t, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt_byname<char16_t, char8_t, mbstate_t> : public codecvt<char16_t, char8_t, mbstate_t> {
public:
  explicit codecvt_byname(const char* name, size_t refs = 0) : codecvt(refs) {
    ::ycxx::detail::check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t refs = 0) : codecvt_byname(name.c_str(), refs) {}

protected:
  ~codecvt_byname() override {}
};
template <>
class [[deprecated("codecvt_byname<char32_t, char8_t, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt_byname<char32_t, char8_t, mbstate_t> : public codecvt<char32_t, char8_t, mbstate_t> {
public:
  explicit codecvt_byname(const char* name, size_t refs = 0) : codecvt(refs) {
    ::ycxx::detail::check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t refs = 0) : codecvt_byname(name.c_str(), refs) {}

protected:
  ~codecvt_byname() override {}
};

// ---- [locale.numpunct] ------------------------------------------------------------------------
template <class charT>
class numpunct : public locale::facet {
public:
  using char_type = charT;
  using string_type = basic_string<charT>;

  explicit numpunct(size_t refs = 0) : locale::facet(refs) {}

  char_type decimal_point() const { return do_decimal_point(); }
  char_type thousands_sep() const { return do_thousands_sep(); }
  string grouping() const { return do_grouping(); }
  string_type truename() const { return do_truename(); }
  string_type falsename() const { return do_falsename(); }

  static locale::id id;

protected:
  ~numpunct() override {}
  virtual char_type do_decimal_point() const { return charT('.'); }
  virtual char_type do_thousands_sep() const { return charT(','); }
  virtual string do_grouping() const { return string(); }
  virtual string_type do_truename() const { return ::ycxx::detail::widen_ascii<charT>("true"); }
  virtual string_type do_falsename() const { return ::ycxx::detail::widen_ascii<charT>("false"); }
};
template <class charT>
locale::id numpunct<charT>::id;

// [locale.numpunct.byname]
template <class charT>
class numpunct_byname : public numpunct<charT> {
public:
  using char_type = charT;
  using string_type = basic_string<charT>;
  // char and wchar_t: LC_NUMERIC's radix character, separator and grouping (read once, here);
  // truename()/falsename() stay "true"/"false" (the C library has no such names). Other
  // character types: the classic values.
  explicit numpunct_byname(const char* name, size_t refs = 0) : numpunct<charT>(refs) {
    if constexpr (is_same_v<charT, char> || is_same_v<charT, wchar_t>)
      ::ycxx::detail::named_numpunct(name, point_, sep_, grouping_);
    else
      ::ycxx::detail::check_locale_name(name, "std::numpunct_byname");
  }
  explicit numpunct_byname(const string& name, size_t refs = 0) : numpunct_byname(name.c_str(), refs) {}

protected:
  ~numpunct_byname() override {}
  charT do_decimal_point() const override { return point_; }
  charT do_thousands_sep() const override { return sep_; }
  string do_grouping() const override { return grouping_; }

private:
  charT point_ = charT('.');
  charT sep_ = charT(',');
  string grouping_;
};

// ---- [locale.collate] -------------------------------------------------------------------------
template <class charT>
class collate : public locale::facet {
public:
  using char_type = charT;
  using string_type = basic_string<charT>;

  explicit collate(size_t refs = 0) : locale::facet(refs) {}

  int compare(const charT* low1, const charT* high1, const charT* low2, const charT* high2) const {
    return do_compare(low1, high1, low2, high2);
  }
  string_type transform(const charT* low, const charT* high) const { return do_transform(low, high); }
  long hash(const charT* low, const charT* high) const { return do_hash(low, high); }

  static locale::id id;

protected:
  ~collate() override {}
  // Lexicographical comparison of the character values (char as unsigned char, as strcmp).
  virtual int do_compare(const charT* low1, const charT* high1, const charT* low2, const charT* high2) const {
    using U = conditional_t<is_same_v<charT, char>, unsigned char, charT>;
    for (; low1 != high1 && low2 != high2; ++low1, ++low2) {
      const U a = static_cast<U>(*low1), b = static_cast<U>(*low2);
      if (a < b)
        return -1;
      if (b < a)
        return 1;
    }
    return low2 != high2 ? -1 : (low1 != high1 ? 1 : 0);
  }
  virtual string_type do_transform(const charT* low, const charT* high) const { return string_type(low, high); }
  virtual long do_hash(const charT* low, const charT* high) const {
    unsigned long h = 14695981039346656037ul; // FNV-1a over the character values
    for (; low != high; ++low) {
      h ^= static_cast<unsigned long>(static_cast<make_unsigned_t<charT>>(*low));
      h *= 1099511628211ul;
    }
    return static_cast<long>(h);
  }
};
template <class charT>
locale::id collate<charT>::id;

// [locale.collate.byname]
template <class charT>
class collate_byname : public collate<charT> {
public:
  using string_type = basic_string<charT>;
  explicit collate_byname(const char* name, size_t refs = 0) : collate<charT>(refs) {
    ::ycxx::detail::check_locale_name(name, "std::collate_byname");
  }
  explicit collate_byname(const string& name, size_t refs = 0) : collate_byname(name.c_str(), refs) {}

protected:
  ~collate_byname() override {}
};
// LC_COLLATE of the name: strcoll_l/strxfrm_l (wcscoll_l/wcsxfrm_l), each run of characters
// between embedded null characters in turn; hash() hashes transform() (src/hosted/locale_named.cpp).
template <>
class collate_byname<char> : public collate<char> {
public:
  using string_type = string;
  explicit collate_byname(const char* name, size_t refs = 0)
      : collate(refs), named_(::ycxx::detail::named_open(name, locale::collate, "std::collate_byname")) {}
  explicit collate_byname(const string& name, size_t refs = 0) : collate_byname(name.c_str(), refs) {}

protected:
  ~collate_byname() override;
  int do_compare(const char* low1, const char* high1, const char* low2, const char* high2) const override;
  string_type do_transform(const char* low, const char* high) const override;
  long do_hash(const char* low, const char* high) const override;

private:
  ycxx::detail::named_locale* named_;
};
template <>
class collate_byname<wchar_t> : public collate<wchar_t> {
public:
  using string_type = wstring;
  explicit collate_byname(const char* name, size_t refs = 0)
      : collate(refs), named_(::ycxx::detail::named_open(name, locale::collate, "std::collate_byname")) {}
  explicit collate_byname(const string& name, size_t refs = 0) : collate_byname(name.c_str(), refs) {}

protected:
  ~collate_byname() override;
  int do_compare(const wchar_t* low1, const wchar_t* high1, const wchar_t* low2, const wchar_t* high2) const override;
  string_type do_transform(const wchar_t* low, const wchar_t* high) const override;
  long do_hash(const wchar_t* low, const wchar_t* high) const override;

private:
  ycxx::detail::named_locale* named_;
};

template <class charT, class traits, class Allocator>
bool locale::operator()(const basic_string<charT, traits, Allocator>& s1,
                        const basic_string<charT, traits, Allocator>& s2) const {
  return use_facet<std::collate<charT>>(*this).compare(s1.data(), s1.data() + s1.size(), s2.data(),
                                                       s2.data() + s2.size()) < 0;
}

// ---- [locale.convenience] ---------------------------------------------------------------------
template <class charT>
bool isspace(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::space, c);
}
template <class charT>
bool isprint(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::print, c);
}
template <class charT>
bool iscntrl(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::cntrl, c);
}
template <class charT>
bool isupper(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::upper, c);
}
template <class charT>
bool islower(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::lower, c);
}
template <class charT>
bool isalpha(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::alpha, c);
}
template <class charT>
bool isdigit(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::digit, c);
}
template <class charT>
bool ispunct(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::punct, c);
}
template <class charT>
bool isxdigit(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::xdigit, c);
}
template <class charT>
bool isalnum(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::alnum, c);
}
template <class charT>
bool isgraph(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::graph, c);
}
template <class charT>
bool isblank(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).is(ctype_base::blank, c);
}
template <class charT>
charT toupper(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).toupper(c);
}
template <class charT>
charT tolower(charT c, const locale& loc) {
  return use_facet<ctype<charT>>(loc).tolower(c);
}

} // namespace std
