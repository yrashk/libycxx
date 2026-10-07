#include <cstdio>
#include <format>
#include <optional>
#include <string>

extern "C" int c_twice(int);

int main() {
  std::optional<std::string> s = std::format("autotools {}", "ok");
  if (*s != "autotools ok" || c_twice(1) != 2) return 1;
  std::printf("%s\n", s->c_str());
}
