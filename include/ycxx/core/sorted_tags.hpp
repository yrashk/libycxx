// libycxx core: sorted_unique_t and sorted_equivalent_t ([flat.map.syn], [flat.set.syn]),
// shared by <flat_map> and <flat_set>. <map> and <set> include it too, so code written for
// both kinds of associative containers finds the tags with either header.
#pragma once

namespace std {

struct sorted_unique_t {
  explicit sorted_unique_t() = default;
};
inline constexpr sorted_unique_t sorted_unique{};

struct sorted_equivalent_t {
  explicit sorted_equivalent_t() = default;
};
inline constexpr sorted_equivalent_t sorted_equivalent{};

} // namespace std
