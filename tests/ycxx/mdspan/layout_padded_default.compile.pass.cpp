// [mdspan.syn]: template<size_t PaddingValue = dynamic_extent> struct layout_left_padded; and
// likewise layout_right_padded: the padding value defaults to dynamic_extent.
#include <mdspan>
#include <type_traits>

static_assert(std::is_same_v<std::layout_left_padded<>, std::layout_left_padded<std::dynamic_extent>>);
static_assert(std::is_same_v<std::layout_right_padded<>, std::layout_right_padded<std::dynamic_extent>>);
static_assert(std::layout_right_padded<>::mapping<std::dextents<int, 2>>::padding_value == std::dynamic_extent);
