// libycxx core: the std::iterator base class template ([depr.iterator], Annex D), declared by
// <iterator>.
#pragma once

#include <ycxx/core/cstddef.hpp>

namespace std {

template <class Category, class T, class Distance = ptrdiff_t, class Pointer = T*, class Reference = T&>
struct [[deprecated("std::iterator is deprecated ([depr.iterator]); declare the member types directly")]] iterator {
  using iterator_category = Category;
  using value_type = T;
  using difference_type = Distance;
  using pointer = Pointer;
  using reference = Reference;
};

} // namespace std
