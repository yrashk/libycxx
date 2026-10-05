// libycxx hosted: the manipulators of <iomanip> ([iostream.format] [std.manip], [ext.manip],
// [quoted.manip]). Each returns a small object in ycxx::detail whose stream operators (hidden
// friends) perform the action.
#pragma once

#include <ycxx/hosted/istream.hpp>
#include <ycxx/hosted/locale_extra.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class T>
inline constexpr bool is_basic_string = false;
template <class charT, class traits, class Allocator>
inline constexpr bool is_basic_string<std::basic_string<charT, traits, Allocator>> = true;

// [std.manip]: f(ios_base&, ...) applied by both << and >>.
template <class F>
struct ios_manip {
  F f;
  template <class charT, class traits>
  friend std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out, const ios_manip& m) {
    m.f(out);
    return out;
  }
  template <class charT, class traits>
  friend std::basic_istream<charT, traits>& operator>>(std::basic_istream<charT, traits>& in, const ios_manip& m) {
    m.f(in);
    return in;
  }
};

struct resetiosflags_fn {
  std::ios_base::fmtflags mask;
  void operator()(std::ios_base& str) const { str.setf(std::ios_base::fmtflags(0), mask); }
};
struct setiosflags_fn {
  std::ios_base::fmtflags mask;
  void operator()(std::ios_base& str) const { str.setf(mask); }
};
struct setbase_fn {
  int base;
  void operator()(std::ios_base& str) const {
    str.setf(base == 8    ? std::ios_base::oct
             : base == 10 ? std::ios_base::dec
             : base == 16 ? std::ios_base::hex
                          : std::ios_base::fmtflags(0),
             std::ios_base::basefield);
  }
};
struct setprecision_fn {
  int n;
  void operator()(std::ios_base& str) const { str.precision(n); }
};
struct setw_fn {
  int n;
  void operator()(std::ios_base& str) const { str.width(n); }
};

template <class charT>
struct setfill_manip {
  charT c;
  template <class traits>
  friend std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out, const setfill_manip& m) {
    out.fill(m.c);
    return out;
  }
};

// [ext.manip]
template <class moneyT>
struct get_money_manip {
  moneyT* mon;
  bool intl;
  template <class charT, class traits>
  friend std::basic_istream<charT, traits>& operator>>(std::basic_istream<charT, traits>& in, const get_money_manip& m) {
    std::ios_base::iostate err = std::ios_base::goodbit;
    if (typename std::basic_istream<charT, traits>::sentry ok{in}) {
      ::ycxx::detail::guarded_io(in, [&] {
        using Iter = std::istreambuf_iterator<charT, traits>;
        const auto& mg = std::use_facet<std::money_get<charT, Iter>>(in.getloc());
        mg.get(Iter(in.rdbuf()), Iter(), m.intl, in, err, *m.mon);
      });
    }
    if (err != std::ios_base::goodbit)
      in.setstate(err);
    return in;
  }
};
template <class moneyT>
struct put_money_manip {
  const moneyT* mon;
  bool intl;
  template <class charT, class traits>
  friend std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out, const put_money_manip& m) {
    std::ios_base::iostate err = std::ios_base::goodbit;
    if (typename std::basic_ostream<charT, traits>::sentry ok{out}) {
      ::ycxx::detail::guarded_io(out, [&] {
        using Iter = std::ostreambuf_iterator<charT, traits>;
        const auto& mp = std::use_facet<std::money_put<charT, Iter>>(out.getloc());
        if (mp.put(Iter(out.rdbuf()), m.intl, out, out.fill(), *m.mon).failed())
          err |= std::ios_base::badbit;
      });
    }
    if (err != std::ios_base::goodbit)
      out.setstate(err);
    return out;
  }
};
template <class charT>
struct get_time_manip {
  std::tm* tmb;
  const charT* fmt;
  // [ext.manip]/8: f(in, tmb, fmt) itself (not a formatted input function)
  template <class traits>
  friend std::basic_istream<charT, traits>& operator>>(std::basic_istream<charT, traits>& in, const get_time_manip& m) {
    using Iter = std::istreambuf_iterator<charT, traits>;
    std::ios_base::iostate err = std::ios_base::goodbit;
    const auto& tg = std::use_facet<std::time_get<charT, Iter>>(in.getloc());
    tg.get(Iter(in.rdbuf()), Iter(), in, err, m.tmb, m.fmt, m.fmt + traits::length(m.fmt));
    if (err != std::ios_base::goodbit)
      in.setstate(err);
    return in;
  }
};
template <class charT>
struct put_time_manip {
  const std::tm* tmb;
  const charT* fmt;
  // [ext.manip]/10: f(out, tmb, fmt) itself
  template <class traits>
  friend std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out, const put_time_manip& m) {
    using Iter = std::ostreambuf_iterator<charT, traits>;
    const auto& tp = std::use_facet<std::time_put<charT, Iter>>(out.getloc());
    if (tp.put(Iter(out.rdbuf()), out, out.fill(), m.tmb, m.fmt, m.fmt + traits::length(m.fmt)).failed())
      out.setstate(std::ios_base::badbit);
    return out;
  }
};

// [quoted.manip]: output of [s, s + n) (traits: the required traits_type, or void for any)
template <class charT, class traits>
struct quoted_out {
  const charT* s;
  std::size_t n;
  charT delim, escape;

  template <class T>
    requires(std::is_void_v<traits> || std::is_same_v<T, traits>)
  friend std::basic_ostream<charT, T>& operator<<(std::basic_ostream<charT, T>& out, const quoted_out& q) {
    // the sequence: delim, each character (escape and delim escaped), delim
    std::size_t len = 2;
    for (std::size_t i = 0; i < q.n; ++i)
      len += T::eq(q.s[i], q.delim) || T::eq(q.s[i], q.escape) ? 2 : 1;
    ::ycxx::detail::small_buffer<charT, 128> seq(len);
    charT* p = seq.get();
    *p++ = q.delim;
    for (std::size_t i = 0; i < q.n; ++i) {
      if (T::eq(q.s[i], q.delim) || T::eq(q.s[i], q.escape))
        *p++ = q.escape;
      *p++ = q.s[i];
    }
    *p++ = q.delim;
    return ::ycxx::detail::ostream_insert(out, seq.get(), static_cast<std::ptrdiff_t>(len));
  }
};

template <class charT, class traits, class Allocator>
struct quoted_inout {
  std::basic_string<charT, traits, Allocator>* s;
  charT delim, escape;

  friend std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out, const quoted_inout& q) {
    return out << quoted_out<charT, traits>{q.s->data(), q.s->size(), q.delim, q.escape};
  }
  friend std::basic_istream<charT, traits>& operator>>(std::basic_istream<charT, traits>& in, const quoted_inout& q) {
    charT c;
    if (!(in >> c))
      return in;
    if (!traits::eq(c, q.delim)) {
      in.unget();
      return in >> *q.s;
    }
    const std::ios_base::fmtflags flags = in.flags();
    in.unsetf(std::ios_base::skipws);
    q.s->clear();
    for (;;) {
      if (!(in >> c))
        break;
      if (traits::eq(c, q.delim))
        break;
      if (traits::eq(c, q.escape)) {
        if (!(in >> c))
          break;
      }
      q.s->push_back(c);
    }
    in.flags(flags);
    return in;
  }
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [std.manip]
inline ycxx::detail::ios_manip<ycxx::detail::resetiosflags_fn> resetiosflags(ios_base::fmtflags mask) {
  return {{mask}};
}
inline ycxx::detail::ios_manip<ycxx::detail::setiosflags_fn> setiosflags(ios_base::fmtflags mask) { return {{mask}}; }
inline ycxx::detail::ios_manip<ycxx::detail::setbase_fn> setbase(int base) { return {{base}}; }
template <class charT>
ycxx::detail::setfill_manip<charT> setfill(charT c) {
  return {c};
}
inline ycxx::detail::ios_manip<ycxx::detail::setprecision_fn> setprecision(int n) { return {{n}}; }
inline ycxx::detail::ios_manip<ycxx::detail::setw_fn> setw(int n) { return {{n}}; }

// [ext.manip]
template <class moneyT>
ycxx::detail::get_money_manip<moneyT> get_money(moneyT& mon, bool intl = false) {
  static_assert(is_same_v<moneyT, long double> || ycxx::detail::is_basic_string<moneyT>,
                "std::get_money: moneyT must be long double or a basic_string ([ext.manip]/2)");
  return {__builtin_addressof(mon), intl};
}
template <class moneyT>
ycxx::detail::put_money_manip<moneyT> put_money(const moneyT& mon, bool intl = false) {
  static_assert(is_same_v<moneyT, long double> || ycxx::detail::is_basic_string<moneyT>,
                "std::put_money: moneyT must be long double or a basic_string ([ext.manip]/5)");
  return {__builtin_addressof(mon), intl};
}
template <class charT>
ycxx::detail::get_time_manip<charT> get_time(tm* tmb, const charT* fmt) {
  return {tmb, fmt};
}
template <class charT>
ycxx::detail::put_time_manip<charT> put_time(const tm* tmb, const charT* fmt) {
  return {tmb, fmt};
}

// [quoted.manip]
template <class charT>
ycxx::detail::quoted_out<charT, void> quoted(const charT* s, charT delim = charT('"'), charT escape = charT('\\')) {
  return {s, char_traits<charT>::length(s), delim, escape};
}
template <class charT, class traits, class Allocator>
ycxx::detail::quoted_out<charT, traits> quoted(const basic_string<charT, traits, Allocator>& s,
                                               charT delim = charT('"'), charT escape = charT('\\')) {
  return {s.data(), s.size(), delim, escape};
}
template <class charT, class traits, class Allocator>
ycxx::detail::quoted_inout<charT, traits, Allocator> quoted(basic_string<charT, traits, Allocator>& s,
                                                            charT delim = charT('"'), charT escape = charT('\\')) {
  return {__builtin_addressof(s), delim, escape};
}
template <class charT, class traits>
ycxx::detail::quoted_out<charT, traits> quoted(basic_string_view<charT, traits> s, charT delim = charT('"'),
                                               charT escape = charT('\\')) {
  return {s.data(), s.size(), delim, escape};
}

} // namespace std
