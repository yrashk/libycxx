#include <iostream>
#include <stdexcept>
#include <string>

std::string greeting(const std::string& who);

int main() {
  std::cout << greeting("world") << '\n';
  try {
    greeting("");
  } catch (const std::invalid_argument& e) {
    std::cout << "caught: " << e.what() << '\n';
    return 0;
  }
  return 1;
}
