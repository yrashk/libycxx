// libycxx core: the formatters of the container adaptors ([container.adaptors.format]), which
// <stack> and <queue> declare ([stack.syn], [queue.syn]). They are defined against declarations
// of the adaptors and of ranges::ref_view; the underlying formatter of ref_view<Container> is
// the range formatter of <format>, as is every formatter of a standard container, so a
// specialization is enabled (formattable<Container, charT>) only once <format> is included.
#pragma once

#include <ycxx/core/format_decl.hpp>
#include <ycxx/core/range_access.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _Tp, class _Container>
class stack;
template <class _Tp, class _Container>
class queue;
template <class _Tp, class _Container, class _Compare>
class priority_queue;
namespace ranges {
template <range _Rp>
  requires is_object_v<_Rp>
class ref_view;
} // namespace ranges
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Rp, class __charT>
concept __fmt_const_formattable_range =
    std::ranges::input_range<const _Rp> && std::formattable<std::ranges::range_reference_t<const _Rp>, __charT>;
template <class _Rp, class __charT>
using __fmt_maybe_const = std::conditional_t<__fmt_const_formattable_range<_Rp, __charT>, const _Rp, _Rp>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

template <class __charT, class _Adaptor, class _Container>
class __fmt_adaptor_formatter {
  using __maybe_const_container = __ycxx::__detail::__fmt_maybe_const<_Container, __charT>;
  using __maybe_const_adaptor = std::conditional_t<std::is_const_v<__maybe_const_container>, const _Adaptor, _Adaptor>;
  std::formatter<std::ranges::ref_view<__maybe_const_container>, __charT> __underlying_;

  // The protected member c, named through a derived class.
  struct access : _Adaptor {
    static constexpr __maybe_const_container& get(__maybe_const_adaptor& a) noexcept { return a.*&access::c; }
  };

public:
  template <class _ParseContext>
  constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    return __underlying_.parse(__ctx);
  }
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(__maybe_const_adaptor& r, _FormatContext& __ctx) const {
    const std::ranges::ref_view<__maybe_const_container> __v(access::get(r));
    return __underlying_.format(__v, __ctx);
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class __charT, class _Tp, formattable<__charT> _Container>
struct formatter<stack<_Tp, _Container>, __charT>
    : __ycxx::__adl_free::__fmt_adaptor_formatter<__charT, stack<_Tp, _Container>, _Container> {};
template <class __charT, class _Tp, formattable<__charT> _Container>
struct formatter<queue<_Tp, _Container>, __charT>
    : __ycxx::__adl_free::__fmt_adaptor_formatter<__charT, queue<_Tp, _Container>, _Container> {};
template <class __charT, class _Tp, formattable<__charT> _Container, class _Compare>
struct formatter<priority_queue<_Tp, _Container, _Compare>, __charT>
    : __ycxx::__adl_free::__fmt_adaptor_formatter<__charT, priority_queue<_Tp, _Container, _Compare>, _Container> {};
template <class _Tp, class _Container>
inline constexpr bool enable_nonlocking_formatter_optimization<stack<_Tp, _Container>> = false;
template <class _Tp, class _Container>
inline constexpr bool enable_nonlocking_formatter_optimization<queue<_Tp, _Container>> = false;
template <class _Tp, class _Container, class _Compare>
inline constexpr bool enable_nonlocking_formatter_optimization<priority_queue<_Tp, _Container, _Compare>> = false;

}} // namespace std
