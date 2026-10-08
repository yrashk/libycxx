// libycxx hosted: the manipulators of <iomanip> ([iostream.format] [std.manip], [ext.manip],
// [quoted.manip]). Each returns a small object in __ycxx::__detail whose stream operators (hidden
// friends) perform the action.
#pragma once

#include <ycxx/hosted/istream.hpp>
#include <ycxx/hosted/locale_extra.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __is_basic_string = false;
template <class __charT, class __traits, class _Allocator>
inline constexpr bool __is_basic_string<std::basic_string<__charT, __traits, _Allocator>> = true;

// [std.manip]: f(ios_base&, ...) applied by both << and >>.
template <class _Fp>
struct __ios_manip {
  _Fp __f;
  template <class __charT, class __traits>
  friend std::basic_ostream<__charT, __traits>& operator<<(std::basic_ostream<__charT, __traits>& out, const __ios_manip& m) {
    m.__f(out);
    return out;
  }
  template <class __charT, class __traits>
  friend std::basic_istream<__charT, __traits>& operator>>(std::basic_istream<__charT, __traits>& in, const __ios_manip& m) {
    m.__f(in);
    return in;
  }
};

struct __resetiosflags_fn {
  std::ios_base::fmtflags mask;
  void operator()(std::ios_base& str) const { str.setf(std::ios_base::fmtflags(0), mask); }
};
struct __setiosflags_fn {
  std::ios_base::fmtflags mask;
  void operator()(std::ios_base& str) const { str.setf(mask); }
};
struct __setbase_fn {
  int base;
  void operator()(std::ios_base& str) const {
    str.setf(base == 8    ? std::ios_base::oct
             : base == 10 ? std::ios_base::dec
             : base == 16 ? std::ios_base::hex
                          : std::ios_base::fmtflags(0),
             std::ios_base::basefield);
  }
};
struct __setprecision_fn {
  int n;
  void operator()(std::ios_base& str) const { str.precision(n); }
};
struct __setw_fn {
  int n;
  void operator()(std::ios_base& str) const { str.width(n); }
};

template <class __charT>
struct __setfill_manip {
  __charT c;
  template <class __traits>
  friend std::basic_ostream<__charT, __traits>& operator<<(std::basic_ostream<__charT, __traits>& out, const __setfill_manip& m) {
    out.fill(m.c);
    return out;
  }
};

// [ext.manip]
template <class __moneyT>
struct __get_money_manip {
  __moneyT* __mon;
  bool intl;
  template <class __charT, class __traits>
  friend std::basic_istream<__charT, __traits>& operator>>(std::basic_istream<__charT, __traits>& in, const __get_money_manip& m) {
    std::ios_base::iostate __err = std::ios_base::goodbit;
    if (typename std::basic_istream<__charT, __traits>::sentry ok{in}) {
      ::__ycxx::__detail::__guarded_io(in, [&] {
        using _Iter = std::istreambuf_iterator<__charT, __traits>;
        const auto& __mg = std::use_facet<std::money_get<__charT, _Iter>>(in.getloc());
        __mg.get(_Iter(in.rdbuf()), _Iter(), m.intl, in, __err, *m.__mon);
      });
    }
    if (__err != std::ios_base::goodbit)
      in.setstate(__err);
    return in;
  }
};
template <class __moneyT>
struct __put_money_manip {
  const __moneyT* __mon;
  bool intl;
  template <class __charT, class __traits>
  friend std::basic_ostream<__charT, __traits>& operator<<(std::basic_ostream<__charT, __traits>& out, const __put_money_manip& m) {
    std::ios_base::iostate __err = std::ios_base::goodbit;
    if (typename std::basic_ostream<__charT, __traits>::sentry ok{out}) {
      ::__ycxx::__detail::__guarded_io(out, [&] {
        using _Iter = std::ostreambuf_iterator<__charT, __traits>;
        const auto& __mp = std::use_facet<std::money_put<__charT, _Iter>>(out.getloc());
        if (__mp.put(_Iter(out.rdbuf()), m.intl, out, out.fill(), *m.__mon).failed())
          __err |= std::ios_base::badbit;
      });
    }
    if (__err != std::ios_base::goodbit)
      out.setstate(__err);
    return out;
  }
};
template <class __charT>
struct __get_time_manip {
  std::tm* __tmb;
  const __charT* __fmt;
  // [ext.manip]/8: f(in, tmb, fmt) itself (not a formatted input function)
  template <class __traits>
  friend std::basic_istream<__charT, __traits>& operator>>(std::basic_istream<__charT, __traits>& in, const __get_time_manip& m) {
    using _Iter = std::istreambuf_iterator<__charT, __traits>;
    std::ios_base::iostate __err = std::ios_base::goodbit;
    const auto& __tg = std::use_facet<std::time_get<__charT, _Iter>>(in.getloc());
    __tg.get(_Iter(in.rdbuf()), _Iter(), in, __err, m.__tmb, m.__fmt, m.__fmt + __traits::length(m.__fmt));
    if (__err != std::ios_base::goodbit)
      in.setstate(__err);
    return in;
  }
};
template <class __charT>
struct __put_time_manip {
  const std::tm* __tmb;
  const __charT* __fmt;
  // [ext.manip]/10: f(out, tmb, fmt) itself
  template <class __traits>
  friend std::basic_ostream<__charT, __traits>& operator<<(std::basic_ostream<__charT, __traits>& out, const __put_time_manip& m) {
    using _Iter = std::ostreambuf_iterator<__charT, __traits>;
    const auto& __tp = std::use_facet<std::time_put<__charT, _Iter>>(out.getloc());
    if (__tp.put(_Iter(out.rdbuf()), out, out.fill(), m.__tmb, m.__fmt, m.__fmt + __traits::length(m.__fmt)).failed())
      out.setstate(std::ios_base::badbit);
    return out;
  }
};

// [quoted.manip]: output of [s, s + n) (traits: the required traits_type, or void for any)
template <class __charT, class __traits>
struct __quoted_out {
  const __charT* s;
  std::size_t n;
  __charT __delim, __escape;

  template <class _Tp>
    requires(std::is_void_v<__traits> || std::is_same_v<_Tp, __traits>)
  friend std::basic_ostream<__charT, _Tp>& operator<<(std::basic_ostream<__charT, _Tp>& out, const __quoted_out& __q) {
    // the sequence: delim, each character (escape and delim escaped), delim
    std::size_t __len = 2;
    for (std::size_t i = 0; i < __q.n; ++i)
      __len += _Tp::eq(__q.s[i], __q.__delim) || _Tp::eq(__q.s[i], __q.__escape) ? 2 : 1;
    ::__ycxx::__detail::__small_buffer<__charT, 128> seq(__len);
    __charT* p = seq.get();
    *p++ = __q.__delim;
    for (std::size_t i = 0; i < __q.n; ++i) {
      if (_Tp::eq(__q.s[i], __q.__delim) || _Tp::eq(__q.s[i], __q.__escape))
        *p++ = __q.__escape;
      *p++ = __q.s[i];
    }
    *p++ = __q.__delim;
    return ::__ycxx::__detail::__ostream_insert(out, seq.get(), static_cast<std::ptrdiff_t>(__len));
  }
};

template <class __charT, class __traits, class _Allocator>
struct __quoted_inout {
  std::basic_string<__charT, __traits, _Allocator>* s;
  __charT __delim, __escape;

  friend std::basic_ostream<__charT, __traits>& operator<<(std::basic_ostream<__charT, __traits>& out, const __quoted_inout& __q) {
    return out << __quoted_out<__charT, __traits>{__q.s->data(), __q.s->size(), __q.__delim, __q.__escape};
  }
  friend std::basic_istream<__charT, __traits>& operator>>(std::basic_istream<__charT, __traits>& in, const __quoted_inout& __q) {
    __charT c;
    if (!(in >> c))
      return in;
    if (!__traits::eq(c, __q.__delim)) {
      in.unget();
      return in >> *__q.s;
    }
    const std::ios_base::fmtflags flags = in.flags();
    in.unsetf(std::ios_base::skipws);
    __q.s->clear();
    for (;;) {
      if (!(in >> c))
        break;
      if (__traits::eq(c, __q.__delim))
        break;
      if (__traits::eq(c, __q.__escape)) {
        if (!(in >> c))
          break;
      }
      __q.s->push_back(c);
    }
    in.flags(flags);
    return in;
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [std.manip]
inline __ycxx::__detail::__ios_manip<__ycxx::__detail::__resetiosflags_fn> resetiosflags(ios_base::fmtflags mask) {
  return {{mask}};
}
inline __ycxx::__detail::__ios_manip<__ycxx::__detail::__setiosflags_fn> setiosflags(ios_base::fmtflags mask) { return {{mask}}; }
inline __ycxx::__detail::__ios_manip<__ycxx::__detail::__setbase_fn> setbase(int base) { return {{base}}; }
template <class __charT>
__ycxx::__detail::__setfill_manip<__charT> setfill(__charT c) {
  return {c};
}
inline __ycxx::__detail::__ios_manip<__ycxx::__detail::__setprecision_fn> setprecision(int n) { return {{n}}; }
inline __ycxx::__detail::__ios_manip<__ycxx::__detail::__setw_fn> setw(int n) { return {{n}}; }

// [ext.manip]
template <class __moneyT>
__ycxx::__detail::__get_money_manip<__moneyT> get_money(__moneyT& __mon, bool intl = false) {
  static_assert(is_same_v<__moneyT, long double> || __ycxx::__detail::__is_basic_string<__moneyT>,
                "std::get_money: moneyT must be long double or a basic_string ([ext.manip]/2)");
  return {__builtin_addressof(__mon), intl};
}
template <class __moneyT>
__ycxx::__detail::__put_money_manip<__moneyT> put_money(const __moneyT& __mon, bool intl = false) {
  static_assert(is_same_v<__moneyT, long double> || __ycxx::__detail::__is_basic_string<__moneyT>,
                "std::put_money: moneyT must be long double or a basic_string ([ext.manip]/5)");
  return {__builtin_addressof(__mon), intl};
}
template <class __charT>
__ycxx::__detail::__get_time_manip<__charT> get_time(tm* __tmb, const __charT* __fmt) {
  return {__tmb, __fmt};
}
template <class __charT>
__ycxx::__detail::__put_time_manip<__charT> put_time(const tm* __tmb, const __charT* __fmt) {
  return {__tmb, __fmt};
}

// [quoted.manip]
template <class __charT>
__ycxx::__detail::__quoted_out<__charT, void> quoted(const __charT* s, __charT __delim = __charT('"'), __charT __escape = __charT('\\')) {
  return {s, char_traits<__charT>::length(s), __delim, __escape};
}
template <class __charT, class __traits, class _Allocator>
__ycxx::__detail::__quoted_out<__charT, __traits> quoted(const basic_string<__charT, __traits, _Allocator>& s,
                                               __charT __delim = __charT('"'), __charT __escape = __charT('\\')) {
  return {s.data(), s.size(), __delim, __escape};
}
template <class __charT, class __traits, class _Allocator>
__ycxx::__detail::__quoted_inout<__charT, __traits, _Allocator> quoted(basic_string<__charT, __traits, _Allocator>& s,
                                                            __charT __delim = __charT('"'), __charT __escape = __charT('\\')) {
  return {__builtin_addressof(s), __delim, __escape};
}
template <class __charT, class __traits>
__ycxx::__detail::__quoted_out<__charT, __traits> quoted(basic_string_view<__charT, __traits> s, __charT __delim = __charT('"'),
                                               __charT __escape = __charT('\\')) {
  return {s.data(), s.size(), __delim, __escape};
}

}} // namespace std
