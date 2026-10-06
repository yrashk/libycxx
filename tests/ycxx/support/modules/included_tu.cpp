// The #include half of tests/ycxx/modules/mixed_translation_units.pass.cpp: the same entities,
// reached through the headers instead of `import std;`.
#include <iostream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

int included_sum(const std::vector<int>& v) {
  int s = 0;
  for (int x : v)
    s += x;
  return s;
}
std::string included_name() { return "included"; }
const void* included_cout() { return &std::cout; }
const std::type_info& included_vector_type() { return typeid(std::vector<int>); }
[[noreturn]] void included_throw() {
#if __cpp_exceptions
  throw std::length_error("from the #include side");
#else
  __builtin_trap(); // not called without exceptions
#endif
}
