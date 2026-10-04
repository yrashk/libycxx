// [ios.failure]: ios_base::failure derives from system_error; failure(msg) uses
// io_errc::stream (code() == io_errc::stream); [iostreams.syn]/[ios.syn]: io_errc::stream == 1,
// is_error_code_enum<io_errc> is true; [error.reporting]: iostream_category().name() is
// "iostream"; make_error_code / make_error_condition.
#include <ios>
#include <cstring>
#include <string>
#include <system_error>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_base_of_v<std::system_error, std::ios_base::failure>);
static_assert(std::is_error_code_enum_v<std::io_errc>);
static_assert(static_cast<int>(std::io_errc::stream) == 1);
static_assert(noexcept(std::iostream_category()));

int main() {
  CHECK(std::strcmp(std::iostream_category().name(), "iostream") == 0);
  std::error_code ec = std::make_error_code(std::io_errc::stream);
  CHECK(ec.value() == 1 && &ec.category() == &std::iostream_category());
  std::error_condition cond = std::make_error_condition(std::io_errc::stream);
  CHECK(cond.value() == 1 && &cond.category() == &std::iostream_category());

  std::ios_base::failure f("msg");
  CHECK(f.code() == std::io_errc::stream);
  CHECK(std::string(f.what()).find("msg") != std::string::npos);
  std::ios_base::failure g(std::string("str"), std::make_error_code(std::errc::io_error));
  CHECK(g.code() == std::errc::io_error);
  std::ios_base::failure h("c", ec);
  CHECK(h.code() == ec);
  return 0;
}
