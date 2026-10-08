// libycxx core: std::rel_ops ([depr.relops], Annex D), declared by <utility>.
#pragma once

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace rel_ops {

template <class _Tp>
[[deprecated("std::rel_ops is deprecated ([depr.relops]); use defaulted comparisons")]]
bool operator!=(const _Tp& __x, const _Tp& y) {
  return !static_cast<bool>(__x == y);
}
template <class _Tp>
[[deprecated("std::rel_ops is deprecated ([depr.relops]); use defaulted comparisons")]]
bool operator>(const _Tp& __x, const _Tp& y) {
  return static_cast<bool>(y < __x);
}
template <class _Tp>
[[deprecated("std::rel_ops is deprecated ([depr.relops]); use defaulted comparisons")]]
bool operator<=(const _Tp& __x, const _Tp& y) {
  return !static_cast<bool>(y < __x);
}
template <class _Tp>
[[deprecated("std::rel_ops is deprecated ([depr.relops]); use defaulted comparisons")]]
bool operator>=(const _Tp& __x, const _Tp& y) {
  return !static_cast<bool>(__x < y);
}

}}} // namespace std::rel_ops
