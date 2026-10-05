// Test-harness shim (see bits/c++config.h), included by testsuite_character.h.
//
// The helper defines pod_int and pod_state (used by many algorithm tests as plain element types)
// and the alternate character types pod_char, pod_uchar, pod_ushort, pod_uint as specializations
// of libstdc++'s __gnu_cxx::character<V, I, S>, whose std::char_traits specialization
// (ext/pod_char_traits.h) is a libstdc++ extension: its semantics (eof(), to_int_type, ...) are
// not in the standard. The helper's non-template numpunct<pod_uint> and moneypunct<pod_uint>
// specializations instantiate basic_string<pod_uint>, so a char_traits specialization is needed
// for the helper to compile at all: the one below is a harness-side char_traits written from the
// requirements of [char.traits.require] (eq/lt on the value; to_int_type/to_char_type through the
// character's to/from, which the helper specializes for pod_char and pod_uchar; eof() an int_type
// value no character converts to, provided for integral int_type only). libstdc++'s choices may
// differ, so tests that use those character types stay skipped (tests/libstdcxx/skip.txt).
//
// The helper also specializes std::codecvt and std::ctype for pod_uchar, deriving from
// libstdc++'s internal bases std::__codecvt_abstract_base and std::__ctype_abstract_base. Those are
// given here as harness-side equivalents written from [locale.codecvt] and [locale.ctype]: the
// public members call the protected virtual do_ members, which a specialization overrides.
#pragma once
#include <cstddef>
#include <cwchar>
#include <ios>
#include <locale>
#include <string>
#include <type_traits>

namespace __gnu_cxx {
template <class V, class I, class S = std::mbstate_t>
struct character {
  using value_type = V;
  using int_type = I;
  using state_type = S;
  using char_type = character;

  value_type value;

  // Conversions from and to int_type (the helper specializes them for pod_char and pod_uchar).
  template <class V2>
  static char_type from(const V2& v) {
    return char_type{static_cast<value_type>(v)};
  }
  template <class V2>
  static V2 to(const char_type& c) {
    return static_cast<V2>(c.value);
  }
};
} // namespace __gnu_cxx

namespace std {
template <class V, class I, class S>
struct char_traits<__gnu_cxx::character<V, I, S>> {
  using char_type = __gnu_cxx::character<V, I, S>;
  using int_type = I;
  using off_type = streamoff;
  using pos_type = fpos<S>;
  using state_type = S;

  static void assign(char_type& r, const char_type& a) noexcept { r = a; }
  static bool eq(const char_type& a, const char_type& b) noexcept { return a.value == b.value; }
  static bool lt(const char_type& a, const char_type& b) noexcept { return a.value < b.value; }
  static int compare(const char_type* p, const char_type* q, size_t n) {
    for (size_t i = 0; i != n; ++i) {
      if (lt(p[i], q[i]))
        return -1;
      if (lt(q[i], p[i]))
        return 1;
    }
    return 0;
  }
  static size_t length(const char_type* p) {
    size_t n = 0;
    while (!eq(p[n], char_type()))
      ++n;
    return n;
  }
  static const char_type* find(const char_type* p, size_t n, const char_type& c) {
    for (size_t i = 0; i != n; ++i)
      if (eq(p[i], c))
        return p + i;
    return nullptr;
  }
  static char_type* move(char_type* s, const char_type* p, size_t n) {
    if (n != 0)
      __builtin_memmove(s, p, n * sizeof(char_type));
    return s;
  }
  static char_type* copy(char_type* s, const char_type* p, size_t n) {
    for (size_t i = 0; i != n; ++i)
      s[i] = p[i];
    return s;
  }
  static char_type* assign(char_type* s, size_t n, char_type c) {
    for (size_t i = 0; i != n; ++i)
      s[i] = c;
    return s;
  }
  static char_type to_char_type(const int_type& i) { return char_type::template from<int_type>(i); }
  static int_type to_int_type(const char_type& c) { return char_type::template to<int_type>(c); }
  static bool eq_int_type(const int_type& a, const int_type& b) { return a == b; }
  static int_type eof() requires is_integral_v<I> { return static_cast<int_type>(-1); }
  static int_type not_eof(const int_type& e) requires is_integral_v<I> { return e == eof() ? int_type(0) : e; }
};

template <class InternT, class ExternT, class StateT>
class __codecvt_abstract_base : public locale::facet, public codecvt_base {
public:
  using intern_type = InternT;
  using extern_type = ExternT;
  using state_type = StateT;

  result out(state_type& state, const intern_type* from, const intern_type* from_end,
             const intern_type*& from_next, extern_type* to, extern_type* to_end,
             extern_type*& to_next) const {
    return do_out(state, from, from_end, from_next, to, to_end, to_next);
  }
  result unshift(state_type& state, extern_type* to, extern_type* to_end, extern_type*& to_next) const {
    return do_unshift(state, to, to_end, to_next);
  }
  result in(state_type& state, const extern_type* from, const extern_type* from_end,
            const extern_type*& from_next, intern_type* to, intern_type* to_end,
            intern_type*& to_next) const {
    return do_in(state, from, from_end, from_next, to, to_end, to_next);
  }
  int encoding() const noexcept { return do_encoding(); }
  bool always_noconv() const noexcept { return do_always_noconv(); }
  int length(state_type& state, const extern_type* from, const extern_type* end, size_t max) const {
    return do_length(state, from, end, max);
  }
  int max_length() const noexcept { return do_max_length(); }

protected:
  explicit __codecvt_abstract_base(size_t refs = 0) : locale::facet(refs) {}
  virtual ~__codecvt_abstract_base() {}

  virtual result do_out(state_type&, const intern_type*, const intern_type*, const intern_type*&,
                        extern_type*, extern_type*, extern_type*&) const = 0;
  virtual result do_in(state_type&, const extern_type*, const extern_type*, const extern_type*&,
                       intern_type*, intern_type*, intern_type*&) const = 0;
  virtual result do_unshift(state_type&, extern_type*, extern_type*, extern_type*&) const = 0;
  virtual int do_encoding() const noexcept = 0;
  virtual bool do_always_noconv() const noexcept = 0;
  virtual int do_length(state_type&, const extern_type*, const extern_type*, size_t) const = 0;
  virtual int do_max_length() const noexcept = 0;
};

template <class CharT>
class __ctype_abstract_base : public locale::facet, public ctype_base {
public:
  using char_type = CharT;

  bool is(mask m, char_type c) const { return do_is(m, c); }
  const char_type* is(const char_type* low, const char_type* high, mask* vec) const {
    return do_is(low, high, vec);
  }
  const char_type* scan_is(mask m, const char_type* low, const char_type* high) const {
    return do_scan_is(m, low, high);
  }
  const char_type* scan_not(mask m, const char_type* low, const char_type* high) const {
    return do_scan_not(m, low, high);
  }
  char_type toupper(char_type c) const { return do_toupper(c); }
  const char_type* toupper(char_type* low, const char_type* high) const { return do_toupper(low, high); }
  char_type tolower(char_type c) const { return do_tolower(c); }
  const char_type* tolower(char_type* low, const char_type* high) const { return do_tolower(low, high); }
  char_type widen(char c) const { return do_widen(c); }
  const char* widen(const char* low, const char* high, char_type* to) const { return do_widen(low, high, to); }
  char narrow(char_type c, char dfault) const { return do_narrow(c, dfault); }
  const char_type* narrow(const char_type* low, const char_type* high, char dfault, char* to) const {
    return do_narrow(low, high, dfault, to);
  }

protected:
  explicit __ctype_abstract_base(size_t refs = 0) : locale::facet(refs) {}
  virtual ~__ctype_abstract_base() {}

  virtual bool do_is(mask, char_type) const = 0;
  virtual const char_type* do_is(const char_type*, const char_type*, mask*) const = 0;
  virtual const char_type* do_scan_is(mask, const char_type*, const char_type*) const = 0;
  virtual const char_type* do_scan_not(mask, const char_type*, const char_type*) const = 0;
  virtual char_type do_toupper(char_type) const = 0;
  virtual const char_type* do_toupper(char_type*, const char_type*) const = 0;
  virtual char_type do_tolower(char_type) const = 0;
  virtual const char_type* do_tolower(char_type*, const char_type*) const = 0;
  virtual char_type do_widen(char) const = 0;
  virtual const char* do_widen(const char*, const char*, char_type*) const = 0;
  virtual char do_narrow(char_type, char) const = 0;
  virtual const char_type* do_narrow(const char_type*, const char_type*, char, char*) const = 0;
};
} // namespace std
