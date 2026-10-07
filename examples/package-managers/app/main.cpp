#include <fmt/format.h>
#include <cstdio>
#include <string>
#include <vector>

int main() {
  std::vector<int> v{1, 2, 3};
  std::string s = fmt::format("package manager: {} ({})", "ok", v.size());
  if (s != "package manager: ok (3)") return 1;
  std::puts("package manager: ok");
}
