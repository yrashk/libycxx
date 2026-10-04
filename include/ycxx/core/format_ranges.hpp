// libycxx core: formatting of ranges ([format.range]) and tuples ([format.tuple]), and the
// formatters of the container adaptors ([container.adaptors.format]) and of
// vector<bool>::reference ([vector.bool.fmt]).
//
// The latter two are defined here against declarations of the adaptors and of
// vector<bool>::reference (ycxx::adl_free::bit_ref), so <stack>, <queue> and <vector> need not
// include the formatting library: the specializations are usable wherever both the container
// header and <format> are included.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/ranges_all.hpp>
#include <ycxx/core/tuple.hpp>

namespace std {
template <class T, class Container>
class stack;
template <class T, class Container>
class queue;
template <class T, class Container, class Compare>
class priority_queue;
} // namespace std

namespace ycxx::adl_free {
template <class Word>
class bit_ref;
} // namespace ycxx::adl_free

namespace ycxx::detail {
template <class T>
inline constexpr bool fmt_is_pair_or_2tuple = false;
template <class T, class U>
inline constexpr bool fmt_is_pair_or_2tuple<std::pair<T, U>> = true;
template <class T, class U>
inline constexpr bool fmt_is_pair_or_2tuple<std::tuple<T, U>> = true;

template <class R>
inline constexpr bool fmt_dependent_false = false;
template <class F>
constexpr void fmt_set_debug(F& f) {
  if constexpr (requires { f.set_debug_format(); })
    f.set_debug_format();
}
} // namespace ycxx::detail

namespace std {

// [format.range.fmtkind]
enum class range_format { disabled, map, set, sequence, string, debug_string };

} // namespace std

namespace ycxx::detail {
template <class R>
consteval std::range_format fmt_kind_primary() {
  static_assert(fmt_dependent_false<R>, "std::format_kind: the primary template is instantiated ([format.range.fmtkind]/1)");
  return std::range_format::disabled;
}
template <class R>
consteval std::range_format fmt_default_kind() {
  using U = std::remove_cvref_t<std::ranges::range_reference_t<R>>;
  if constexpr (__is_same(U, R))
    return std::range_format::disabled;
  else if constexpr (requires { typename R::key_type; }) {
    if constexpr (requires { typename R::mapped_type; } && fmt_is_pair_or_2tuple<U>)
      return std::range_format::map;
    else
      return std::range_format::set;
  } else
    return std::range_format::sequence;
}
} // namespace ycxx::detail

namespace std {

template <class R>
inline constexpr range_format format_kind = ycxx::detail::fmt_kind_primary<R>();
template <ranges::input_range R>
  requires same_as<R, remove_cvref_t<R>>
inline constexpr range_format format_kind<R> = ycxx::detail::fmt_default_kind<R>();

// [format.range.formatter]
template <class T, class charT = char>
  requires same_as<remove_cvref_t<T>, T> && formattable<T, charT>
class range_formatter {
  enum class kind : unsigned char { sequence, string, debug_string };

  formatter<T, charT> underlying_;
  basic_string_view<charT> separator_ = ycxx::detail::fmt_lit<charT>(", ", L", ");
  basic_string_view<charT> opening_ = ycxx::detail::fmt_lit<charT>("[", L"[");
  basic_string_view<charT> closing_ = ycxx::detail::fmt_lit<charT>("]", L"]");
  ycxx::detail::fmt_spec<charT> spec_; // range-fill-and-align and width
  kind kind_ = kind::sequence;

  template <class R, class FormatContext>
  constexpr void write_elements(R& r, FormatContext& ctx) const {
    ctx.advance_to(ycxx::detail::fmt_put<charT>(ctx.out(), opening_.data(), opening_.size()));
    bool first = true;
    auto it = ranges::begin(r);
    const auto last = ranges::end(r);
    for (; it != last; ++it) {
      if (!first)
        ctx.advance_to(ycxx::detail::fmt_put<charT>(ctx.out(), separator_.data(), separator_.size()));
      first = false;
      ctx.advance_to(underlying_.format(*it, ctx));
    }
    ctx.advance_to(ycxx::detail::fmt_put<charT>(ctx.out(), closing_.data(), closing_.size()));
  }

public:
  constexpr void set_separator(basic_string_view<charT> sep) noexcept { separator_ = sep; }
  constexpr void set_brackets(basic_string_view<charT> opening, basic_string_view<charT> closing) noexcept {
    opening_ = opening;
    closing_ = closing;
  }
  constexpr formatter<T, charT>& underlying() noexcept { return underlying_; }
  constexpr const formatter<T, charT>& underlying() const noexcept { return underlying_; }

  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& ctx) {
    auto p = ctx.begin();
    const auto e = ctx.end();
    p = ycxx::detail::fmt_parse_fill_align(p, e, spec_, false);
    p = ycxx::detail::fmt_parse_width(ctx, p, e, spec_);
    bool no_brackets = false;
    if (p != e && *p == charT('n')) {
      no_brackets = true;
      ++p;
    }
    bool map = false;
    if (p != e && *p == charT('m')) {
      if constexpr (!ycxx::detail::fmt_is_pair_or_2tuple<T>)
        ycxx::detail::throw_format_error("std::range_formatter: m needs elements that are pairs or 2-tuples");
      map = true;
      ++p;
    } else if (p != e && (*p == charT('s') || *p == charT('?'))) {
      if (*p == charT('?')) {
        ++p;
        if (p == e || *p != charT('s'))
          ycxx::detail::throw_format_error("std::range_formatter: ? must be followed by s");
        kind_ = kind::debug_string;
      } else {
        kind_ = kind::string;
      }
      ++p;
      if constexpr (!same_as<T, charT>)
        ycxx::detail::throw_format_error("std::range_formatter: s and ?s need a range of the character type");
      if (no_brackets)
        ycxx::detail::throw_format_error("std::range_formatter: n cannot be combined with s or ?s");
    }
    const bool has_underlying = p != e && *p == charT(':');
    if (has_underlying) {
      if (kind_ != kind::sequence)
        ycxx::detail::throw_format_error("std::range_formatter: s and ?s take no underlying format-spec");
      ++p;
    } else if (p != e && *p != charT('}')) {
      ycxx::detail::throw_format_error("std::range_formatter: invalid range-format-spec");
    }
    if (map) {
      if constexpr (ycxx::detail::fmt_is_pair_or_2tuple<T>) {
        set_brackets(ycxx::detail::fmt_lit<charT>("{", L"{"), ycxx::detail::fmt_lit<charT>("}", L"}"));
        set_separator(ycxx::detail::fmt_lit<charT>(", ", L", "));
        underlying_.set_brackets({}, {});
        underlying_.set_separator(ycxx::detail::fmt_lit<charT>(": ", L": "));
      }
    }
    if (no_brackets)
      set_brackets({}, {});
    ctx.advance_to(p);
    p = underlying_.parse(ctx);
    if constexpr (requires { underlying_.set_debug_format(); }) {
      if (kind_ == kind::sequence && !has_underlying)
        underlying_.set_debug_format();
    }
    return p;
  }

  template <ranges::input_range R, class FormatContext>
    requires formattable<ranges::range_reference_t<R>, charT> && same_as<remove_cvref_t<ranges::range_reference_t<R>>, T>
  constexpr typename FormatContext::iterator format(R&& r, FormatContext& ctx) const {
    if constexpr (same_as<T, charT>) {
      if (kind_ != kind::sequence) {
        ycxx::detail::fmt_spec<charT> s = spec_;
        s.type = kind_ == kind::debug_string ? '?' : 0;
        if constexpr (ranges::contiguous_range<R> && ranges::sized_range<R>) {
          return ycxx::detail::fmt_write_string(ctx, ranges::data(r), static_cast<size_t>(ranges::size(r)), s);
        } else {
          const basic_string<charT> str(from_range, r);
          return ycxx::detail::fmt_write_string(ctx, str.data(), str.size(), s);
        }
      }
    }
    const size_t width = ycxx::detail::fmt_width(spec_, ctx);
    if (width == 0) {
      write_elements(r, ctx);
      return ctx.out();
    }
    ycxx::detail::fmt_dynbuf<charT> tmp;
    auto tctx = ycxx::detail::fmt_access::context_like(ctx, tmp);
    write_elements(r, tctx);
    return ycxx::detail::fmt_write_padded<charT>(ctx.out(), spec_, ycxx::detail::fmt_align::left, width,
                                                 ycxx::detail::uni::width(tmp.data(), tmp.size()), tmp.data(),
                                                 tmp.size());
  }
};

} // namespace std

namespace ycxx::detail {
template <class R, class charT>
concept fmt_const_formattable_range =
    std::ranges::input_range<const R> && std::formattable<std::ranges::range_reference_t<const R>, charT>;
template <class R, class charT>
using fmt_maybe_const = std::conditional_t<fmt_const_formattable_range<R, charT>, const R, R>;
} // namespace ycxx::detail

namespace ycxx::adl_free {

// range-default-formatter ([format.range.fmtdef], [format.range.fmtmap], [format.range.fmtset],
// [format.range.fmtstr]).
template <std::range_format K, std::ranges::input_range R, class charT>
struct fmt_range_default;

template <std::ranges::input_range R, class charT>
struct fmt_range_default<std::range_format::sequence, R, charT> {
private:
  using maybe_const_r = ycxx::detail::fmt_maybe_const<R, charT>;
  std::range_formatter<std::remove_cvref_t<std::ranges::range_reference_t<maybe_const_r>>, charT> underlying_;

public:
  constexpr void set_separator(std::basic_string_view<charT> sep) noexcept { underlying_.set_separator(sep); }
  constexpr void set_brackets(std::basic_string_view<charT> opening, std::basic_string_view<charT> closing) noexcept {
    underlying_.set_brackets(opening, closing);
  }
  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& ctx) {
    return underlying_.parse(ctx);
  }
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(maybe_const_r& elems, FormatContext& ctx) const {
    return underlying_.format(elems, ctx);
  }
};

template <std::ranges::input_range R, class charT>
struct fmt_range_default<std::range_format::map, R, charT> {
private:
  using maybe_const_map = ycxx::detail::fmt_maybe_const<R, charT>;
  using element_type = std::remove_cvref_t<std::ranges::range_reference_t<maybe_const_map>>;
  std::range_formatter<element_type, charT> underlying_;

public:
  constexpr fmt_range_default() {
    static_assert(ycxx::detail::fmt_is_pair_or_2tuple<element_type>,
                  "std::formatter: a map's elements must be pairs or 2-tuples ([format.range.fmtmap]/1)");
    underlying_.set_brackets(ycxx::detail::fmt_lit<charT>("{", L"{"), ycxx::detail::fmt_lit<charT>("}", L"}"));
    underlying_.underlying().set_brackets({}, {});
    underlying_.underlying().set_separator(ycxx::detail::fmt_lit<charT>(": ", L": "));
  }
  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& ctx) {
    return underlying_.parse(ctx);
  }
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(maybe_const_map& r, FormatContext& ctx) const {
    return underlying_.format(r, ctx);
  }
};

template <std::ranges::input_range R, class charT>
struct fmt_range_default<std::range_format::set, R, charT> {
private:
  using maybe_const_set = ycxx::detail::fmt_maybe_const<R, charT>;
  std::range_formatter<std::remove_cvref_t<std::ranges::range_reference_t<maybe_const_set>>, charT> underlying_;

public:
  constexpr fmt_range_default() {
    underlying_.set_brackets(ycxx::detail::fmt_lit<charT>("{", L"{"), ycxx::detail::fmt_lit<charT>("}", L"}"));
  }
  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& ctx) {
    return underlying_.parse(ctx);
  }
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(maybe_const_set& r, FormatContext& ctx) const {
    return underlying_.format(r, ctx);
  }
};

// string and debug_string: formatted as basic_string<charT> (a view of the elements when the
// range is contiguous, which formats the same).
template <std::range_format K, std::ranges::input_range R, class charT>
  requires(K == std::range_format::string || K == std::range_format::debug_string)
struct fmt_range_default<K, R, charT> {
private:
  static_assert(__is_same(std::remove_cvref_t<std::ranges::range_reference_t<R>>, charT),
                "std::formatter: a range formatted as a string must have elements of the character type "
                "([format.range.fmtstr]/1)");
  std::formatter<std::basic_string_view<charT>, charT> underlying_;
  using str_r = std::conditional_t<std::ranges::input_range<const R>, const R, R>;

public:
  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& ctx) {
    auto i = underlying_.parse(ctx);
    if constexpr (K == std::range_format::debug_string)
      underlying_.set_debug_format();
    return i;
  }
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(str_r& r, FormatContext& ctx) const {
    if constexpr (std::ranges::contiguous_range<str_r> && std::ranges::sized_range<str_r>) {
      return underlying_.format(std::basic_string_view<charT>(std::ranges::data(r), std::ranges::size(r)), ctx);
    } else {
      const std::basic_string<charT> s(std::from_range, r);
      return underlying_.format(std::basic_string_view<charT>(s), ctx);
    }
  }
};

// [format.tuple]
template <class charT, class Tuple, class... Ts>
class fmt_tuple_formatter {
  std::tuple<std::formatter<std::remove_cvref_t<Ts>, charT>...> underlying_;
  std::basic_string_view<charT> separator_ = ycxx::detail::fmt_lit<charT>(", ", L", ");
  std::basic_string_view<charT> opening_ = ycxx::detail::fmt_lit<charT>("(", L"(");
  std::basic_string_view<charT> closing_ = ycxx::detail::fmt_lit<charT>(")", L")");
  ycxx::detail::fmt_spec<charT> spec_;

  using elems_t = std::conditional_t<(std::formattable<const Ts, charT> && ...), const Tuple, Tuple>;

  template <class FormatContext>
  constexpr void write(elems_t& elems, FormatContext& ctx) const {
    ctx.advance_to(ycxx::detail::fmt_put<charT>(ctx.out(), opening_.data(), opening_.size()));
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      ((static_cast<void>(I != 0 ? (ctx.advance_to(ycxx::detail::fmt_put<charT>(ctx.out(), separator_.data(),
                                                                                   separator_.size())),
                                    0)
                                 : 0),
        ctx.advance_to(std::get<I>(underlying_).format(std::get<I>(elems), ctx))),
       ...);
    }(std::index_sequence_for<Ts...>());
    ctx.advance_to(ycxx::detail::fmt_put<charT>(ctx.out(), closing_.data(), closing_.size()));
  }

public:
  constexpr void set_separator(std::basic_string_view<charT> sep) noexcept { separator_ = sep; }
  constexpr void set_brackets(std::basic_string_view<charT> opening, std::basic_string_view<charT> closing) noexcept {
    opening_ = opening;
    closing_ = closing;
  }

  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& ctx) {
    auto p = ctx.begin();
    const auto e = ctx.end();
    p = ycxx::detail::fmt_parse_fill_align(p, e, spec_, false);
    p = ycxx::detail::fmt_parse_width(ctx, p, e, spec_);
    if (p != e && *p == charT('m')) {
      if constexpr (sizeof...(Ts) != 2)
        ycxx::detail::throw_format_error("std::formatter: the m tuple-type needs exactly two elements");
      set_separator(ycxx::detail::fmt_lit<charT>(": ", L": "));
      set_brackets({}, {});
      ++p;
    } else if (p != e && *p == charT('n')) {
      set_brackets({}, {});
      ++p;
    }
    if (p != e && *p != charT('}'))
      ycxx::detail::throw_format_error("std::formatter: invalid tuple-format-spec");
    ctx.advance_to(p);
    std::apply(
        [&ctx, p](auto&... f) {
          ((ctx.advance_to(p), static_cast<void>(f.parse(ctx))), ...);
          (ycxx::detail::fmt_set_debug(f), ...);
        },
        underlying_);
    ctx.advance_to(p);
    return p;
  }

  template <class FormatContext>
  constexpr typename FormatContext::iterator format(elems_t& elems, FormatContext& ctx) const {
    const std::size_t width = ycxx::detail::fmt_width(spec_, ctx);
    if (width == 0) {
      write(elems, ctx);
      return ctx.out();
    }
    ycxx::detail::fmt_dynbuf<charT> tmp;
    auto tctx = ycxx::detail::fmt_access::context_like(ctx, tmp);
    write(elems, tctx);
    return ycxx::detail::fmt_write_padded<charT>(ctx.out(), spec_, ycxx::detail::fmt_align::left, width,
                                                 ycxx::detail::uni::width(tmp.data(), tmp.size()), tmp.data(),
                                                 tmp.size());
  }
};

// [container.adaptors.format]
template <class charT, class Adaptor, class Container>
class fmt_adaptor_formatter {
  using maybe_const_container = ycxx::detail::fmt_maybe_const<Container, charT>;
  using maybe_const_adaptor = std::conditional_t<std::is_const_v<maybe_const_container>, const Adaptor, Adaptor>;
  std::formatter<std::ranges::ref_view<maybe_const_container>, charT> underlying_;

  // The protected member c, named through a derived class.
  struct access : Adaptor {
    static constexpr maybe_const_container& get(maybe_const_adaptor& a) noexcept { return a.*&access::c; }
  };

public:
  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& ctx) {
    return underlying_.parse(ctx);
  }
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(maybe_const_adaptor& r, FormatContext& ctx) const {
    const std::ranges::ref_view<maybe_const_container> v(access::get(r));
    return underlying_.format(v, ctx);
  }
};

} // namespace ycxx::adl_free

namespace ycxx::detail {
template <class T>
inline constexpr bool fmt_is_bit_ref = false;
template <class Word>
inline constexpr bool fmt_is_bit_ref<ycxx::adl_free::bit_ref<Word>> = true;
} // namespace ycxx::detail

namespace std {

// [format.range.fmtmap], [format.range.fmtset], [format.range.fmtstr]
// (format_kind<R> is only asked of cv-unqualified non-reference types: its primary template must
// not be instantiated.)
template <ranges::input_range R, class charT>
  requires same_as<R, remove_cvref_t<R>> && (format_kind<R> != range_format::disabled) &&
           formattable<ranges::range_reference_t<R>, charT>
struct formatter<R, charT> : ycxx::adl_free::fmt_range_default<format_kind<R>, R, charT> {};
template <ranges::input_range R>
  requires same_as<R, remove_cvref_t<R>> && (format_kind<R> != range_format::disabled)
inline constexpr bool enable_nonlocking_formatter_optimization<R> = false;

// [format.tuple]
template <class charT, formattable<charT>... Ts>
struct formatter<tuple<Ts...>, charT> : ycxx::adl_free::fmt_tuple_formatter<charT, tuple<Ts...>, Ts...> {};
template <class charT, formattable<charT> T1, formattable<charT> T2>
struct formatter<pair<T1, T2>, charT> : ycxx::adl_free::fmt_tuple_formatter<charT, pair<T1, T2>, T1, T2> {};
template <class... Ts>
inline constexpr bool enable_nonlocking_formatter_optimization<tuple<Ts...>> =
    (enable_nonlocking_formatter_optimization<remove_cvref_t<Ts>> && ...);
template <class T1, class T2>
inline constexpr bool enable_nonlocking_formatter_optimization<pair<T1, T2>> =
    enable_nonlocking_formatter_optimization<remove_cvref_t<T1>> &&
    enable_nonlocking_formatter_optimization<remove_cvref_t<T2>>;

// [container.adaptors.format]
template <class charT, class T, formattable<charT> Container>
struct formatter<stack<T, Container>, charT>
    : ycxx::adl_free::fmt_adaptor_formatter<charT, stack<T, Container>, Container> {};
template <class charT, class T, formattable<charT> Container>
struct formatter<queue<T, Container>, charT>
    : ycxx::adl_free::fmt_adaptor_formatter<charT, queue<T, Container>, Container> {};
template <class charT, class T, formattable<charT> Container, class Compare>
struct formatter<priority_queue<T, Container, Compare>, charT>
    : ycxx::adl_free::fmt_adaptor_formatter<charT, priority_queue<T, Container, Compare>, Container> {};
template <class T, class Container>
inline constexpr bool enable_nonlocking_formatter_optimization<stack<T, Container>> = false;
template <class T, class Container>
inline constexpr bool enable_nonlocking_formatter_optimization<queue<T, Container>> = false;
template <class T, class Container, class Compare>
inline constexpr bool enable_nonlocking_formatter_optimization<priority_queue<T, Container, Compare>> = false;

// [vector.bool.fmt]
template <class T, class charT>
  requires ycxx::detail::fmt_is_bit_ref<T>
struct formatter<T, charT> {
private:
  formatter<bool, charT> underlying_;

public:
  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& ctx) {
    return underlying_.parse(ctx);
  }
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(const T& ref, FormatContext& ctx) const {
    return underlying_.format(ref, ctx);
  }
};
template <class Word>
inline constexpr bool enable_nonlocking_formatter_optimization<ycxx::adl_free::bit_ref<Word>> = true;

} // namespace std
