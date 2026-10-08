// libycxx hosted: formatter<stacktrace_entry> and formatter<basic_stacktrace<Allocator>>
// ([stacktrace.format]): to_string of the entry or trace, copied through the context's output
// iterator; an entry's format-spec takes fill-and-align and width (strings align left by default).
// Needs only the core of <format>.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/core/format_unicode.hpp>
#include <ycxx/hosted/stacktrace.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <>
struct formatter<stacktrace_entry> {
private:
  __ycxx::__detail::__fmt_spec<char> __spec_;

public:
  constexpr format_parse_context::iterator parse(format_parse_context& __ctx) {
    auto p = __ycxx::__detail::__fmt_parse_fill_align(__ctx.begin(), __ctx.end(), __spec_);
    p = __ycxx::__detail::__fmt_parse_width(__ctx, p, __ctx.end(), __spec_);
    if (p != __ctx.end() && *p != '}')
      __ycxx::__detail::__throw_format_error("std::formatter<std::stacktrace_entry>: invalid stacktrace-entry-format-spec");
    return p;
  }

  template <class _FormatContext>
  typename _FormatContext::iterator format(const stacktrace_entry& e, _FormatContext& __ctx) const {
    const string s = std::to_string(e);
    return __ycxx::__detail::__fmt_write_padded<char>(__ctx.out(), __spec_, __ycxx::__detail::__fmt_align::left,
                                                __ycxx::__detail::__fmt_width(__spec_, __ctx),
                                                __ycxx::__detail::__uni::width(s.data(), s.size()), s.data(), s.size());
  }
};

template <class _Allocator>
struct formatter<basic_stacktrace<_Allocator>> {
  constexpr format_parse_context::iterator parse(format_parse_context& __ctx) {
    auto p = __ctx.begin();
    if (p != __ctx.end() && *p != '}')
      __ycxx::__detail::__throw_format_error("std::formatter<std::basic_stacktrace>: the format-spec must be empty");
    return p;
  }

  template <class _FormatContext>
  typename _FormatContext::iterator format(const basic_stacktrace<_Allocator>& __st, _FormatContext& __ctx) const {
    const string s = std::to_string(__st);
    return __ycxx::__detail::__fmt_put<char>(__ctx.out(), s.data(), s.size());
  }
};

// [format.formatter.spec]/3 (not specified otherwise).
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<stacktrace_entry> = true;
template <class _Allocator>
inline constexpr bool enable_nonlocking_formatter_optimization<basic_stacktrace<_Allocator>> = true;

}} // namespace std
