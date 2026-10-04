// [string.view.comparison]/3: operator<=> "Mandates: R denotes a comparison category type."
// Here traits::comparison_category is int, which is not a comparison category type.
#include <string_view>
#include <cstddef>

struct BadTraits {
  using char_type = char;
  using int_type = int;
  using comparison_category = int;
  static constexpr bool eq(char a, char b) noexcept { return a == b; }
  static constexpr bool lt(char a, char b) noexcept { return a < b; }
  static constexpr int compare(const char*, const char*, std::size_t) { return 0; }
  static constexpr std::size_t length(const char*) { return 0; }
  static constexpr const char* find(const char*, std::size_t, const char&) { return nullptr; }
};

using V = std::basic_string_view<char, BadTraits>;
auto r = V() <=> V();
