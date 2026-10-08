// The compilers build std::source_location::current() through std::source_location::__impl
// (__builtin_source_location).
#include <source_location>
#include <cstdio>
#include <cstring>
int line(std::source_location l = std::source_location::current()) { return static_cast<int>(l.line()); }
int main() {
  bool r = line() == __LINE__ && std::strstr(std::source_location::current().function_name(), "main");
  std::puts(r ? "ok" : "FAIL");
  return r ? 0 : 1;
}
