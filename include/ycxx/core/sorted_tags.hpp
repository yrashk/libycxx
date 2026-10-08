// libycxx core: sorted_unique_t and sorted_equivalent_t ([flat.map.syn], [flat.set.syn]),
// shared by <flat_map> and <flat_set>.
#pragma once

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

struct sorted_unique_t {
  explicit sorted_unique_t() = default;
};
inline constexpr sorted_unique_t sorted_unique{};

struct sorted_equivalent_t {
  explicit sorted_equivalent_t() = default;
};
inline constexpr sorted_equivalent_t sorted_equivalent{};

}} // namespace std
