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

namespace ycxx::detail {

// ---- out of line (src/hosted/num.cpp) ---------------------------------------------------------

// num_put stage 1 for an integer: writes what printf(spec, v) prints for the specifier of
// [tab:facet.num.put.int] / [tab:facet.num.put.conv] (the magnitude and the sign are passed
// separately: is_signed selects %d, neg the sign). buf holds at least 72 characters. Returns the
// length; *pad is the internal-padding position (after the sign or a leading 0x / 0X).
std::size_t num_put_integer(char* buf, unsigned long long magnitude, bool neg, bool is_signed,
                            std::ios_base::fmtflags flags, std::size_t* pad) noexcept;
// num_put stage 1 for a floating-point value ([tab:facet.num.put.fp]). Writes at most cap
// characters and returns the full length (call again with a larger buffer if it exceeds cap).
std::size_t num_put_float(char* buf, std::size_t cap, double v, std::ios_base::fmtflags flags, std::streamsize prec,
                          std::size_t* pad) noexcept;
std::size_t num_put_float(char* buf, std::size_t cap, long double v, std::ios_base::fmtflags flags,
                          std::streamsize prec, std::size_t* pad) noexcept;
// num_put stage 1 for %p (as the C library's printf). buf holds at least 72 characters.
std::size_t num_put_pointer(char* buf, const void* v, std::size_t* pad) noexcept;

enum class num_parse : unsigned char { ok, not_converted, overflow, underflow };
// num_get stage 3 for an integer field (base 8, 10, 16, or 0 for %i): the field's magnitude and
// sign, as strtoull reads them. overflow: the magnitude does not fit in unsigned long long.
num_parse num_get_integer(const char* field, std::size_t n, int base, unsigned long long* magnitude,
                          bool* neg) noexcept;
// num_get stage 3 for a floating-point field, as strtof / strtod / strtold: on overflow *v is
// +-HUGE_VAL, on underflow +-0.
num_parse num_get_float(const char* field, std::size_t n, float* v) noexcept;
num_parse num_get_float(const char* field, std::size_t n, double* v) noexcept;
num_parse num_get_float(const char* field, std::size_t n, long double* v) noexcept;

// Whether the separator positions recorded by num_get stage 2 match grouping ([facet.num.get.
// virtuals]/4). groups[0..n) are the digit counts between separators, leftmost first (the last
// one is the group before the decimal point).
bool num_grouping_ok(const std::string& grouping, const unsigned* groups, std::size_t n) noexcept;

// ---- helpers ----------------------------------------------------------------------------------

// A buffer of T: N elements in place, more from the heap.
template <class T, std::size_t N>
class small_buffer {
public:
  explicit small_buffer(std::size_t n) : p_(n <= N ? local_ : new T[n]) {}
  ~small_buffer() {
    if (p_ != local_)
      delete[] p_;
  }
  small_buffer(const small_buffer&) = delete;
  small_buffer& operator=(const small_buffer&) = delete;
  T* get() noexcept { return p_; }

private:
  T local_[N];
  T* p_;
};

// num_put stages 2-4 ([facet.num.put.virtuals]): widens s[0..n) through ctype, replaces '.' with
// the decimal point, inserts thousands separators into the integer digits s[gbeg..gend) per
// grouping, pads to width() at the position adjustfield selects (internal: pad), resets the
// width and writes the result to out.
template <class charT, class OutIt>
OutIt num_put_output(OutIt out, std::ios_base& str, charT fill, const char* s, std::size_t n, std::size_t pad,
                     std::size_t gbeg, std::size_t gend) {
  const std::locale loc = str.getloc();
  const std::ctype<charT>& ct = std::use_facet<std::ctype<charT>>(loc);
  const std::numpunct<charT>& np = std::use_facet<std::numpunct<charT>>(loc);
  const std::string grouping = gend > gbeg ? np.grouping() : std::string();
  small_buffer<charT, 96> wide(2 * n + 1);
  charT* w = wide.get();
  std::size_t len = 0;
  // the group boundaries, counted from the right end of the digit run
  std::size_t digits = gend - gbeg;
  bool group = false;
  if (!grouping.empty() && static_cast<signed char>(grouping[0]) > 0 && grouping[0] != std::numeric_limits<char>::max())
    group = true;
  const charT sep = group ? np.thousands_sep() : charT();
  const charT point = np.decimal_point();
  // mark[k] (k digits from the right): a separator goes before that digit
  small_buffer<bool, 96> marks(digits + 1);
  bool* mark = marks.get();
  for (std::size_t k = 0; k <= digits; ++k)
    mark[k] = false;
  if (group) {
    std::size_t at = 0;
    for (std::size_t i = 0;; ++i) {
      const char g = grouping[i < grouping.size() ? i : grouping.size() - 1];
      if (static_cast<signed char>(g) <= 0 || g == std::numeric_limits<char>::max())
        break;
      at += static_cast<unsigned char>(g);
      if (at >= digits)
        break;
      mark[at] = true;
    }
  }
  for (std::size_t i = 0; i < n; ++i) {
    if (s[i] == '.') {
      w[len++] = point;
      continue;
    }
    if (i > gbeg && i < gend && mark[gend - i])
      w[len++] = sep;
    w[len++] = ct.widen(s[i]);
  }
  // (pad is at most gbeg, so the separators never move it)
  const std::streamsize width = str.width();
  str.width(0);
  std::size_t fill_count = width > 0 && static_cast<std::size_t>(width) > len ? static_cast<std::size_t>(width) - len : 0;
  const std::ios_base::fmtflags adjust = str.flags() & std::ios_base::adjustfield;
  std::size_t at = 0; // where the fill goes
  if (adjust == std::ios_base::left)
    at = len;
  else if (adjust == std::ios_base::internal)
    at = pad;
  for (std::size_t i = 0; i < at; ++i, ++out)
    *out = w[i];
  for (; fill_count != 0; --fill_count, ++out)
    *out = fill;
  for (std::size_t i = at; i < len; ++i, ++out)
    *out = w[i];
  return out;
}

} // namespace ycxx::detail

namespace std {

// [locale.num.get]
template <class charT, class InputIterator>
class num_get : public locale::facet {
public:
  using char_type = charT;
  using iter_type = InputIterator;

  explicit num_get(size_t refs = 0) : locale::facet(refs) {}

  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, bool& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, long& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, long long& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, unsigned short& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, unsigned int& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, unsigned long& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, unsigned long long& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, float& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, double& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, long double& v) const {
    return do_get(in, end, str, err, v);
  }
  iter_type get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, void*& v) const {
    return do_get(in, end, str, err, v);
  }

  static locale::id id;

protected:
  ~num_get() override {}

  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, bool& v) const {
    if (!(str.flags() & ios_base::boolalpha)) {
      long lv = -1;
      in = get_integer(in, end, str, err, lv);
      if (lv == 0)
        v = false;
      else if (lv == 1)
        v = true;
      else {
        v = true;
        err |= ios_base::failbit;
      }
      return in;
    }
    return get_bool_alpha(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, long& v) const {
    return get_integer(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, long long& v) const {
    return get_integer(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err,
                           unsigned short& v) const {
    return get_integer(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, unsigned int& v) const {
    return get_integer(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err,
                           unsigned long& v) const {
    return get_integer(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err,
                           unsigned long long& v) const {
    return get_integer(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, float& v) const {
    return get_floating(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, double& v) const {
    return get_floating(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err,
                           long double& v) const {
    return get_floating(in, end, str, err, v);
  }
  virtual iter_type do_get(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, void*& v) const {
    string field;
    bool grouping_ok = true;
    in = accumulate(in, end, str, err, 'p', field, grouping_ok);
    unsigned long long mag = 0;
    bool neg = false;
    const ycxx::detail::num_parse r = ycxx::detail::num_get_integer(field.data(), field.size(), 16, &mag, &neg);
    if (r != ycxx::detail::num_parse::ok || mag > numeric_limits<__UINTPTR_TYPE__>::max()) {
      v = nullptr;
      err |= ios_base::failbit;
    } else {
      if (neg)
        mag = 0 - mag;
      v = reinterpret_cast<void*>(static_cast<__UINTPTR_TYPE__>(mag));
      if (!grouping_ok)
        err |= ios_base::failbit;
    }
    return in;
  }

private:
  // Stage 2 ([facet.num.get.virtuals]/3): accumulates the field of conversion `spec` ('d', 'u',
  // 'o', 'X', 'i', 'g' or 'p'), each character only if it can continue a field of that
  // conversion (as scanf's would). Records the separator positions for the grouping check and
  // sets eofbit in err if stopped by in == end.
  static iter_type accumulate(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, char spec,
                              string& field, bool& grouping_ok) {
    static constexpr char src[] = "0123456789abcdefpxABCDEFPX+-";
    const locale loc = str.getloc();
    const numpunct<charT>& np = use_facet<numpunct<charT>>(loc);
    charT atoms[sizeof(src)];
    use_facet<ctype<charT>>(loc).widen(src, src + sizeof(src), atoms);
    const charT point_char = np.decimal_point();
    const charT sep = np.thousands_sep();
    const string grouping = np.grouping();
    const bool grouped = !grouping.empty();

    const bool is_float = spec == 'g';
    const bool may_prefix = spec == 'X' || spec == 'p' || spec == 'i' || spec == 'g';
    int radix = spec == 'o' ? 8 : (spec == 'X' || spec == 'p') ? 16 : spec == 'i' ? 0 : 10; // 0: not yet known
    bool at_start = true;   // nothing accumulated: a sign may come
    bool zero_only = false; // the mantissa is just a leading "0" so far: an 'x' may follow
    bool any_digit = false; // a mantissa digit (after the 0x prefix, if any)
    bool point = false;
    int exp_state = 0; // 0: none; 1: after e/p; 2: after the exponent's sign; 3: exponent digits
    unsigned groups[64];
    size_t ngroups = 0;
    unsigned run = 0; // integer digits since the last separator
    bool seen_sep = false;

    for (;; ++in) {
      if (in == end) {
        err |= ios_base::eofbit;
        break;
      }
      const charT ct = *in;
      if (grouped && ct == sep) {
        if (point)
          break; // a separator after the decimal point ends Stage 2
        if (exp_state != 0)
          grouping_ok = false;
        if (ngroups < sizeof groups / sizeof groups[0])
          groups[ngroups++] = run;
        else
          grouping_ok = false;
        run = 0;
        seen_sep = true;
        continue; // remembered, otherwise ignored
      }
      size_t k = 0;
      while (k != sizeof(src) - 1 && !(atoms[k] == ct))
        ++k;
      char c = src[k];
      if (ct == point_char)
        c = '.';
      if (c == '\0')
        break;

      bool ok = false;
      if (exp_state != 0) {
        if ((c == '+' || c == '-') && exp_state == 1) {
          exp_state = 2;
          ok = true;
        } else if (c >= '0' && c <= '9') {
          exp_state = 3;
          ok = true;
        }
      } else if (c == '+' || c == '-') {
        ok = at_start;
      } else if (c == '.') {
        if (is_float && !point) {
          point = true;
          zero_only = false;
          ok = true;
        }
      } else if ((c == 'x' || c == 'X') && zero_only && may_prefix) {
        radix = 16;
        zero_only = false;
        any_digit = false;
        run = 0;
        ok = true;
      } else {
        const int d = c <= '9' ? c - '0' : c <= 'Z' ? c - 'A' + 10 : c - 'a' + 10;
        const bool is_digit = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (is_digit && d < (radix == 0 ? 10 : radix)) {
          if (!any_digit && !point && d == 0) {
            zero_only = true;
            if (radix == 0)
              radix = 8; // %i: a leading 0 selects octal (or 0x hexadecimal)
          } else {
            zero_only = false;
          }
          if (radix == 0)
            radix = 10;
          any_digit = true;
          if (!point)
            ++run;
          ok = true;
        } else if (is_float && any_digit &&
                   (radix == 16 ? (c == 'p' || c == 'P') : (c == 'e' || c == 'E'))) {
          exp_state = 1;
          zero_only = false;
          ok = true;
        }
      }
      if (!ok)
        break;
      at_start = false;
      field.push_back(c);
    }
    if (seen_sep && grouping_ok) {
      if (ngroups < sizeof groups / sizeof groups[0]) {
        groups[ngroups++] = run;
        grouping_ok = ycxx::detail::num_grouping_ok(grouping, groups, ngroups);
      } else {
        grouping_ok = false;
      }
    }
    return in;
  }
  template <class T>
  static iter_type get_integer(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, T& v) {
    const ios_base::fmtflags basefield = str.flags() & ios_base::basefield;
    char spec;
    int base;
    if (basefield == ios_base::oct) {
      spec = 'o';
      base = 8;
    } else if (basefield == ios_base::hex) {
      spec = 'X';
      base = 16;
    } else if (basefield == ios_base::fmtflags{}) {
      spec = 'i';
      base = 0;
    } else {
      spec = is_signed_v<T> ? 'd' : 'u';
      base = 10;
    }
    string field;
    bool grouping_ok = true;
    in = accumulate(in, end, str, err, spec, field, grouping_ok);
    unsigned long long mag = 0;
    bool neg = false;
    const ycxx::detail::num_parse r = ycxx::detail::num_get_integer(field.data(), field.size(), base, &mag, &neg);
    using U = make_unsigned_t<T>;
    if (r == ycxx::detail::num_parse::not_converted) {
      v = 0;
      err |= ios_base::failbit;
      return in;
    }
    if constexpr (is_signed_v<T>) {
      const unsigned long long limit =
          neg ? static_cast<unsigned long long>(numeric_limits<T>::max()) + 1 : numeric_limits<T>::max();
      if (r == ycxx::detail::num_parse::overflow || mag > limit) {
        v = neg ? numeric_limits<T>::min() : numeric_limits<T>::max();
        err |= ios_base::failbit;
        return in;
      }
      v = neg ? static_cast<T>(0 - static_cast<U>(mag)) : static_cast<T>(mag);
    } else {
      // strtoull's rule: a negative field is the negated magnitude, in val's type
      if (r == ycxx::detail::num_parse::overflow || mag > numeric_limits<T>::max()) {
        v = numeric_limits<T>::max();
        err |= ios_base::failbit;
        return in;
      }
      v = neg ? static_cast<T>(0 - static_cast<U>(mag)) : static_cast<T>(mag);
    }
    if (!grouping_ok)
      err |= ios_base::failbit;
    return in;
  }

  template <class T>
  static iter_type get_floating(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, T& v) {
    string field;
    bool grouping_ok = true;
    in = accumulate(in, end, str, err, 'g', field, grouping_ok);
    T result{};
    const ycxx::detail::num_parse r = ycxx::detail::num_get_float(field.data(), field.size(), &result);
    if (r == ycxx::detail::num_parse::not_converted) {
      v = 0;
      err |= ios_base::failbit;
      return in;
    }
    v = result;
    if (r != ycxx::detail::num_parse::ok || !grouping_ok)
      err |= ios_base::failbit;
    return in;
  }
  // [facet.num.get.virtuals]/7-8: matching truename() / falsename(), reading characters only as
  // needed: consumption continues while some target can be extended by the next character; a
  // target matches if it is exactly the consumed sequence.
  static iter_type get_bool_alpha(iter_type in, iter_type end, ios_base& str, ios_base::iostate& err, bool& v) {
    const numpunct<charT>& np = use_facet<numpunct<charT>>(str.getloc());
    const basic_string<charT> t = np.truename(), f = np.falsename();
    bool t_alive = true, f_alive = true;
    size_t i = 0;
    bool at_end = false;
    for (;;) {
      const bool t_more = t_alive && i < t.size(), f_more = f_alive && i < f.size();
      if (!t_more && !f_more)
        break;
      if (in == end) {
        at_end = true;
        break;
      }
      const charT c = *in;
      const bool t_ok = t_more && t[i] == c, f_ok = f_more && f[i] == c;
      if (!t_ok && !f_ok)
        break;
      t_alive = t_ok;
      f_alive = f_ok;
      ++in;
      ++i;
    }
    const bool t_match = t_alive && i == t.size(), f_match = f_alive && i == f.size();
    if (t_match != f_match) {
      v = t_match;
      err = at_end ? ios_base::eofbit : ios_base::goodbit;
    } else {
      v = false;
      err = at_end ? (ios_base::failbit | ios_base::eofbit) : ios_base::failbit;
    }
    return in;
  }
};
template <class charT, class InputIterator>
locale::id num_get<charT, InputIterator>::id;

// [locale.nm.put]
template <class charT, class OutputIterator>
class num_put : public locale::facet {
public:
  using char_type = charT;
  using iter_type = OutputIterator;

  explicit num_put(size_t refs = 0) : locale::facet(refs) {}

  iter_type put(iter_type s, ios_base& f, char_type fill, bool v) const { return do_put(s, f, fill, v); }
  iter_type put(iter_type s, ios_base& f, char_type fill, long v) const { return do_put(s, f, fill, v); }
  iter_type put(iter_type s, ios_base& f, char_type fill, long long v) const { return do_put(s, f, fill, v); }
  iter_type put(iter_type s, ios_base& f, char_type fill, unsigned long v) const { return do_put(s, f, fill, v); }
  iter_type put(iter_type s, ios_base& f, char_type fill, unsigned long long v) const {
    return do_put(s, f, fill, v);
  }
  iter_type put(iter_type s, ios_base& f, char_type fill, double v) const { return do_put(s, f, fill, v); }
  iter_type put(iter_type s, ios_base& f, char_type fill, long double v) const { return do_put(s, f, fill, v); }
  iter_type put(iter_type s, ios_base& f, char_type fill, const void* v) const { return do_put(s, f, fill, v); }

  static locale::id id;

protected:
  ~num_put() override {}

  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, bool v) const {
    if (!(str.flags() & ios_base::boolalpha))
      return do_put(out, str, fill, static_cast<long>(v));
    const numpunct<charT>& np = use_facet<numpunct<charT>>(str.getloc());
    const basic_string<charT> s = v ? np.truename() : np.falsename();
    // padded as the other conversions (Table 101; no sign, so internal pads before)
    const streamsize width = str.width();
    str.width(0);
    size_t fill_count = width > 0 && static_cast<size_t>(width) > s.size() ? static_cast<size_t>(width) - s.size() : 0;
    const bool left = (str.flags() & ios_base::adjustfield) == ios_base::left;
    if (!left)
      for (; fill_count != 0; --fill_count, ++out)
        *out = fill;
    for (charT c : s) {
      *out = c;
      ++out;
    }
    for (; fill_count != 0; --fill_count, ++out)
      *out = fill;
    return out;
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, long v) const {
    return put_integer(out, str, fill, v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, long long v) const {
    return put_integer(out, str, fill, v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, unsigned long v) const {
    return put_integer(out, str, fill, v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, unsigned long long v) const {
    return put_integer(out, str, fill, v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, double v) const {
    return put_floating(out, str, fill, v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, long double v) const {
    return put_floating(out, str, fill, v);
  }
  virtual iter_type do_put(iter_type out, ios_base& str, char_type fill, const void* v) const {
    char buf[72];
    size_t pad = 0;
    const size_t n = ycxx::detail::num_put_pointer(buf, v, &pad);
    return ycxx::detail::num_put_output(out, str, fill, buf, n, pad, 0, 0);
  }

private:
  template <class T>
  static iter_type put_integer(iter_type out, ios_base& str, char_type fill, T v) {
    char buf[72];
    size_t pad = 0;
    bool neg = false;
    unsigned long long mag;
    if constexpr (is_signed_v<T>) {
      neg = v < 0;
      mag = neg ? 0ull - static_cast<unsigned long long>(v) : static_cast<unsigned long long>(v);
      // %o / %x / %X convert the value as unsigned: the bits of v in its own width
      const ios_base::fmtflags base = str.flags() & ios_base::basefield;
      if (neg && (base == ios_base::oct || base == ios_base::hex)) {
        mag = static_cast<make_unsigned_t<T>>(v);
        neg = false;
      }
    } else {
      mag = v;
    }
    const size_t n = ycxx::detail::num_put_integer(buf, mag, neg, is_signed_v<T>, str.flags(), &pad);
    return ycxx::detail::num_put_output(out, str, fill, buf, n, pad, pad, n);
  }

  template <class T>
  static iter_type put_floating(iter_type out, ios_base& str, char_type fill, T v) {
    char local[128];
    size_t pad = 0;
    size_t n = ycxx::detail::num_put_float(local, sizeof local, v, str.flags(), str.precision(), &pad);
    if (n <= sizeof local)
      return finish_floating(out, str, fill, local, n, pad);
    ycxx::detail::small_buffer<char, 1> big(n);
    n = ycxx::detail::num_put_float(big.get(), n, v, str.flags(), str.precision(), &pad);
    return finish_floating(out, str, fill, big.get(), n, pad);
  }
  static iter_type finish_floating(iter_type out, ios_base& str, char_type fill, const char* s, size_t n, size_t pad) {
    // the integer digits of the mantissa: decimal digits from pad up to '.', the exponent or
    // the end (none for inf / nan, and none for %a, whose digits are hexadecimal)
    size_t gend = pad;
    const bool hexfloat = pad >= 2 && (s[pad - 1] == 'x' || s[pad - 1] == 'X');
    if (!hexfloat)
      while (gend < n && s[gend] >= '0' && s[gend] <= '9')
        ++gend;
    return ycxx::detail::num_put_output(out, str, fill, s, n, pad, pad, gend);
  }
};
template <class charT, class OutputIterator>
locale::id num_put<charT, OutputIterator>::id;

} // namespace std
