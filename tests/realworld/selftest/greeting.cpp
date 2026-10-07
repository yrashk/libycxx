#include <stdexcept>
#include <string>

[[gnu::visibility("default")]] std::string greeting(const std::string& who) {
  if (who.empty())
    throw std::invalid_argument("nobody to greet");
  return "hello, " + who;
}
