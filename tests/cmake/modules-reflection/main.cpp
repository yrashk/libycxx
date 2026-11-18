// std::meta through `import std;` (libycxx built with YCXX_MODULES_REFLECTION=ON). Prints
// "libycxx example: ok" and returns 0 when every check holds.
import std;

struct point {
  int x;
  int y;
};

consteval std::size_t member_count(std::meta::info type) {
  return std::meta::nonstatic_data_members_of(type, std::meta::access_context::unchecked()).size();
}

int main() {
  constexpr auto members = std::define_static_array(
      std::meta::nonstatic_data_members_of(^^point, std::meta::access_context::unchecked()));
  static_assert(member_count(^^point) == 2);
  constexpr std::string_view second = std::meta::identifier_of(members[1]);
  point p{1, 2};
  if (second != "y" || p.[:members[1]:] != 2)
    return 1;
  std::println("libycxx example: ok");
  return 0;
}
