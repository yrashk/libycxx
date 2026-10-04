// [text.encoding.members]/11-12: literal() returns a text_encoding object representing the
// ordinary character literal encoding. Both GCC and Clang default to UTF-8 for ordinary
// literals (-fexec-charset=UTF-8), so literal() is UTF-8 here; it is consteval.
#include <text_encoding>
#include "check.hpp"

static_assert(std::text_encoding::literal() == std::text_encoding::id::UTF8);
static_assert(std::text_encoding::literal().mib() == std::text_encoding::UTF8);

int main() {
  CHECK(std::text_encoding::literal() == std::text_encoding(std::text_encoding::id::UTF8));
  return 0;
}
