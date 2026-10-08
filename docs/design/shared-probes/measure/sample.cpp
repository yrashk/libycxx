// A representative translation unit for the cost measurements (measure.sh): containers, strings,
// streams, formatting, algorithms, smart pointers and exceptions, instantiated for program types.
#include <algorithm>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <variant>
#include <functional>
struct record { std::string name; std::vector<int> values; std::optional<double> weight; };
struct widget : std::runtime_error { using std::runtime_error::runtime_error; std::map<std::string, record> parts; };
std::unordered_map<std::string, std::shared_ptr<record>> index_of(const std::vector<record>& rs) {
  std::unordered_map<std::string, std::shared_ptr<record>> m;
  for (const auto& r : rs) m.emplace(r.name, std::make_shared<record>(r));
  return m;
}
std::string describe(const record& r) {
  std::ostringstream os;
  os << r.name << ':' << r.values.size();
  return os.str() + std::format(" {:.2f}", r.weight.value_or(0.0));
}
int main(int argc, char**) {
  std::vector<record> rs;
  for (int i = 0; i < 100 + argc; ++i)
    rs.push_back({std::to_string(i), std::vector<int>(i % 7, i), i % 3 ? std::optional<double>(i) : std::nullopt});
  std::ranges::sort(rs, {}, &record::name);
  auto m = index_of(rs);
  std::variant<int, std::string> v = describe(*m["42"]);
  std::function<std::size_t()> f = [&] { return std::get<std::string>(v).size(); };
  try {
    throw widget("w");
  } catch (const std::exception& e) {
    std::cout << e.what() << ' ' << f() << '\n';
  }
  return 0;
}
