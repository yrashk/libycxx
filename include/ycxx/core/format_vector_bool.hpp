// libycxx core: formatter<vector<bool>::reference> ([vector.bool.fmt]), which <vector> declares
// ([vector.syn]). Its parse and format forward to formatter<bool> (format_decl.hpp), whose
// member bodies need the formatting library only when they are used.
#pragma once

#include <ycxx/core/format_decl.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {
template <class _Word>
class __bit_ref;
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
inline constexpr bool __fmt_is_bit_ref = false;
template <class _Word>
inline constexpr bool __fmt_is_bit_ref<__ycxx::__adl_free::__bit_ref<_Word>> = true;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp, class __charT>
  requires __ycxx::__detail::__fmt_is_bit_ref<_Tp>
struct formatter<_Tp, __charT> {
private:
  formatter<bool, __charT> __underlying_;

public:
  template <class _ParseContext>
  constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    return __underlying_.parse(__ctx);
  }
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(const _Tp& ref, _FormatContext& __ctx) const {
    return __underlying_.format(ref, __ctx);
  }
};
// [format.formatter.spec]/3: not specified otherwise.
template <class _Word>
inline constexpr bool enable_nonlocking_formatter_optimization<__ycxx::__adl_free::__bit_ref<_Word>> = true;

}} // namespace std
