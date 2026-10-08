// libycxx core: the std::iterator base class template ([depr.iterator], Annex D), declared by
// <iterator>.
#pragma once

#include <ycxx/core/cstddef.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Category, class _Tp, class _Distance = ptrdiff_t, class _Pointer = _Tp*, class _Reference = _Tp&>
struct [[deprecated("std::iterator is deprecated ([depr.iterator]); declare the member types directly")]] iterator {
  using iterator_category = _Category;
  using value_type = _Tp;
  using difference_type = _Distance;
  using pointer = _Pointer;
  using reference = _Reference;
};

}} // namespace std
