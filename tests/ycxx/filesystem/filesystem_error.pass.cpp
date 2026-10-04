// [fs.class.filesystem.error], [fs.filesystem.error.members]: filesystem_error derives from
// system_error; the constructors store what_arg, the error_code and zero, one or two paths
// (path1()/path2() are empty when not supplied); what() "Returns: An ntbs that incorporates the
// what_arg argument supplied to the constructor. The exact format is unspecified.
// Implementations should include the system_error::what() string and the pathnames of path1 and
// path2 in the native format in the returned string."
#include <filesystem>
#include <string>
#include <system_error>
#include <type_traits>
#include "check.hpp"

namespace fs = std::filesystem;

static_assert(std::is_base_of_v<std::system_error, fs::filesystem_error>);

int main() {
  auto ec = std::make_error_code(std::errc::permission_denied);
  fs::filesystem_error e0("zero", ec);
  CHECK(e0.code() == ec);
  CHECK(e0.path1().empty() && e0.path2().empty());
  CHECK(std::string(e0.what()).find("zero") != std::string::npos);

  fs::filesystem_error e1("one", fs::path("/p1"), ec);
  CHECK(e1.path1() == "/p1" && e1.path2().empty());
  CHECK(std::string(e1.what()).find("one") != std::string::npos);

  fs::filesystem_error e2(std::string("two"), fs::path("a"), fs::path("b"), ec);
  CHECK(e2.path1() == "a" && e2.path2() == "b");
  CHECK(e2.code() == std::errc::permission_denied);
  std::string w = e2.what();
  CHECK(w.find("two") != std::string::npos);

  fs::filesystem_error copy = e2;
  CHECK(copy.path2() == "b");
  static_assert(noexcept(e2.path1()));
  static_assert(noexcept(e2.what()));
  return 0;
}
