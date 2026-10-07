#include "greet.hpp"
#include "sub.hpp"
#include <stdexcept>

std::string greet(const std::vector<std::string>& names) {
  std::string out = "integration:";
  for (const auto& n : names) out += " " + n;
  return out + sub_suffix();
}

void greet_throw(int code) { throw std::runtime_error("code " + std::to_string(code)); }
