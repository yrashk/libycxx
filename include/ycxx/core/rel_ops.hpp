// libycxx core: std::rel_ops ([depr.relops], Annex D), declared by <utility>.
#pragma once

namespace [[gnu::visibility("hidden")]] std { namespace rel_ops {

template <class T>
[[deprecated("std::rel_ops is deprecated ([depr.relops]); use defaulted comparisons")]]
bool operator!=(const T& x, const T& y) {
  return !static_cast<bool>(x == y);
}
template <class T>
[[deprecated("std::rel_ops is deprecated ([depr.relops]); use defaulted comparisons")]]
bool operator>(const T& x, const T& y) {
  return static_cast<bool>(y < x);
}
template <class T>
[[deprecated("std::rel_ops is deprecated ([depr.relops]); use defaulted comparisons")]]
bool operator<=(const T& x, const T& y) {
  return !static_cast<bool>(y < x);
}
template <class T>
[[deprecated("std::rel_ops is deprecated ([depr.relops]); use defaulted comparisons")]]
bool operator>=(const T& x, const T& y) {
  return !static_cast<bool>(x < y);
}

}} // namespace std::rel_ops
