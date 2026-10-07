#include <greet.hpp>
#include <cstdio>
int main() {
  if (greet({"ok"}) != "integration: ok") return 1;
  std::printf("integration: ok\n");
}
