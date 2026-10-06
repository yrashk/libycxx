// libycxx core: formatting of ranges ([format.range]) and tuples ([format.tuple]). The formatters
// of the container adaptors and of vector<bool>::reference are declared by their containers'
// headers (format_adaptors.hpp, format_vector_bool.hpp).
#pragma once

#include <ycxx/core/format_adaptors.hpp>
#include <ycxx/core/format_base.hpp>
#include <ycxx/core/format_kind.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/ranges_all.hpp>
#include <ycxx/core/tuple.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Fp>
constexpr void __fmt_set_debug(_Fp& __f) {
  if constexpr (requires { __f.set_debug_format(); })
    __f.set_debug_format();
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [format.range.formatter]
template <class _Tp, class __charT = char>
  requires same_as<remove_cvref_t<_Tp>, _Tp> && formattable<_Tp, __charT>
class range_formatter {
  enum class kind : unsigned char { sequence, string, debug_string };

  formatter<_Tp, __charT> __underlying_;
  basic_string_view<__charT> __separator_ = __ycxx::__detail::__fmt_lit<__charT>(", ", L", ");
  basic_string_view<__charT> __opening_ = __ycxx::__detail::__fmt_lit<__charT>("[", L"[");
  basic_string_view<__charT> __closing_ = __ycxx::__detail::__fmt_lit<__charT>("]", L"]");
  __ycxx::__detail::__fmt_spec<__charT> __spec_; // range-fill-and-align and width
  kind __kind_ = kind::sequence;

  template <class _Rp, class _FormatContext>
  constexpr void __write_elements(_Rp& r, _FormatContext& __ctx) const {
    __ctx.advance_to(__ycxx::__detail::__fmt_put<__charT>(__ctx.out(), __opening_.data(), __opening_.size()));
    bool first = true;
    auto __it = ranges::begin(r);
    const auto last = ranges::end(r);
    for (; __it != last; ++__it) {
      if (!first)
        __ctx.advance_to(__ycxx::__detail::__fmt_put<__charT>(__ctx.out(), __separator_.data(), __separator_.size()));
      first = false;
      __ctx.advance_to(__underlying_.format(*__it, __ctx));
    }
    __ctx.advance_to(__ycxx::__detail::__fmt_put<__charT>(__ctx.out(), __closing_.data(), __closing_.size()));
  }

public:
  constexpr void set_separator(basic_string_view<__charT> __sep) noexcept { __separator_ = __sep; }
  constexpr void set_brackets(basic_string_view<__charT> __opening, basic_string_view<__charT> __closing) noexcept {
    __opening_ = __opening;
    __closing_ = __closing;
  }
  constexpr formatter<_Tp, __charT>& underlying() noexcept { return __underlying_; }
  constexpr const formatter<_Tp, __charT>& underlying() const noexcept { return __underlying_; }

  template <class _ParseContext>
  constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    auto p = __ctx.begin();
    const auto e = __ctx.end();
    p = __ycxx::__detail::__fmt_parse_fill_align(p, e, __spec_, false);
    p = __ycxx::__detail::__fmt_parse_width(__ctx, p, e, __spec_);
    bool __no_brackets = false;
    if (p != e && *p == __charT('n')) {
      __no_brackets = true;
      ++p;
    }
    bool map = false;
    if (p != e && *p == __charT('m')) {
      if constexpr (!__ycxx::__detail::__fmt_is_pair_or_2tuple<_Tp>)
        __ycxx::__detail::__throw_format_error("std::range_formatter: m needs elements that are pairs or 2-tuples");
      map = true;
      ++p;
    } else if (p != e && (*p == __charT('s') || *p == __charT('?'))) {
      if (*p == __charT('?')) {
        ++p;
        if (p == e || *p != __charT('s'))
          __ycxx::__detail::__throw_format_error("std::range_formatter: ? must be followed by s");
        __kind_ = kind::debug_string;
      } else {
        __kind_ = kind::string;
      }
      ++p;
      if constexpr (!same_as<_Tp, __charT>)
        __ycxx::__detail::__throw_format_error("std::range_formatter: s and ?s need a range of the character type");
      if (__no_brackets)
        __ycxx::__detail::__throw_format_error("std::range_formatter: n cannot be combined with s or ?s");
    }
    const bool __has_underlying = p != e && *p == __charT(':');
    if (__has_underlying) {
      if (__kind_ != kind::sequence)
        __ycxx::__detail::__throw_format_error("std::range_formatter: s and ?s take no underlying format-spec");
      ++p;
    } else if (p != e && *p != __charT('}')) {
      __ycxx::__detail::__throw_format_error("std::range_formatter: invalid range-format-spec");
    }
    if (map) {
      if constexpr (__ycxx::__detail::__fmt_is_pair_or_2tuple<_Tp>) {
        set_brackets(__ycxx::__detail::__fmt_lit<__charT>("{", L"{"), __ycxx::__detail::__fmt_lit<__charT>("}", L"}"));
        set_separator(__ycxx::__detail::__fmt_lit<__charT>(", ", L", "));
        __underlying_.set_brackets({}, {});
        __underlying_.set_separator(__ycxx::__detail::__fmt_lit<__charT>(": ", L": "));
      }
    }
    if (__no_brackets)
      set_brackets({}, {});
    __ctx.advance_to(p);
    p = __underlying_.parse(__ctx);
    if constexpr (requires { __underlying_.set_debug_format(); }) {
      if (__kind_ == kind::sequence && !__has_underlying)
        __underlying_.set_debug_format();
    }
    return p;
  }

  template <ranges::input_range _Rp, class _FormatContext>
    requires formattable<ranges::range_reference_t<_Rp>, __charT> && same_as<remove_cvref_t<ranges::range_reference_t<_Rp>>, _Tp>
  constexpr typename _FormatContext::iterator format(_Rp&& r, _FormatContext& __ctx) const {
    if constexpr (same_as<_Tp, __charT>) {
      if (__kind_ != kind::sequence) {
        __ycxx::__detail::__fmt_spec<__charT> s = __spec_;
        s.type = __kind_ == kind::debug_string ? '?' : 0;
        if constexpr (ranges::contiguous_range<_Rp> && ranges::sized_range<_Rp>) {
          return __ycxx::__detail::__fmt_write_string(__ctx, ranges::data(r), static_cast<size_t>(ranges::size(r)), s);
        } else {
          const basic_string<__charT> str(from_range, r);
          return __ycxx::__detail::__fmt_write_string(__ctx, str.data(), str.size(), s);
        }
      }
    }
    const size_t width = __ycxx::__detail::__fmt_width(__spec_, __ctx);
    if (width == 0) {
      __write_elements(r, __ctx);
      return __ctx.out();
    }
    __ycxx::__detail::__fmt_dynbuf<__charT> __tmp;
    auto __tctx = __ycxx::__detail::__fmt_access::__context_like(__ctx, __tmp);
    __write_elements(r, __tctx);
    return __ycxx::__detail::__fmt_write_padded<__charT>(__ctx.out(), __spec_, __ycxx::__detail::__fmt_align::left, width,
                                                 __ycxx::__detail::__uni::width(__tmp.data(), __tmp.size()), __tmp.data(),
                                                 __tmp.size());
  }
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// range-default-formatter ([format.range.fmtdef], [format.range.fmtmap], [format.range.fmtset],
// [format.range.fmtstr]).
template <std::range_format _Kp, std::ranges::input_range _Rp, class __charT>
struct __fmt_range_default;

template <std::ranges::input_range _Rp, class __charT>
struct __fmt_range_default<std::range_format::sequence, _Rp, __charT> {
private:
  using __maybe_const_r = __ycxx::__detail::__fmt_maybe_const<_Rp, __charT>;
  std::range_formatter<std::remove_cvref_t<std::ranges::range_reference_t<__maybe_const_r>>, __charT> __underlying_;

public:
  constexpr void set_separator(std::basic_string_view<__charT> __sep) noexcept { __underlying_.set_separator(__sep); }
  constexpr void set_brackets(std::basic_string_view<__charT> __opening, std::basic_string_view<__charT> __closing) noexcept {
    __underlying_.set_brackets(__opening, __closing);
  }
  template <class _ParseContext>
  constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    return __underlying_.parse(__ctx);
  }
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(__maybe_const_r& __y_elems, _FormatContext& __ctx) const {
    return __underlying_.format(__y_elems, __ctx);
  }
};

template <std::ranges::input_range _Rp, class __charT>
struct __fmt_range_default<std::range_format::map, _Rp, __charT> {
private:
  using __maybe_const_map = __ycxx::__detail::__fmt_maybe_const<_Rp, __charT>;
  using element_type = std::remove_cvref_t<std::ranges::range_reference_t<__maybe_const_map>>;
  std::range_formatter<element_type, __charT> __underlying_;

public:
  constexpr __fmt_range_default() {
    static_assert(__ycxx::__detail::__fmt_is_pair_or_2tuple<element_type>,
                  "std::formatter: a map's elements must be pairs or 2-tuples ([format.range.fmtmap]/1)");
    __underlying_.set_brackets(__ycxx::__detail::__fmt_lit<__charT>("{", L"{"), __ycxx::__detail::__fmt_lit<__charT>("}", L"}"));
    __underlying_.underlying().set_brackets({}, {});
    __underlying_.underlying().set_separator(__ycxx::__detail::__fmt_lit<__charT>(": ", L": "));
  }
  template <class _ParseContext>
  constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    return __underlying_.parse(__ctx);
  }
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(__maybe_const_map& r, _FormatContext& __ctx) const {
    return __underlying_.format(r, __ctx);
  }
};

template <std::ranges::input_range _Rp, class __charT>
struct __fmt_range_default<std::range_format::set, _Rp, __charT> {
private:
  using __maybe_const_set = __ycxx::__detail::__fmt_maybe_const<_Rp, __charT>;
  std::range_formatter<std::remove_cvref_t<std::ranges::range_reference_t<__maybe_const_set>>, __charT> __underlying_;

public:
  constexpr __fmt_range_default() {
    __underlying_.set_brackets(__ycxx::__detail::__fmt_lit<__charT>("{", L"{"), __ycxx::__detail::__fmt_lit<__charT>("}", L"}"));
  }
  template <class _ParseContext>
  constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    return __underlying_.parse(__ctx);
  }
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(__maybe_const_set& r, _FormatContext& __ctx) const {
    return __underlying_.format(r, __ctx);
  }
};

// string and debug_string: formatted as basic_string<charT> (a view of the elements when the
// range is contiguous, which formats the same).
template <std::range_format _Kp, std::ranges::input_range _Rp, class __charT>
  requires(_Kp == std::range_format::string || _Kp == std::range_format::debug_string)
struct __fmt_range_default<_Kp, _Rp, __charT> {
private:
  static_assert(__is_same(std::remove_cvref_t<std::ranges::range_reference_t<_Rp>>, __charT),
                "std::formatter: a range formatted as a string must have elements of the character type "
                "([format.range.fmtstr]/1)");
  std::formatter<std::basic_string_view<__charT>, __charT> __underlying_;
  using __str_r = std::conditional_t<std::ranges::input_range<const _Rp>, const _Rp, _Rp>;

public:
  template <class _ParseContext>
  constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    auto i = __underlying_.parse(__ctx);
    if constexpr (_Kp == std::range_format::debug_string)
      __underlying_.set_debug_format();
    return i;
  }
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(__str_r& r, _FormatContext& __ctx) const {
    if constexpr (std::ranges::contiguous_range<__str_r> && std::ranges::sized_range<__str_r>) {
      return __underlying_.format(std::basic_string_view<__charT>(std::ranges::data(r), std::ranges::size(r)), __ctx);
    } else {
      const std::basic_string<__charT> s(std::from_range, r);
      return __underlying_.format(std::basic_string_view<__charT>(s), __ctx);
    }
  }
};

// [format.tuple]
template <class __charT, class _Tuple, class... _Ts>
class __fmt_tuple_formatter {
  std::tuple<std::formatter<std::remove_cvref_t<_Ts>, __charT>...> __underlying_;
  std::basic_string_view<__charT> __separator_ = __ycxx::__detail::__fmt_lit<__charT>(", ", L", ");
  std::basic_string_view<__charT> __opening_ = __ycxx::__detail::__fmt_lit<__charT>("(", L"(");
  std::basic_string_view<__charT> __closing_ = __ycxx::__detail::__fmt_lit<__charT>(")", L")");
  __ycxx::__detail::__fmt_spec<__charT> __spec_;

  using __elems_t = std::conditional_t<(std::formattable<const _Ts, __charT> && ...), const _Tuple, _Tuple>;

  template <class _FormatContext>
  constexpr void write(__elems_t& __y_elems, _FormatContext& __ctx) const {
    __ctx.advance_to(__ycxx::__detail::__fmt_put<__charT>(__ctx.out(), __opening_.data(), __opening_.size()));
    [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) {
      ((static_cast<void>(_Ip != 0 ? (__ctx.advance_to(__ycxx::__detail::__fmt_put<__charT>(__ctx.out(), __separator_.data(),
                                                                                   __separator_.size())),
                                    0)
                                 : 0),
        __ctx.advance_to(std::get<_Ip>(__underlying_).format(std::get<_Ip>(__y_elems), __ctx))),
       ...);
    }(std::index_sequence_for<_Ts...>());
    __ctx.advance_to(__ycxx::__detail::__fmt_put<__charT>(__ctx.out(), __closing_.data(), __closing_.size()));
  }

public:
  constexpr void set_separator(std::basic_string_view<__charT> __sep) noexcept { __separator_ = __sep; }
  constexpr void set_brackets(std::basic_string_view<__charT> __opening, std::basic_string_view<__charT> __closing) noexcept {
    __opening_ = __opening;
    __closing_ = __closing;
  }

  template <class _ParseContext>
  constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    auto p = __ctx.begin();
    const auto e = __ctx.end();
    p = __ycxx::__detail::__fmt_parse_fill_align(p, e, __spec_, false);
    p = __ycxx::__detail::__fmt_parse_width(__ctx, p, e, __spec_);
    if (p != e && *p == __charT('m')) {
      if constexpr (sizeof...(_Ts) != 2)
        __ycxx::__detail::__throw_format_error("std::formatter: the m tuple-type needs exactly two elements");
      set_separator(__ycxx::__detail::__fmt_lit<__charT>(": ", L": "));
      set_brackets({}, {});
      ++p;
    } else if (p != e && *p == __charT('n')) {
      set_brackets({}, {});
      ++p;
    }
    if (p != e && *p != __charT('}'))
      __ycxx::__detail::__throw_format_error("std::formatter: invalid tuple-format-spec");
    __ctx.advance_to(p);
    std::apply(
        [&__ctx, p](auto&... __f) {
          ((__ctx.advance_to(p), static_cast<void>(__f.parse(__ctx))), ...);
          (__ycxx::__detail::__fmt_set_debug(__f), ...);
        },
        __underlying_);
    __ctx.advance_to(p);
    return p;
  }

  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(__elems_t& __y_elems, _FormatContext& __ctx) const {
    const std::size_t width = __ycxx::__detail::__fmt_width(__spec_, __ctx);
    if (width == 0) {
      write(__y_elems, __ctx);
      return __ctx.out();
    }
    __ycxx::__detail::__fmt_dynbuf<__charT> __tmp;
    auto __tctx = __ycxx::__detail::__fmt_access::__context_like(__ctx, __tmp);
    write(__y_elems, __tctx);
    return __ycxx::__detail::__fmt_write_padded<__charT>(__ctx.out(), __spec_, __ycxx::__detail::__fmt_align::left, width,
                                                 __ycxx::__detail::__uni::width(__tmp.data(), __tmp.size()), __tmp.data(),
                                                 __tmp.size());
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

// [format.range.fmtmap], [format.range.fmtset], [format.range.fmtstr]
// (format_kind<R> is only asked of cv-unqualified non-reference types: its primary template must
// not be instantiated.)
template <ranges::input_range _Rp, class __charT>
  requires same_as<_Rp, remove_cvref_t<_Rp>> && (format_kind<_Rp> != range_format::disabled) &&
           formattable<ranges::range_reference_t<_Rp>, __charT>
struct formatter<_Rp, __charT> : __ycxx::__adl_free::__fmt_range_default<format_kind<_Rp>, _Rp, __charT> {};
template <ranges::input_range _Rp>
  requires same_as<_Rp, remove_cvref_t<_Rp>> && (format_kind<_Rp> != range_format::disabled)
inline constexpr bool enable_nonlocking_formatter_optimization<_Rp> = false;

// [format.tuple]
template <class __charT, formattable<__charT>... _Ts>
struct formatter<tuple<_Ts...>, __charT> : __ycxx::__adl_free::__fmt_tuple_formatter<__charT, tuple<_Ts...>, _Ts...> {};
template <class __charT, formattable<__charT> _T1, formattable<__charT> _T2>
struct formatter<pair<_T1, _T2>, __charT> : __ycxx::__adl_free::__fmt_tuple_formatter<__charT, pair<_T1, _T2>, _T1, _T2> {};
template <class... _Ts>
inline constexpr bool enable_nonlocking_formatter_optimization<tuple<_Ts...>> =
    (enable_nonlocking_formatter_optimization<remove_cvref_t<_Ts>> && ...);
template <class _T1, class _T2>
inline constexpr bool enable_nonlocking_formatter_optimization<pair<_T1, _T2>> =
    enable_nonlocking_formatter_optimization<remove_cvref_t<_T1>> &&
    enable_nonlocking_formatter_optimization<remove_cvref_t<_T2>>;

} // namespace std
