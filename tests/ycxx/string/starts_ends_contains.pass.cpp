// [string.starts.with], [string.ends.with], [string.contains]: starts_with, ends_with and
// contains for basic_string_view, charT and const charT*, equivalent to the
// basic_string_view operations on (data(), size()).
#include <string>
#include <string_view>
#include "check.hpp"

constexpr bool test() {
  const std::string s = "prefix-body-suffix";
  if (!s.starts_with(std::string_view("prefix")) || s.starts_with(std::string_view("body"))) return false;
  if (!s.starts_with('p') || s.starts_with('x')) return false;
  if (!s.starts_with("pre") || s.starts_with("prefix-body-suffix!")) return false;
  if (!s.starts_with(std::string("prefix-"))) return false;  // converts to string_view
  if (!s.ends_with(std::string_view("suffix")) || s.ends_with(std::string_view("body"))) return false;
  if (!s.ends_with('x') || s.ends_with('p')) return false;
  if (!s.ends_with("fix") || s.ends_with("!prefix-body-suffix")) return false;
  if (!s.contains(std::string_view("-body-")) || s.contains(std::string_view("Body"))) return false;
  if (!s.contains('-') || s.contains('z')) return false;
  if (!s.contains("dy-su") || s.contains("ydob")) return false;
  if (!s.starts_with("") || !s.ends_with("") || !s.contains("")) return false;
  const std::string e;
  if (e.starts_with('a') || e.ends_with('a') || e.contains('a')) return false;
  if (!e.starts_with("") || !e.contains(std::string_view())) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
