// libycxx hosted: formatter<stacktrace_entry> and formatter<basic_stacktrace<Allocator>>
// ([stacktrace.format]): to_string of the entry or trace, copied through the context's output
// iterator; an entry's format-spec takes fill-and-align and width (strings align left by default).
// Needs only the core of <format>.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/core/format_unicode.hpp>
#include <ycxx/hosted/stacktrace.hpp>

namespace [[gnu::visibility("hidden")]] std {

template <>
struct formatter<stacktrace_entry> {
private:
  ycxx::detail::fmt_spec<char> spec_;

public:
  constexpr format_parse_context::iterator parse(format_parse_context& ctx) {
    auto p = ycxx::detail::fmt_parse_fill_align(ctx.begin(), ctx.end(), spec_);
    p = ycxx::detail::fmt_parse_width(ctx, p, ctx.end(), spec_);
    if (p != ctx.end() && *p != '}')
      ycxx::detail::throw_format_error("std::formatter<std::stacktrace_entry>: invalid stacktrace-entry-format-spec");
    return p;
  }

  template <class FormatContext>
  typename FormatContext::iterator format(const stacktrace_entry& e, FormatContext& ctx) const {
    const string s = std::to_string(e);
    return ycxx::detail::fmt_write_padded<char>(ctx.out(), spec_, ycxx::detail::fmt_align::left,
                                                ycxx::detail::fmt_width(spec_, ctx),
                                                ycxx::detail::uni::width(s.data(), s.size()), s.data(), s.size());
  }
};

template <class Allocator>
struct formatter<basic_stacktrace<Allocator>> {
  constexpr format_parse_context::iterator parse(format_parse_context& ctx) {
    auto p = ctx.begin();
    if (p != ctx.end() && *p != '}')
      ycxx::detail::throw_format_error("std::formatter<std::basic_stacktrace>: the format-spec must be empty");
    return p;
  }

  template <class FormatContext>
  typename FormatContext::iterator format(const basic_stacktrace<Allocator>& st, FormatContext& ctx) const {
    const string s = std::to_string(st);
    return ycxx::detail::fmt_put<char>(ctx.out(), s.data(), s.size());
  }
};

// [format.formatter.spec]/3 (not specified otherwise).
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<stacktrace_entry> = true;
template <class Allocator>
inline constexpr bool enable_nonlocking_formatter_optimization<basic_stacktrace<Allocator>> = true;

} // namespace std
