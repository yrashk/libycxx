#include <string>
#include <vector>

std::string joined(const std::vector<std::string>& parts) {
  std::string out;
  for (const auto& p : parts) out += (out.empty() ? "" : " ") + p;
  return out;
}
