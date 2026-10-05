// libycxx core: std::ignore ([tuple.syn]), which <utility> also provides ([tuple.general]/2).
#pragma once

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free { // std::ignore's type; see DECISIONS §2
struct ignore_type {
  constexpr const ignore_type& operator=(const auto&) const noexcept { return *this; }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {
inline constexpr ycxx::adl_free::ignore_type ignore;
} // namespace std
