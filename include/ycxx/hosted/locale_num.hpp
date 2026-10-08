// libycxx hosted: num_get and num_put ([category.numeric]).
//
// Stage 2 of num_get (accumulating the field character by character against the atoms) and
// stages 2-4 of num_put (widening, grouping, padding, output) are templates here. The
// character-level work is done once, out of line in the hosted runtime (src/hosted/num.cpp):
// stage 1 of num_put (the characters printf would produce in the "C" locale, through
// <charconv> for floating-point values, so the result is correctly rounded and does not depend
// on the C library's locale) and stage 3 of num_get (converting the accumulated field as
// strtoll / strtoull / strtod would, again through <charconv>).
#pragma once

#include <ycxx/core/limits.hpp>
#include <ycxx/hosted/ios.hpp>
#include <ycxx/hosted/streambuf.hpp> // the default iterators work on stream buffers

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// ---- out of line (src/hosted/num.cpp) ---------------------------------------------------------

// num_put stage 1 for an integer: writes what printf(spec, v) prints for the specifier of
// [tab:facet.num.put.int] / [tab:facet.num.put.conv] (the magnitude and the sign are passed
// separately: is_signed selects %d, neg the sign). buf holds at least 72 characters. Returns the
// length; *pad is the internal-padding position (after the sign or a leading 0x / 0X).
std::size_t __num_put_integer(char* __buf, unsigned long long __magnitude, bool __neg, bool is_signed,
                            std::ios_base::fmtflags flags, std::size_t* __pad) noexcept;
// num_put stage 1 for a floating-point value ([tab:facet.num.put.fp]). Writes at most cap
// characters and returns the full length (call again with a larger buffer if it exceeds cap).
std::size_t __num_put_float(char* __buf, std::size_t __cap, double __v, std::ios_base::fmtflags flags, std::streamsize __prec,
                          std::size_t* __pad) noexcept;
std::size_t __num_put_float(char* __buf, std::size_t __cap, long double __v, std::ios_base::fmtflags flags,
                          std::streamsize __prec, std::size_t* __pad) noexcept;
// num_put stage 1 for %p (as the C library's printf). buf holds at least 72 characters.
std::size_t __num_put_pointer(char* __buf, const void* __v, std::size_t* __pad) noexcept;

enum class __num_parse : unsigned char { ok, __not_converted, overflow, underflow };
// num_get stage 3 for an integer field (base 8, 10, 16, or 0 for %i): the field's magnitude and
// sign, as strtoull reads them. overflow: the magnitude does not fit in unsigned long long.
__num_parse __num_get_integer(const char* field, std::size_t n, int base, unsigned long long* __magnitude,
                          bool* __neg) noexcept;
// num_get stage 3 for a floating-point field, as strtof / strtod / strtold: on overflow *v is
// +-HUGE_VAL, on underflow +-0.
__num_parse __num_get_float(const char* field, std::size_t n, float* __v) noexcept;
__num_parse __num_get_float(const char* field, std::size_t n, double* __v) noexcept;
__num_parse __num_get_float(const char* field, std::size_t n, long double* __v) noexcept;

// Whether the separator positions recorded by num_get stage 2 match grouping ([facet.num.get.
// virtuals]/4). groups[0..n) are the digit counts between separators, leftmost first (the last
// one is the group before the decimal point).
bool __num_grouping_ok(const std::string& grouping, const unsigned* __groups, std::size_t n) noexcept;

// ---- helpers ----------------------------------------------------------------------------------

// A buffer of T: N elements in place, more from the heap.
template <class _Tp, std::size_t _Np>
class __small_buffer {
public:
  explicit __small_buffer(std::size_t n) : __p_(n <= _Np ? __local_ : new _Tp[n]) {}
  ~__small_buffer() {
    if (__p_ != __local_)
      delete[] __p_;
  }
  __small_buffer(const __small_buffer&) = delete;
  __small_buffer& operator=(const __small_buffer&) = delete;
  _Tp* get() noexcept { return __p_; }

private:
  _Tp __local_[_Np];
  _Tp* __p_;
};

// num_put stages 2-4 ([facet.num.put.virtuals]): widens s[0..n) through ctype, replaces '.' with
// the decimal point, inserts thousands separators into the integer digits s[gbeg..gend) per
// grouping, pads to width() at the position adjustfield selects (internal: pad), resets the
// width and writes the result to out.
template <class __charT, class _OutIt>
_OutIt __num_put_output(_OutIt out, std::ios_base& str, __charT fill, const char* s, std::size_t n, std::size_t __pad,
                     std::size_t __gbeg, std::size_t __gend) {
  const std::locale& __loc = __ycxx::__detail::__ios_access::__locale_of(str);
  const std::ctype<__charT>& __ct = std::use_facet<std::ctype<__charT>>(__loc);
  const std::numpunct<__charT>& __np = std::use_facet<std::numpunct<__charT>>(__loc);
  // The classic char facets: ctype widens every character to itself, numpunct has '.' and no
  // grouping; their values need no virtual calls.
  bool __identity_widen = false, __classic_punct = false;
  if constexpr (std::is_same_v<__charT, char>) {
    const std::locale& c = std::locale::classic();
    __identity_widen = &__ct == &std::use_facet<std::ctype<char>>(c);
    __classic_punct = &__np == &std::use_facet<std::numpunct<char>>(c);
  }
  const std::string grouping = __gend > __gbeg && !__classic_punct ? __np.grouping() : std::string();
  [[indeterminate]] __small_buffer<__charT, 96> __wide(2 * n + 1);
  __charT* __w = __wide.get();
  std::size_t __len = 0;
  // the group boundaries, counted from the right end of the digit run
  std::size_t digits = __gend - __gbeg;
  bool __group = false;
  if (!grouping.empty() && static_cast<signed char>(grouping[0]) > 0 && grouping[0] != std::numeric_limits<char>::max())
    __group = true;
  const __charT __sep = __group ? __np.thousands_sep() : __charT();
  const __charT __point = __classic_punct ? static_cast<__charT>('.') : __np.decimal_point();
  if (!__group) {
    // No separators: the characters one to one, with the decimal point.
    for (std::size_t i = 0; i < n; ++i)
      __w[i] = s[i] == '.' ? __point : __identity_widen ? static_cast<__charT>(s[i]) : __ct.widen(s[i]);
    __len = n;
  } else {
    // mark[k] (k digits from the right): a separator goes before that digit
    [[indeterminate]] __small_buffer<bool, 96> __marks(digits + 1);
    bool* __mark = __marks.get();
    for (std::size_t k = 0; k <= digits; ++k)
      __mark[k] = false;
    std::size_t at = 0;
    for (std::size_t i = 0;; ++i) {
      const char __g = grouping[i < grouping.size() ? i : grouping.size() - 1];
      if (static_cast<signed char>(__g) <= 0 || __g == std::numeric_limits<char>::max())
        break;
      at += static_cast<unsigned char>(__g);
      if (at >= digits)
        break;
      __mark[at] = true;
    }
    for (std::size_t i = 0; i < n; ++i) {
      if (s[i] == '.') {
        __w[__len++] = __point;
        continue;
      }
      if (i > __gbeg && i < __gend && __mark[__gend - i])
        __w[__len++] = __sep;
      __w[__len++] = __ct.widen(s[i]);
    }
  }
  // (pad is at most gbeg, so the separators never move it)
  const std::streamsize width = str.width();
  str.width(0);
  std::size_t __fill_count = width > 0 && static_cast<std::size_t>(width) > __len ? static_cast<std::size_t>(width) - __len : 0;
  const std::ios_base::fmtflags __adjust = str.flags() & std::ios_base::adjustfield;
  std::size_t at = 0; // where the fill goes
  if (__adjust == std::ios_base::left)
    at = __len;
  else if (__adjust == std::ios_base::internal)
    at = __pad;
  for (std::size_t i = 0; i < at; ++i, static_cast<void>(++out))
    *out = __w[i];
  for (; __fill_count != 0; --__fill_count, static_cast<void>(++out))
    *out = fill;
  for (std::size_t i = at; i < __len; ++i, static_cast<void>(++out))
    *out = __w[i];
  return out;
}

// The field num_get accumulates in stage 2: in place up to 64 characters (any integer field
// and most floating-point ones), then in a string.
class __num_field {
  char __buf_[64];
  std::size_t __n_ = 0;
  std::string __heap_; // used once the field outgrows buf_

public:
  __num_field() = default;
  __num_field(const __num_field&) = delete;
  [[__gnu__::__always_inline__]] void push_back(char c) {
    if (__n_ < sizeof __buf_) {
      __buf_[__n_++] = c;
    } else {
      if (__n_ == sizeof __buf_)
        __heap_.assign(__buf_, __n_);
      __heap_.push_back(c);
      ++__n_;
    }
  }
  const char* data() const noexcept { return __n_ <= sizeof __buf_ ? __buf_ : __heap_.data(); }
  std::size_t size() const noexcept { return __n_; }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [locale.num.get]
template <class __charT, class _InputIterator>
class num_get : public locale::facet {
public:
  using char_type = __charT;
  using iter_type = _InputIterator;

  explicit num_get(size_t __refs = 0) : locale::facet(__refs) {}

  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, bool& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, long& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, long long& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, unsigned short& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, unsigned int& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, unsigned long& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, unsigned long long& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, float& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, double& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, long double& __v) const {
    return do_get(in, end, str, __err, __v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, void*& __v) const {
    return do_get(in, end, str, __err, __v);
  }

  static locale::id id;

protected:
  ~num_get() override {}

  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, bool& __v) const {
    if (!(str.flags() & ios_base::boolalpha)) {
      long __lv = -1;
      in = __get_integer(in, end, str, __err, __lv);
      if (__lv == 0)
        __v = false;
      else if (__lv == 1)
        __v = true;
      else {
        __v = true;
        __err |= ios_base::failbit;
      }
      return in;
    }
    return __get_bool_alpha(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, long& __v) const {
    return __get_integer(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, long long& __v) const {
    return __get_integer(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err,
                           unsigned short& __v) const {
    return __get_integer(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, unsigned int& __v) const {
    return __get_integer(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err,
                           unsigned long& __v) const {
    return __get_integer(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err,
                           unsigned long long& __v) const {
    return __get_integer(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, float& __v) const {
    return __get_floating(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, double& __v) const {
    return __get_floating(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err,
                           long double& __v) const {
    return __get_floating(in, end, str, __err, __v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, void*& __v) const {
    [[indeterminate]] __ycxx::__detail::__num_field field;
    bool __grouping_ok = true;
    in = accumulate(in, end, str, __err, 'p', field, __grouping_ok);
    unsigned long long __mag = 0;
    bool __neg = false;
    const __ycxx::__detail::__num_parse r = __ycxx::__detail::__num_get_integer(field.data(), field.size(), 16, &__mag, &__neg);
    if (r != __ycxx::__detail::__num_parse::ok || __mag > numeric_limits<__UINTPTR_TYPE__>::max()) {
      __v = nullptr;
      __err |= ios_base::failbit;
    } else {
      if (__neg)
        __mag = 0 - __mag;
      __v = reinterpret_cast<void*>(static_cast<__UINTPTR_TYPE__>(__mag));
      if (!__grouping_ok)
        __err |= ios_base::failbit;
    }
    return in;
  }

private:
  // Stage 2 ([facet.num.get.virtuals]/3): accumulates the field of conversion `__spec` ('d', 'u',
  // 'o', 'X', 'i', 'g' or 'p'), each character only if it can continue a field of that
  // conversion (as scanf's would). Records the separator positions for the grouping check and
  // sets eofbit in err if stopped by in == end.
  static iter_type accumulate(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, char __spec,
                              __ycxx::__detail::__num_field& field, bool& __grouping_ok) {
    static constexpr char __src[] = "0123456789abcdefpxABCDEFPX+-";
    const locale& __loc = __ycxx::__detail::__ios_access::__locale_of(str);
    const numpunct<__charT>& __np = use_facet<numpunct<__charT>>(__loc);
    const ctype<__charT>& __ctf = use_facet<ctype<__charT>>(__loc);
    // char atoms that widen to themselves: looked up in a table instead of searching the atom
    // list for every character.
    static constexpr auto __atom_of = [] {
      struct table {
        char c[256] = {};
        constexpr char operator[](unsigned char __x) const { return c[__x]; }
      } t;
      for (const char* p = __src; *p != '\0'; ++p)
        t.c[static_cast<unsigned char>(*p)] = *p;
      return t;
    }();
    // The classic char facets (widening to the same character, '.', no grouping) need no
    // virtual calls.
    bool classic = false;
    if constexpr (is_same_v<__charT, char>) {
      const locale& c = locale::classic();
      classic = &__np == &use_facet<numpunct<char>>(c) && &__ctf == &use_facet<ctype<char>>(c);
    }
    [[indeterminate]] __charT __atoms[sizeof(__src)];
    __charT __point_char = static_cast<__charT>('.');
    __charT __sep = static_cast<__charT>(',');
    string grouping;
    bool __identity_atoms = classic;
    if (!classic) {
      __ctf.widen(__src, __src + sizeof(__src), __atoms);
      __point_char = __np.decimal_point();
      __sep = __np.thousands_sep();
      grouping = __np.grouping();
      if constexpr (is_same_v<__charT, char>)
        __identity_atoms = char_traits<char>::compare(__atoms, __src, sizeof(__src) - 1) == 0;
    }
    const bool __grouped = !grouping.empty();

    // %d / %u with plain char atoms and no grouping (the classic case): an optional sign, then
    // decimal digits, exactly what the general loop below accepts there.
    if (__identity_atoms && !__grouped && (__spec == 'd' || __spec == 'u') && !(__point_char >= '0' && __point_char <= '9')) {
      for (bool __at_first = true;; static_cast<void>(++in), __at_first = false) {
        if (in == end) {
          __err |= ios_base::eofbit;
          break;
        }
        const __charT __ct = *in;
        if ((__ct >= '0' && __ct <= '9') || (__at_first && (__ct == '+' || __ct == '-')))
          field.push_back(static_cast<char>(__ct));
        else
          break;
      }
      return in;
    }

    const bool __is_float = __spec == 'g';
    const bool __may_prefix = __spec == 'X' || __spec == 'p' || __spec == 'i' || __spec == 'g';
    int radix = __spec == 'o' ? 8 : (__spec == 'X' || __spec == 'p') ? 16 : __spec == 'i' ? 0 : 10; // 0: not yet known
    bool __at_start = true;   // nothing accumulated: a sign may come
    bool __zero_only = false; // the mantissa is just a leading "0" so far: an 'x' may follow
    bool __any_digit = false; // a mantissa digit (after the 0x prefix, if any)
    bool __point = false;
    int __exp_state = 0; // 0: none; 1: after e/p; 2: after the exponent's sign; 3: exponent digits
    [[indeterminate]] unsigned __groups[64];
    size_t __ngroups = 0;
    unsigned run = 0; // integer digits since the last separator
    bool __seen_sep = false;

    for (;; ++in) {
      if (in == end) {
        __err |= ios_base::eofbit;
        break;
      }
      const __charT __ct = *in;
      if (__grouped && __ct == __sep) {
        if (__point)
          break; // a separator after the decimal point ends Stage 2
        if (__exp_state != 0)
          __grouping_ok = false;
        if (__ngroups < sizeof __groups / sizeof __groups[0])
          __groups[__ngroups++] = run;
        else
          __grouping_ok = false;
        run = 0;
        __seen_sep = true;
        continue; // remembered, otherwise ignored
      }
      // The common case, a decimal or hexadecimal digit 0-9 of the mantissa, with the effects
      // of the general path below.
      if (__identity_atoms && __exp_state == 0 && (radix == 10 || radix == 16) && __ct >= '0' && __ct <= '9' &&
          !(__ct == __point_char)) {
        __zero_only = !__any_digit && !__point && __ct == '0';
        __any_digit = true;
        if (!__point)
          ++run;
        __at_start = false;
        field.push_back(static_cast<char>(__ct));
        continue;
      }
      char c;
      if (__identity_atoms) {
        c = __atom_of[static_cast<unsigned char>(__ct)];
      } else {
        size_t k = 0;
        while (k != sizeof(__src) - 1 && !(__atoms[k] == __ct))
          ++k;
        c = __src[k];
      }
      if (__ct == __point_char)
        c = '.';
      if (c == '\0')
        break;

      bool ok = false;
      if (__exp_state != 0) {
        if ((c == '+' || c == '-') && __exp_state == 1) {
          __exp_state = 2;
          ok = true;
        } else if (c >= '0' && c <= '9') {
          __exp_state = 3;
          ok = true;
        }
      } else if (c == '+' || c == '-') {
        ok = __at_start;
      } else if (c == '.') {
        if (__is_float && !__point) {
          __point = true;
          __zero_only = false;
          ok = true;
        }
      } else if ((c == 'x' || c == 'X') && __zero_only && __may_prefix) {
        radix = 16;
        __zero_only = false;
        __any_digit = false;
        run = 0;
        ok = true;
      } else {
        const int d = c <= '9' ? c - '0' : c <= 'Z' ? c - 'A' + 10 : c - 'a' + 10;
        const bool __is_digit = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (__is_digit && d < (radix == 0 ? 10 : radix)) {
          if (!__any_digit && !__point && d == 0) {
            __zero_only = true;
            if (radix == 0)
              radix = 8; // %i: a leading 0 selects octal (or 0x hexadecimal)
          } else {
            __zero_only = false;
          }
          if (radix == 0)
            radix = 10;
          __any_digit = true;
          if (!__point)
            ++run;
          ok = true;
        } else if (__is_float && __any_digit &&
                   (radix == 16 ? (c == 'p' || c == 'P') : (c == 'e' || c == 'E'))) {
          __exp_state = 1;
          __zero_only = false;
          ok = true;
        }
      }
      if (!ok)
        break;
      __at_start = false;
      field.push_back(c);
    }
    if (__seen_sep && __grouping_ok) {
      if (__ngroups < sizeof __groups / sizeof __groups[0]) {
        __groups[__ngroups++] = run;
        __grouping_ok = __ycxx::__detail::__num_grouping_ok(grouping, __groups, __ngroups);
      } else {
        __grouping_ok = false;
      }
    }
    return in;
  }
  template <class _Tp>
  static iter_type __get_integer(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, _Tp& __v) {
    const ios_base::fmtflags basefield = str.flags() & ios_base::basefield;
    char __spec;
    int base;
    if (basefield == ios_base::oct) {
      __spec = 'o';
      base = 8;
    } else if (basefield == ios_base::hex) {
      __spec = 'X';
      base = 16;
    } else if (basefield == ios_base::fmtflags{}) {
      __spec = 'i';
      base = 0;
    } else {
      __spec = is_signed_v<_Tp> ? 'd' : 'u';
      base = 10;
    }
    [[indeterminate]] __ycxx::__detail::__num_field field;
    bool __grouping_ok = true;
    in = accumulate(in, end, str, __err, __spec, field, __grouping_ok);
    unsigned long long __mag = 0;
    bool __neg = false;
    const __ycxx::__detail::__num_parse r = __ycxx::__detail::__num_get_integer(field.data(), field.size(), base, &__mag, &__neg);
    if (r == __ycxx::__detail::__num_parse::__not_converted) {
      __v = 0;
      __err |= ios_base::failbit;
      return in;
    }
    if constexpr (is_signed_v<_Tp>) {
      const unsigned long long __limit =
          __neg ? static_cast<unsigned long long>(numeric_limits<_Tp>::max()) + 1 : numeric_limits<_Tp>::max();
      if (r == __ycxx::__detail::__num_parse::overflow || __mag > __limit) {
        __v = __neg ? numeric_limits<_Tp>::min() : numeric_limits<_Tp>::max();
        __err |= ios_base::failbit;
        return in;
      }
      using _Up = make_unsigned_t<_Tp>;
      __v = __neg ? static_cast<_Tp>(0 - static_cast<_Up>(__mag)) : static_cast<_Tp>(__mag);
    } else {
      // strtoull's rule: a negative field is the negated magnitude in unsigned long long
      // ("-1" is ULLONG_MAX); a result that val cannot represent stores its maximum
      const unsigned long long value = __neg ? 0 - __mag : __mag;
      if (r == __ycxx::__detail::__num_parse::overflow || value > numeric_limits<_Tp>::max()) {
        __v = numeric_limits<_Tp>::max();
        __err |= ios_base::failbit;
        return in;
      }
      __v = static_cast<_Tp>(value);
    }
    if (!__grouping_ok)
      __err |= ios_base::failbit;
    return in;
  }

  template <class _Tp>
  static iter_type __get_floating(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, _Tp& __v) {
    [[indeterminate]] __ycxx::__detail::__num_field field;
    bool __grouping_ok = true;
    in = accumulate(in, end, str, __err, 'g', field, __grouping_ok);
    _Tp result{};
    const __ycxx::__detail::__num_parse r = __ycxx::__detail::__num_get_float(field.data(), field.size(), &result);
    if (r == __ycxx::__detail::__num_parse::__not_converted) {
      __v = 0;
      __err |= ios_base::failbit;
      return in;
    }
    __v = result;
    if (r != __ycxx::__detail::__num_parse::ok || !__grouping_ok)
      __err |= ios_base::failbit;
    return in;
  }
  // [facet.num.get.virtuals]/7-8: matching truename() / falsename(), reading characters only as
  // needed: consumption continues while some target can be extended by the next character; a
  // target matches if it is exactly the consumed sequence.
  static iter_type __get_bool_alpha(iter_type in, iter_type end, ios_base& str, ios_base::iostate& __err, bool& __v) {
    const numpunct<__charT>& __np = use_facet<numpunct<__charT>>(str.getloc());
    const basic_string<__charT> t = __np.truename(), __f = __np.falsename();
    bool __t_alive = true, __f_alive = true;
    size_t i = 0;
    bool __at_end = false;
    for (;;) {
      const bool __t_more = __t_alive && i < t.size(), __f_more = __f_alive && i < __f.size();
      if (!__t_more && !__f_more)
        break;
      if (in == end) {
        __at_end = true;
        break;
      }
      const __charT c = *in;
      const bool __t_ok = __t_more && t[i] == c, __f_ok = __f_more && __f[i] == c;
      if (!__t_ok && !__f_ok)
        break;
      __t_alive = __t_ok;
      __f_alive = __f_ok;
      ++in;
      ++i;
    }
    const bool __t_match = __t_alive && i == t.size(), __f_match = __f_alive && i == __f.size();
    if (__t_match != __f_match) {
      __v = __t_match;
      __err = __at_end ? ios_base::eofbit : ios_base::goodbit;
    } else {
      __v = false;
      __err = __at_end ? (ios_base::failbit | ios_base::eofbit) : ios_base::failbit;
    }
    return in;
  }
};
template <class __charT, class _InputIterator>
locale::id num_get<__charT, _InputIterator>::id;

// [locale.nm.put]
template <class __charT, class _OutputIterator>
class num_put : public locale::facet {
public:
  using char_type = __charT;
  using iter_type = _OutputIterator;

  explicit num_put(size_t __refs = 0) : locale::facet(__refs) {}

  iter_type put(iter_type s, ios_base& __f, char_type fill, bool __v) const { return do_put(s, __f, fill, __v); }
  iter_type put(iter_type s, ios_base& __f, char_type fill, long __v) const { return do_put(s, __f, fill, __v); }
  iter_type put(iter_type s, ios_base& __f, char_type fill, long long __v) const { return do_put(s, __f, fill, __v); }
  iter_type put(iter_type s, ios_base& __f, char_type fill, unsigned long __v) const { return do_put(s, __f, fill, __v); }
  iter_type put(iter_type s, ios_base& __f, char_type fill, unsigned long long __v) const {
    return do_put(s, __f, fill, __v);
  }
  iter_type put(iter_type s, ios_base& __f, char_type fill, double __v) const { return do_put(s, __f, fill, __v); }
  iter_type put(iter_type s, ios_base& __f, char_type fill, long double __v) const { return do_put(s, __f, fill, __v); }
  iter_type put(iter_type s, ios_base& __f, char_type fill, const void* __v) const { return do_put(s, __f, fill, __v); }

  static locale::id id;

protected:
  ~num_put() override {}

  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, bool __v) const {
    if (!(str.flags() & ios_base::boolalpha))
      return do_put(out, str, fill, static_cast<long>(__v));
    const numpunct<__charT>& __np = use_facet<numpunct<__charT>>(str.getloc());
    const basic_string<__charT> s = __v ? __np.truename() : __np.falsename();
    // padded as the other conversions (Table 101; no sign, so internal pads before)
    const streamsize width = str.width();
    str.width(0);
    size_t __fill_count = width > 0 && static_cast<size_t>(width) > s.size() ? static_cast<size_t>(width) - s.size() : 0;
    const bool left = (str.flags() & ios_base::adjustfield) == ios_base::left;
    if (!left)
      for (; __fill_count != 0; --__fill_count, static_cast<void>(++out))
        *out = fill;
    for (__charT c : s) {
      *out = c;
      ++out;
    }
    for (; __fill_count != 0; --__fill_count, static_cast<void>(++out))
      *out = fill;
    return out;
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, long __v) const {
    return __put_integer(out, str, fill, __v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, long long __v) const {
    return __put_integer(out, str, fill, __v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, unsigned long __v) const {
    return __put_integer(out, str, fill, __v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, unsigned long long __v) const {
    return __put_integer(out, str, fill, __v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, double __v) const {
    return __put_floating(out, str, fill, __v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, long double __v) const {
    return __put_floating(out, str, fill, __v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, const void* __v) const {
    [[indeterminate]] char __buf[72];
    size_t __pad = 0;
    const size_t n = __ycxx::__detail::__num_put_pointer(__buf, __v, &__pad);
    return __ycxx::__detail::__num_put_output(out, str, fill, __buf, n, __pad, 0, 0);
  }

private:
  template <class _Tp>
  static iter_type __put_integer(iter_type out, ios_base& str, char_type fill, _Tp __v) {
    [[indeterminate]] char __buf[72];
    size_t __pad = 0;
    bool __neg = false;
    unsigned long long __mag;
    if constexpr (is_signed_v<_Tp>) {
      __neg = __v < 0;
      __mag = __neg ? 0ull - static_cast<unsigned long long>(__v) : static_cast<unsigned long long>(__v);
      // %o / %x / %X convert the value as unsigned: the bits of v in its own width
      const ios_base::fmtflags base = str.flags() & ios_base::basefield;
      if (__neg && (base == ios_base::oct || base == ios_base::hex)) {
        __mag = static_cast<make_unsigned_t<_Tp>>(__v);
        __neg = false;
      }
    } else {
      __mag = __v;
    }
    const size_t n = __ycxx::__detail::__num_put_integer(__buf, __mag, __neg, is_signed_v<_Tp>, str.flags(), &__pad);
    return __ycxx::__detail::__num_put_output(out, str, fill, __buf, n, __pad, __pad, n);
  }

  template <class _Tp>
  static iter_type __put_floating(iter_type out, ios_base& str, char_type fill, _Tp __v) {
    [[indeterminate]] char __y_local[128];
    size_t __pad = 0;
    size_t n = __ycxx::__detail::__num_put_float(__y_local, sizeof __y_local, __v, str.flags(), str.precision(), &__pad);
    if (n <= sizeof __y_local)
      return __finish_floating(out, str, fill, __y_local, n, __pad);
    __ycxx::__detail::__small_buffer<char, 1> big(n);
    n = __ycxx::__detail::__num_put_float(big.get(), n, __v, str.flags(), str.precision(), &__pad);
    return __finish_floating(out, str, fill, big.get(), n, __pad);
  }
  static iter_type __finish_floating(iter_type out, ios_base& str, char_type fill, const char* s, size_t n, size_t __pad) {
    // the integer digits of the mantissa: decimal digits from pad up to '.', the exponent or
    // the end (none for inf / nan, and none for %a, whose digits are hexadecimal)
    size_t __gend = __pad;
    const bool hexfloat = __pad >= 2 && (s[__pad - 1] == 'x' || s[__pad - 1] == 'X');
    if (!hexfloat)
      while (__gend < n && s[__gend] >= '0' && s[__gend] <= '9')
        ++__gend;
    return __ycxx::__detail::__num_put_output(out, str, fill, s, n, __pad, __pad, __gend);
  }
};
template <class __charT, class _OutputIterator>
locale::id num_put<__charT, _OutputIterator>::id;

}} // namespace std
