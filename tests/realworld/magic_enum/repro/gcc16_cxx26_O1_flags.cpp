// GCC 16.2 -O1 -std=c++26 (not -O0, not -std=c++23, not Clang 23): enum_flags_contains of a
// string naming a flag twice is false. Built with libstdc++: g++-16 -std=c++26 -O1 me2.cpp
#include <magic_enum/magic_enum_flags.hpp>
#include <cstdio>
enum class Color { RED = 1, GREEN = 2, BLUE = 4 };
template <> struct magic_enum::customize::enum_range<Color> { static constexpr bool is_flags = true; };
int main() {
  bool a = magic_enum::enum_flags_contains<Color>("GREEN|RED|RED");
  std::printf("%d\n", a);
  return a ? 0 : 1;
}
