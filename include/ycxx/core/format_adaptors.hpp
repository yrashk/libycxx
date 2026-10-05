// libycxx core: the formatters of the container adaptors ([container.adaptors.format]), which
// <stack> and <queue> declare ([stack.syn], [queue.syn]). They are defined against declarations
// of the adaptors and of ranges::ref_view; the underlying formatter of ref_view<Container> is
// the range formatter of <format>, as is every formatter of a standard container, so a
// specialization is enabled (formattable<Container, charT>) only once <format> is included.
#pragma once

#include <ycxx/core/format_decl.hpp>
#include <ycxx/core/range_access.hpp>

namespace [[gnu::visibility("hidden")]] std {
template <class T, class Container>
class stack;
template <class T, class Container>
class queue;
template <class T, class Container, class Compare>
class priority_queue;
namespace ranges {
template <range R>
  requires is_object_v<R>
class ref_view;
} // namespace ranges
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class R, class charT>
concept fmt_const_formattable_range =
    std::ranges::input_range<const R> && std::formattable<std::ranges::range_reference_t<const R>, charT>;
template <class R, class charT>
using fmt_maybe_const = std::conditional_t<fmt_const_formattable_range<R, charT>, const R, R>;
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

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

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

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

} // namespace std
