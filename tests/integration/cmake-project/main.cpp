// Prints "integration: ok" when everything works: a string and a vector built here and used in
// the shared library, a string built there and destroyed here, an exception thrown there and
// caught here, C code, and the dependencies built by FetchContent and ExternalProject.
#include "greet.hpp"
#include "sub.hpp"
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string_view>

int main() {
  std::optional<std::string> s = greet({"ok"});
  if (*s != std::string("integration: ok") + sub_suffix()) return 1;
  try {
    greet_throw(7);
    return 2;
  } catch (const std::runtime_error& e) {
    if (std::string_view(e.what()) != "code 7") return 3;
  }
  if (greet_c_part(21) != 42 || sub_answer() != 42) return 4;
  std::printf("integration: ok\n");
}
