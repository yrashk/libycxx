// [std.modules]/5: a declaration denotes the same entity whether it was reached through an
// #include or `import std;`. One translation unit includes the headers, this one imports std:
// they pass std::vector and std::string to each other, share std::cout, agree on type_info, and
// an exception thrown on the #include side is caught here as the imported class.
// MODULES: std
// FILES: ../support/modules/included_tu.cpp
import std;
#include "module_check.hpp"

int included_sum(const std::vector<int>& v);
std::string included_name();
const void* included_cout();
const std::type_info& included_vector_type();
[[noreturn]] void included_throw();

int main() {
  CHECK(included_sum(std::vector<int>{1, 2, 3}) == 6);
  CHECK(included_name() == "included");
  CHECK(included_cout() == &std::cout);
  CHECK(included_vector_type() == typeid(std::vector<int>));
#if __cpp_exceptions
  bool caught = false;
  try {
    included_throw();
  } catch (const std::length_error& e) {
    caught = std::string_view(e.what()) == "from the #include side";
  }
  CHECK(caught);
#endif
  return 0;
}
