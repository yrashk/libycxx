// [syserr.errcode.nonmembers]/2: operator<<(os, ec) is equivalent to
// `return os << ec.category().name() << ':' << ec.value();`, for any character type.
#include <system_error>
#include <sstream>
#include <string>
#include <type_traits>
#include "check.hpp"

struct Cat : std::error_category {
  const char* name() const noexcept override { return "custom.cat"; }
  std::string message(int) const override { return "unused"; }
};

int main() {
  Cat c;
  {
    std::ostringstream os;
    std::ostream& r = (os << std::error_code(42, c));
    CHECK(&r == &os);
    CHECK(os.str() == "custom.cat:42");
  }
  {
    std::ostringstream os;
    os << std::make_error_code(std::errc::invalid_argument) << '|' << std::error_code(-3, c) << '|'
       << std::error_code();
    CHECK(os.str() == "generic:" + std::to_string(static_cast<int>(std::errc::invalid_argument)) +
                          "|custom.cat:-3|system:0");
  }
  {  // the formatting state of the stream applies to the value (it is an ordinary insertion)
    std::ostringstream os;
    os << std::hex << std::error_code(255, c);
    CHECK(os.str() == "custom.cat:ff");
  }
  {
    std::wostringstream os;
    std::wostream& r = (os << std::error_code(7, c));
    CHECK(&r == &os);
    CHECK(os.str() == L"custom.cat:7");
  }
  return 0;
}
