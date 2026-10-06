// libycxx hosted: the locale machinery and the facets the iostreams need everywhere
// ([locales], [category.ctype], [category.numeric] numpunct, [category.collate]).
//
// locale is a pointer to a reference-counted __ycxx::__detail::__locale_impl (opaque here; defined in
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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
struct __locale_impl; // src/hosted/locale.cpp
struct __locale_access;
// Selects locale's private constructor from a locale_impl*: without it, a null pointer constant
// would also convert to that constructor's parameter and make locale(nullptr) ambiguous.
struct __locale_impl_tag {};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

class locale;
struct text_encoding; // <text_encoding>; <locale> includes it
template <class _Facet>
const _Facet& use_facet(const locale&);
template <class _Facet>
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
  explicit locale(const char* __std_name);
  explicit locale(const string& __std_name) : locale(__std_name.c_str()) {}
  locale(const locale& other, const char* __std_name, category __cats);
  locale(const locale& other, const string& __std_name, category __cats) : locale(other, __std_name.c_str(), __cats) {}
  template <class _Facet>
  locale(const locale& other, _Facet* __f);
  locale(const locale& other, const locale& __one, category __cats);
  ~locale();
  const locale& operator=(const locale& other) noexcept;

  template <class _Facet>
  locale combine(const locale& other) const;
  string name() const;
  text_encoding encoding() const; // src/hosted/text_encoding.cpp
  bool operator==(const locale& other) const;
  template <class __charT, class __traits, class _Allocator>
  bool operator()(const basic_string<__charT, __traits, _Allocator>& __s1, const basic_string<__charT, __traits, _Allocator>& __s2) const;

  static locale global(const locale& __loc);
  static const locale& classic();

private:
  friend __ycxx::__detail::__locale_access;
  template <class _Facet>
  friend const _Facet& use_facet(const locale&);
  template <class _Facet>
  friend bool has_facet(const locale&) noexcept;

  locale(__ycxx::__detail::__locale_impl_tag, __ycxx::__detail::__locale_impl* __y_impl) noexcept : __impl_(__y_impl) {}
  // A copy of other with f installed under index i (null f: a copy of other).
  locale(const locale& other, const facet* __f, const id& i);
  // The facet under index i, or null.
  const facet* find(const id& i) const noexcept;

  __ycxx::__detail::__locale_impl* __impl_;
};

// [locale.facet]
class locale::facet {
protected:
  explicit facet(size_t __refs = 0) noexcept : __refs_(__refs) {}
  virtual ~facet();
  facet(const facet&) = delete;
  void operator=(const facet&) = delete;

private:
  friend __ycxx::__detail::__locale_access;
  mutable size_t __refs_; // refs, plus one per locale holding the facet (atomic)
};

// [locale.id]
class locale::id {
public:
  constexpr id() noexcept {}
  void operator=(const id&) = delete;
  id(const id&) = delete;

private:
  friend locale;
  friend __ycxx::__detail::__locale_access;
  // The index, assigned on first use (0: not yet assigned).
  size_t index() const noexcept {
    size_t i = __atomic_load_n(&__index_, __ATOMIC_ACQUIRE);
    return i != 0 ? i : assign();
  }
  size_t assign() const noexcept;
  mutable size_t __index_ = 0;
};

template <class _Facet>
locale::locale(const locale& other, _Facet* __f) : locale(other, __f, _Facet::id) {}

template <class _Facet>
locale locale::combine(const locale& other) const {
  const facet* __f = other.find(_Facet::id);
  if (__f == nullptr)
    ::__ycxx::__detail::__throw_runtime_error("std::locale::combine: the facet is not present in the other locale");
  return locale(*this, __f, _Facet::id);
}

// [locale.global.templates]
template <class _Facet>
const _Facet& use_facet(const locale& __loc) {
  const locale::facet* __f = __loc.find(_Facet::id);
  if (__f == nullptr)
    ::__ycxx::__detail::__raise_with(ycxx_error_bad_cast, "std::use_facet: the facet is not present in the locale",
                               [] { return bad_cast(); });
  return static_cast<const _Facet&>(*__f);
}
template <class _Facet>
bool has_facet(const locale& __loc) noexcept {
  return __loc.find(_Facet::id) != nullptr;
}

// ---- [locale.syn]: the default template arguments, declared once --------------------------------
template <class __charT, class _InputIterator = istreambuf_iterator<__charT>>
class num_get;
template <class __charT, class _OutputIterator = ostreambuf_iterator<__charT>>
class num_put;
template <class __charT, class _InputIterator = istreambuf_iterator<__charT>>
class time_get;
template <class __charT, class _InputIterator = istreambuf_iterator<__charT>>
class time_get_byname;
template <class __charT, class _OutputIterator = ostreambuf_iterator<__charT>>
class time_put;
template <class __charT, class _OutputIterator = ostreambuf_iterator<__charT>>
class time_put_byname;
template <class __charT, class _InputIterator = istreambuf_iterator<__charT>>
class money_get;
template <class __charT, class _OutputIterator = ostreambuf_iterator<__charT>>
class money_put;
template <class __charT, bool _Intl = false>
class moneypunct;
template <class __charT, bool _Intl = false>
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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// The "C" classification of the 128 ASCII characters.
consteval auto __make_ascii_masks() {
  using _Bp = std::ctype_base;
  struct table {
    _Bp::mask m[128];
  } t{};
  for (int c = 0; c < 128; ++c) {
    _Bp::mask m = 0;
    if (c < 32 || c == 127)
      m |= _Bp::cntrl;
    if (c == ' ' || (c >= '\t' && c <= '\r'))
      m |= _Bp::space;
    if (c == ' ' || c == '\t')
      m |= _Bp::blank;
    if (c >= 32 && c < 127)
      m |= _Bp::print;
    if (c >= 'A' && c <= 'Z')
      m |= _Bp::upper | _Bp::alpha;
    if (c >= 'a' && c <= 'z')
      m |= _Bp::lower | _Bp::alpha;
    if (c >= '0' && c <= '9')
      m |= _Bp::digit;
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
      m |= _Bp::xdigit;
    if (c > 32 && c < 127 && !(m & (_Bp::alpha | _Bp::digit)))
      m |= _Bp::punct;
    t.m[c] = m;
  }
  return t;
}
inline constexpr auto __ascii_masks = ::__ycxx::__detail::__make_ascii_masks();

template <class __charT>
constexpr std::ctype_base::mask __classic_mask(__charT c) noexcept {
  using _Up = std::make_unsigned_t<__charT>;
  const _Up __u = static_cast<_Up>(c);
  return __u < 128 ? __ascii_masks.m[__u] : std::ctype_base::mask(0);
}
template <class __charT>
constexpr __charT __ascii_toupper(__charT c) noexcept {
  return c >= __charT('a') && c <= __charT('z') ? __charT(c - __charT('a') + __charT('A')) : c;
}
template <class __charT>
constexpr __charT __ascii_tolower(__charT c) noexcept {
  return c >= __charT('A') && c <= __charT('Z') ? __charT(c - __charT('A') + __charT('a')) : c;
}

// A string of the library's ASCII literal widened to charT (numpunct names and the like).
template <class __charT>
std::basic_string<__charT> __widen_ascii(const char* s) {
  std::basic_string<__charT> r(__builtin_strlen(s), __charT());
  for (std::size_t i = 0; i < r.size(); ++i)
    r[i] = static_cast<__charT>(s[i]);
  return r;
}

// Accepts the locale names with classic semantics (see the file comment); throws runtime_error
// for any other name, null included. Used by the _byname facets. Defined in the hosted runtime.
void __check_locale_name(const char* name, const char* what);

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [locale.ctype]
template <class __charT>
class ctype : public locale::facet, public ctype_base {
public:
  using char_type = __charT;

  explicit ctype(size_t __refs = 0) : locale::facet(__refs) {}

  bool is(mask m, __charT c) const { return do_is(m, c); }
  const __charT* is(const __charT* __low, const __charT* __high, mask* vec) const { return do_is(__low, __high, vec); }
  const __charT* scan_is(mask m, const __charT* __low, const __charT* __high) const { return do_scan_is(m, __low, __high); }
  const __charT* scan_not(mask m, const __charT* __low, const __charT* __high) const { return do_scan_not(m, __low, __high); }
  __charT toupper(__charT c) const { return do_toupper(c); }
  const __charT* toupper(__charT* __low, const __charT* __high) const { return do_toupper(__low, __high); }
  __charT tolower(__charT c) const { return do_tolower(c); }
  const __charT* tolower(__charT* __low, const __charT* __high) const { return do_tolower(__low, __high); }
  __charT widen(char c) const { return do_widen(c); }
  const char* widen(const char* __low, const char* __high, __charT* to) const { return do_widen(__low, __high, to); }
  char narrow(__charT c, char __dfault) const { return do_narrow(c, __dfault); }
  const __charT* narrow(const __charT* __low, const __charT* __high, char __dfault, char* to) const {
    return do_narrow(__low, __high, __dfault, to);
  }

  static locale::id id;

protected:
  ~ctype() override {}
  virtual bool do_is(mask m, __charT c) const { return (::__ycxx::__detail::__classic_mask(c) & m) != 0; }
  virtual const __charT* do_is(const __charT* __low, const __charT* __high, mask* vec) const {
    for (; __low != __high; ++__low, ++vec)
      *vec = ::__ycxx::__detail::__classic_mask(*__low);
    return __high;
  }
  virtual const __charT* do_scan_is(mask m, const __charT* __low, const __charT* __high) const {
    while (__low != __high && !is(m, *__low))
      ++__low;
    return __low;
  }
  virtual const __charT* do_scan_not(mask m, const __charT* __low, const __charT* __high) const {
    while (__low != __high && is(m, *__low))
      ++__low;
    return __low;
  }
  virtual __charT do_toupper(__charT c) const { return ::__ycxx::__detail::__ascii_toupper(c); }
  virtual const __charT* do_toupper(__charT* __low, const __charT* __high) const {
    for (; __low != __high; ++__low)
      *__low = ::__ycxx::__detail::__ascii_toupper(*__low);
    return __high;
  }
  virtual __charT do_tolower(__charT c) const { return ::__ycxx::__detail::__ascii_tolower(c); }
  virtual const __charT* do_tolower(__charT* __low, const __charT* __high) const {
    for (; __low != __high; ++__low)
      *__low = ::__ycxx::__detail::__ascii_tolower(*__low);
    return __high;
  }
  virtual __charT do_widen(char c) const { return static_cast<__charT>(static_cast<unsigned char>(c)); }
  virtual const char* do_widen(const char* __low, const char* __high, __charT* __dest) const {
    for (; __low != __high; ++__low, ++__dest)
      *__dest = do_widen(*__low);
    return __high;
  }
  virtual char do_narrow(__charT c, char __dfault) const {
    using _Up = make_unsigned_t<__charT>;
    return static_cast<_Up>(c) < 256 ? static_cast<char>(static_cast<unsigned char>(c)) : __dfault;
  }
  virtual const __charT* do_narrow(const __charT* __low, const __charT* __high, char __dfault, char* __dest) const {
    for (; __low != __high; ++__low, ++__dest)
      *__dest = do_narrow(*__low, __dfault);
    return __high;
  }
};
template <class __charT>
locale::id ctype<__charT>::id;

// [facet.ctype.special]
template <>
class ctype<char> : public locale::facet, public ctype_base {
public:
  using char_type = char;

  explicit ctype(const mask* __tbl = nullptr, bool __del = false, size_t __refs = 0)
      : locale::facet(__refs), __table_(__tbl ? __tbl : classic_table()), __del_(__tbl != nullptr && __del) {}

  bool is(mask m, char c) const { return (__table_[static_cast<unsigned char>(c)] & m) != 0; }
  const char* is(const char* __low, const char* __high, mask* vec) const {
    for (; __low != __high; ++__low, ++vec)
      *vec = __table_[static_cast<unsigned char>(*__low)];
    return __high;
  }
  const char* scan_is(mask m, const char* __low, const char* __high) const {
    while (__low != __high && !(__table_[static_cast<unsigned char>(*__low)] & m))
      ++__low;
    return __low;
  }
  const char* scan_not(mask m, const char* __low, const char* __high) const {
    while (__low != __high && (__table_[static_cast<unsigned char>(*__low)] & m))
      ++__low;
    return __low;
  }
  char toupper(char c) const { return do_toupper(c); }
  const char* toupper(char* __low, const char* __high) const { return do_toupper(__low, __high); }
  char tolower(char c) const { return do_tolower(c); }
  const char* tolower(char* __low, const char* __high) const { return do_tolower(__low, __high); }
  char widen(char c) const { return do_widen(c); }
  const char* widen(const char* __low, const char* __high, char* to) const { return do_widen(__low, __high, to); }
  char narrow(char c, char __dfault) const { return do_narrow(c, __dfault); }
  const char* narrow(const char* __low, const char* __high, char __dfault, char* to) const {
    return do_narrow(__low, __high, __dfault, to);
  }

  static locale::id id;
  static constexpr size_t table_size = 256;
  const mask* table() const noexcept { return __table_; }
  static const mask* classic_table() noexcept;

protected:
  ~ctype() override;
  virtual char do_toupper(char c) const { return ::__ycxx::__detail::__ascii_toupper(c); }
  virtual const char* do_toupper(char* __low, const char* __high) const {
    for (; __low != __high; ++__low)
      *__low = ::__ycxx::__detail::__ascii_toupper(*__low);
    return __high;
  }
  virtual char do_tolower(char c) const { return ::__ycxx::__detail::__ascii_tolower(c); }
  virtual const char* do_tolower(char* __low, const char* __high) const {
    for (; __low != __high; ++__low)
      *__low = ::__ycxx::__detail::__ascii_tolower(*__low);
    return __high;
  }
  virtual char do_widen(char c) const { return c; }
  virtual const char* do_widen(const char* __low, const char* __high, char* __dest) const {
    if (__low != __high)
      __builtin_memmove(__dest, __low, static_cast<size_t>(__high - __low));
    return __high;
  }
  virtual char do_narrow(char c, char) const { return c; }
  virtual const char* do_narrow(const char* __low, const char* __high, char, char* __dest) const {
    if (__low != __high)
      __builtin_memmove(__dest, __low, static_cast<size_t>(__high - __low));
    return __high;
  }

private:
  const mask* __table_;
  bool __del_;
};

// [locale.ctype.byname]
template <class __charT>
class ctype_byname : public ctype<__charT> {
public:
  using mask = typename ctype<__charT>::mask;
  explicit ctype_byname(const char* name, size_t __refs = 0) : ctype<__charT>(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::ctype_byname");
  }
  explicit ctype_byname(const string& name, size_t __refs = 0) : ctype_byname(name.c_str(), __refs) {}

protected:
  ~ctype_byname() override {}
};
template <>
class ctype_byname<char> : public ctype<char> {
public:
  explicit ctype_byname(const char* name, size_t __refs = 0) : ctype<char>(nullptr, false, __refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::ctype_byname");
  }
  explicit ctype_byname(const string& name, size_t __refs = 0) : ctype_byname(name.c_str(), __refs) {}

protected:
  ~ctype_byname() override {}
};

// [locale.codecvt]
class codecvt_base {
public:
  enum result { ok, partial, error, noconv };
};

// The primary template: a degenerate (noconv) conversion, as codecvt<char, char, mbstate_t>
// ([locale.codecvt.general]/3); a program specializes it for its own state types.
template <class __internT, class __externT, class __stateT>
class codecvt : public locale::facet, public codecvt_base {
public:
  using intern_type = __internT;
  using extern_type = __externT;
  using state_type = __stateT;

  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}

  result out(__stateT& state, const __internT* from, const __internT* __from_end, const __internT*& __from_next, __externT* to,
             __externT* __to_end, __externT*& __to_next) const {
    return do_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
  }
  result unshift(__stateT& state, __externT* to, __externT* __to_end, __externT*& __to_next) const {
    return do_unshift(state, to, __to_end, __to_next);
  }
  result in(__stateT& state, const __externT* from, const __externT* __from_end, const __externT*& __from_next, __internT* to,
            __internT* __to_end, __internT*& __to_next) const {
    return do_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
  }
  int encoding() const noexcept { return do_encoding(); }
  bool always_noconv() const noexcept { return do_always_noconv(); }
  int length(__stateT& state, const __externT* from, const __externT* end, size_t max) const {
    return do_length(state, from, end, max);
  }
  int max_length() const noexcept { return do_max_length(); }

  static locale::id id;

protected:
  ~codecvt() override {}
  virtual result do_out(__stateT&, const __internT* from, const __internT*, const __internT*& __from_next, __externT* to,
                        __externT*, __externT*& __to_next) const {
    __from_next = from;
    __to_next = to;
    return noconv;
  }
  virtual result do_in(__stateT&, const __externT* from, const __externT*, const __externT*& __from_next, __internT* to,
                       __internT*, __internT*& __to_next) const {
    __from_next = from;
    __to_next = to;
    return noconv;
  }
  virtual result do_unshift(__stateT&, __externT* to, __externT*, __externT*& __to_next) const {
    __to_next = to;
    return noconv;
  }
  virtual int do_encoding() const noexcept { return 1; }
  virtual bool do_always_noconv() const noexcept { return true; }
  virtual int do_length(__stateT&, const __externT* from, const __externT* end, size_t max) const {
    const size_t n = static_cast<size_t>(end - from);
    return static_cast<int>(n < max ? n : max);
  }
  virtual int do_max_length() const noexcept { return 1; }
};
template <class __internT, class __externT, class __stateT>
locale::id codecvt<__internT, __externT, __stateT>::id;

// The four required specializations on mbstate_t share this shape; their members are defined in
// the hosted runtime.
template <>
class codecvt<char, char, mbstate_t> : public locale::facet, public codecvt_base {
public:
  using intern_type = char;
  using extern_type = char;
  using state_type = mbstate_t;

  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}
  result out(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next, char* to,
             char* __to_end, char*& __to_next) const {
    return do_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
  }
  result unshift(mbstate_t& state, char* to, char* __to_end, char*& __to_next) const {
    return do_unshift(state, to, __to_end, __to_next);
  }
  result in(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next, char* to, char* __to_end,
            char*& __to_next) const {
    return do_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
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
  virtual result do_out(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next, char* to,
                        char* __to_end, char*& __to_next) const;
  virtual result do_in(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next, char* to,
                       char* __to_end, char*& __to_next) const;
  virtual result do_unshift(mbstate_t& state, char* to, char* __to_end, char*& __to_next) const;
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

  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}
  result out(mbstate_t& state, const wchar_t* from, const wchar_t* __from_end, const wchar_t*& __from_next, char* to,
             char* __to_end, char*& __to_next) const {
    return do_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
  }
  result unshift(mbstate_t& state, char* to, char* __to_end, char*& __to_next) const {
    return do_unshift(state, to, __to_end, __to_next);
  }
  result in(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next, wchar_t* to,
            wchar_t* __to_end, wchar_t*& __to_next) const {
    return do_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
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
  virtual result do_out(mbstate_t& state, const wchar_t* from, const wchar_t* __from_end, const wchar_t*& __from_next,
                        char* to, char* __to_end, char*& __to_next) const;
  virtual result do_in(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next, wchar_t* to,
                       wchar_t* __to_end, wchar_t*& __to_next) const;
  virtual result do_unshift(mbstate_t& state, char* to, char* __to_end, char*& __to_next) const;
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

  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}
  result out(mbstate_t& state, const char16_t* from, const char16_t* __from_end, const char16_t*& __from_next,
             char8_t* to, char8_t* __to_end, char8_t*& __to_next) const {
    return do_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
  }
  result unshift(mbstate_t& state, char8_t* to, char8_t* __to_end, char8_t*& __to_next) const {
    return do_unshift(state, to, __to_end, __to_next);
  }
  result in(mbstate_t& state, const char8_t* from, const char8_t* __from_end, const char8_t*& __from_next, char16_t* to,
            char16_t* __to_end, char16_t*& __to_next) const {
    return do_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
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
  virtual result do_out(mbstate_t& state, const char16_t* from, const char16_t* __from_end, const char16_t*& __from_next,
                        char8_t* to, char8_t* __to_end, char8_t*& __to_next) const;
  virtual result do_in(mbstate_t& state, const char8_t* from, const char8_t* __from_end, const char8_t*& __from_next,
                       char16_t* to, char16_t* __to_end, char16_t*& __to_next) const;
  virtual result do_unshift(mbstate_t& state, char8_t* to, char8_t* __to_end, char8_t*& __to_next) const;
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

  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}
  result out(mbstate_t& state, const char32_t* from, const char32_t* __from_end, const char32_t*& __from_next,
             char8_t* to, char8_t* __to_end, char8_t*& __to_next) const {
    return do_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
  }
  result unshift(mbstate_t& state, char8_t* to, char8_t* __to_end, char8_t*& __to_next) const {
    return do_unshift(state, to, __to_end, __to_next);
  }
  result in(mbstate_t& state, const char8_t* from, const char8_t* __from_end, const char8_t*& __from_next, char32_t* to,
            char32_t* __to_end, char32_t*& __to_next) const {
    return do_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
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
  virtual result do_out(mbstate_t& state, const char32_t* from, const char32_t* __from_end, const char32_t*& __from_next,
                        char8_t* to, char8_t* __to_end, char8_t*& __to_next) const;
  virtual result do_in(mbstate_t& state, const char8_t* from, const char8_t* __from_end, const char8_t*& __from_next,
                       char32_t* to, char32_t* __to_end, char32_t*& __to_next) const;
  virtual result do_unshift(mbstate_t& state, char8_t* to, char8_t* __to_end, char8_t*& __to_next) const;
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

  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}
  result out(mbstate_t& state, const char16_t* from, const char16_t* __from_end, const char16_t*& __from_next,
             char* to, char* __to_end, char*& __to_next) const {
    return do_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
  }
  result unshift(mbstate_t& state, char* to, char* __to_end, char*& __to_next) const {
    return do_unshift(state, to, __to_end, __to_next);
  }
  result in(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next, char16_t* to,
            char16_t* __to_end, char16_t*& __to_next) const {
    return do_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
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
  virtual result do_out(mbstate_t& state, const char16_t* from, const char16_t* __from_end, const char16_t*& __from_next,
                        char* to, char* __to_end, char*& __to_next) const;
  virtual result do_in(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next,
                       char16_t* to, char16_t* __to_end, char16_t*& __to_next) const;
  virtual result do_unshift(mbstate_t& state, char* to, char* __to_end, char*& __to_next) const;
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

  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}
  result out(mbstate_t& state, const char32_t* from, const char32_t* __from_end, const char32_t*& __from_next,
             char* to, char* __to_end, char*& __to_next) const {
    return do_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
  }
  result unshift(mbstate_t& state, char* to, char* __to_end, char*& __to_next) const {
    return do_unshift(state, to, __to_end, __to_next);
  }
  result in(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next, char32_t* to,
            char32_t* __to_end, char32_t*& __to_next) const {
    return do_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
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
  virtual result do_out(mbstate_t& state, const char32_t* from, const char32_t* __from_end, const char32_t*& __from_next,
                        char* to, char* __to_end, char*& __to_next) const;
  virtual result do_in(mbstate_t& state, const char* from, const char* __from_end, const char*& __from_next,
                       char32_t* to, char32_t* __to_end, char32_t*& __to_next) const;
  virtual result do_unshift(mbstate_t& state, char* to, char* __to_end, char*& __to_next) const;
  virtual int do_encoding() const noexcept;
  virtual bool do_always_noconv() const noexcept;
  virtual int do_length(mbstate_t&, const char* from, const char* end, size_t max) const;
  virtual int do_max_length() const noexcept;
};

// [locale.codecvt.byname]
template <class __internT, class __externT, class __stateT>
class codecvt_byname : public codecvt<__internT, __externT, __stateT> {
public:
  explicit codecvt_byname(const char* name, size_t __refs = 0) : codecvt<__internT, __externT, __stateT>(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t __refs = 0) : codecvt_byname(name.c_str(), __refs) {}

protected:
  ~codecvt_byname() override {}
};
// [depr.locale.category]/2: the Annex D codecvt_byname facets.
template <>
class [[deprecated("codecvt_byname<char16_t, char, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt_byname<char16_t, char, mbstate_t> : public codecvt<char16_t, char, mbstate_t> {
public:
  explicit codecvt_byname(const char* name, size_t __refs = 0) : codecvt(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t __refs = 0) : codecvt_byname(name.c_str(), __refs) {}

protected:
  ~codecvt_byname() override {}
};
template <>
class [[deprecated("codecvt_byname<char32_t, char, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt_byname<char32_t, char, mbstate_t> : public codecvt<char32_t, char, mbstate_t> {
public:
  explicit codecvt_byname(const char* name, size_t __refs = 0) : codecvt(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t __refs = 0) : codecvt_byname(name.c_str(), __refs) {}

protected:
  ~codecvt_byname() override {}
};
template <>
class [[deprecated("codecvt_byname<char16_t, char8_t, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt_byname<char16_t, char8_t, mbstate_t> : public codecvt<char16_t, char8_t, mbstate_t> {
public:
  explicit codecvt_byname(const char* name, size_t __refs = 0) : codecvt(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t __refs = 0) : codecvt_byname(name.c_str(), __refs) {}

protected:
  ~codecvt_byname() override {}
};
template <>
class [[deprecated("codecvt_byname<char32_t, char8_t, mbstate_t> is deprecated ([depr.locale.category])")]]
codecvt_byname<char32_t, char8_t, mbstate_t> : public codecvt<char32_t, char8_t, mbstate_t> {
public:
  explicit codecvt_byname(const char* name, size_t __refs = 0) : codecvt(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::codecvt_byname");
  }
  explicit codecvt_byname(const string& name, size_t __refs = 0) : codecvt_byname(name.c_str(), __refs) {}

protected:
  ~codecvt_byname() override {}
};

// ---- [locale.numpunct] ------------------------------------------------------------------------
template <class __charT>
class numpunct : public locale::facet {
public:
  using char_type = __charT;
  using string_type = basic_string<__charT>;

  explicit numpunct(size_t __refs = 0) : locale::facet(__refs) {}

  char_type decimal_point() const { return do_decimal_point(); }
  char_type thousands_sep() const { return do_thousands_sep(); }
  string grouping() const { return do_grouping(); }
  string_type truename() const { return do_truename(); }
  string_type falsename() const { return do_falsename(); }

  static locale::id id;

protected:
  ~numpunct() override {}
  virtual char_type do_decimal_point() const { return __charT('.'); }
  virtual char_type do_thousands_sep() const { return __charT(','); }
  virtual string do_grouping() const { return string(); }
  virtual string_type do_truename() const { return ::__ycxx::__detail::__widen_ascii<__charT>("true"); }
  virtual string_type do_falsename() const { return ::__ycxx::__detail::__widen_ascii<__charT>("false"); }
};
template <class __charT>
locale::id numpunct<__charT>::id;

// [locale.numpunct.byname]
template <class __charT>
class numpunct_byname : public numpunct<__charT> {
public:
  using char_type = __charT;
  using string_type = basic_string<__charT>;
  explicit numpunct_byname(const char* name, size_t __refs = 0) : numpunct<__charT>(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::numpunct_byname");
  }
  explicit numpunct_byname(const string& name, size_t __refs = 0) : numpunct_byname(name.c_str(), __refs) {}

protected:
  ~numpunct_byname() override {}
};

// ---- [locale.collate] -------------------------------------------------------------------------
template <class __charT>
class collate : public locale::facet {
public:
  using char_type = __charT;
  using string_type = basic_string<__charT>;

  explicit collate(size_t __refs = 0) : locale::facet(__refs) {}

  int compare(const __charT* __low1, const __charT* __high1, const __charT* __low2, const __charT* __high2) const {
    return do_compare(__low1, __high1, __low2, __high2);
  }
  string_type transform(const __charT* __low, const __charT* __high) const { return do_transform(__low, __high); }
  long hash(const __charT* __low, const __charT* __high) const { return do_hash(__low, __high); }

  static locale::id id;

protected:
  ~collate() override {}
  // Lexicographical comparison of the character values (char as unsigned char, as strcmp).
  virtual int do_compare(const __charT* __low1, const __charT* __high1, const __charT* __low2, const __charT* __high2) const {
    using _Up = conditional_t<is_same_v<__charT, char>, unsigned char, __charT>;
    for (; __low1 != __high1 && __low2 != __high2; ++__low1, ++__low2) {
      const _Up a = static_cast<_Up>(*__low1), b = static_cast<_Up>(*__low2);
      if (a < b)
        return -1;
      if (b < a)
        return 1;
    }
    return __low2 != __high2 ? -1 : (__low1 != __high1 ? 1 : 0);
  }
  virtual string_type do_transform(const __charT* __low, const __charT* __high) const { return string_type(__low, __high); }
  virtual long do_hash(const __charT* __low, const __charT* __high) const {
    unsigned long h = 14695981039346656037ul; // FNV-1a over the character values
    for (; __low != __high; ++__low) {
      h ^= static_cast<unsigned long>(static_cast<make_unsigned_t<__charT>>(*__low));
      h *= 1099511628211ul;
    }
    return static_cast<long>(h);
  }
};
template <class __charT>
locale::id collate<__charT>::id;

// [locale.collate.byname]
template <class __charT>
class collate_byname : public collate<__charT> {
public:
  using string_type = basic_string<__charT>;
  explicit collate_byname(const char* name, size_t __refs = 0) : collate<__charT>(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::collate_byname");
  }
  explicit collate_byname(const string& name, size_t __refs = 0) : collate_byname(name.c_str(), __refs) {}

protected:
  ~collate_byname() override {}
};

template <class __charT, class __traits, class _Allocator>
bool locale::operator()(const basic_string<__charT, __traits, _Allocator>& __s1,
                        const basic_string<__charT, __traits, _Allocator>& __s2) const {
  return use_facet<std::collate<__charT>>(*this).compare(__s1.data(), __s1.data() + __s1.size(), __s2.data(),
                                                       __s2.data() + __s2.size()) < 0;
}

// ---- [locale.convenience] ---------------------------------------------------------------------
template <class __charT>
bool isspace(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::space, c);
}
template <class __charT>
bool isprint(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::print, c);
}
template <class __charT>
bool iscntrl(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::cntrl, c);
}
template <class __charT>
bool isupper(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::upper, c);
}
template <class __charT>
bool islower(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::lower, c);
}
template <class __charT>
bool isalpha(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::alpha, c);
}
template <class __charT>
bool isdigit(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::digit, c);
}
template <class __charT>
bool ispunct(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::punct, c);
}
template <class __charT>
bool isxdigit(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::xdigit, c);
}
template <class __charT>
bool isalnum(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::alnum, c);
}
template <class __charT>
bool isgraph(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::graph, c);
}
template <class __charT>
bool isblank(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).is(ctype_base::blank, c);
}
template <class __charT>
__charT toupper(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).toupper(c);
}
template <class __charT>
__charT tolower(__charT c, const locale& __loc) {
  return use_facet<ctype<__charT>>(__loc).tolower(c);
}

} // namespace std
