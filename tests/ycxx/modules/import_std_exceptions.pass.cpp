// [std.modules]/2: the exception classes, exception_ptr and the library's own throws work through
// `import std;`: a std::out_of_range thrown inside the library is caught as declared by the module.
// MODULES: std
// REQUIRES: exceptions
import std;
#include "module_check.hpp"

int main() {
  std::vector<int> v(2);
  bool caught = false;
  try {
    (void)v.at(5);
  } catch (const std::out_of_range& e) {
    caught = std::string_view(e.what()).size() > 0;
  }
  CHECK(caught);
  try {
    throw std::runtime_error("mine");
  } catch (const std::exception& e) {
    CHECK(std::string_view(e.what()) == "mine");
  }
  auto p = std::make_exception_ptr(std::logic_error("ptr"));
  try {
    std::rethrow_exception(p);
  } catch (const std::logic_error& e) {
    CHECK(std::string_view(e.what()) == "ptr");
  }
  try {
    std::optional<int> o;
    (void)o.value();
  } catch (const std::bad_optional_access&) {
    caught = false;
  }
  CHECK(!caught);
  try {
    throw std::system_error(std::make_error_code(std::errc::io_error), "io");
  } catch (const std::system_error& e) {
    CHECK(e.code() == std::errc::io_error);
  }
  CHECK(std::uncaught_exceptions() == 0);
  return 0;
}
