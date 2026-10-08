// libycxx core: std::ignore ([tuple.syn]), which <utility> also provides ([tuple.general]/2).
#pragma once

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free { // std::ignore's type; see DECISIONS §2
struct __ignore_type {
  constexpr const __ignore_type& operator=(const auto&) const noexcept { return *this; }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
inline constexpr __ycxx::__adl_free::__ignore_type ignore;
}} // namespace std
