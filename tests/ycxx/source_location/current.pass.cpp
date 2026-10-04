// [support.srcloc.class]: source_location models semiregular, is_nothrow_swappable_v is true;
// current() is consteval noexcept; default ctor is constexpr noexcept; observers are constexpr
// noexcept and return uint_least32_t / const char*. [support.srcloc.cons]: current() yields the
// presumed line (affected by #line), the presumed file name, and the function name "such as in
// __func__"; a call used as a default argument corresponds to the location of the invocation.
#include <source_location>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "check.hpp"

using SL = std::source_location;
static_assert(std::semiregular<SL>);
static_assert(std::is_nothrow_swappable_v<SL>);
static_assert(std::is_nothrow_default_constructible_v<SL>);
static_assert(std::is_nothrow_copy_constructible_v<SL>);
static_assert(noexcept(SL::current()));
static_assert(std::is_same_v<decltype(SL().line()), std::uint_least32_t>);
static_assert(std::is_same_v<decltype(SL().column()), std::uint_least32_t>);
static_assert(std::is_same_v<decltype(SL().file_name()), const char*>);
static_assert(std::is_same_v<decltype(SL().function_name()), const char*>);
static_assert(noexcept(SL().line()) && noexcept(SL().column()) && noexcept(SL().file_name()) &&
              noexcept(SL().function_name()));

constexpr SL here = SL::current();
static_assert(here.line() == __LINE__ - 1);

constexpr std::uint_least32_t where(SL loc = SL::current()) { return loc.line(); }
static_assert(where() == __LINE__);

struct Member {
  SL loc = SL::current();
  Member() = default;  // the default member initializer corresponds to this constructor
};

const char* named_function() { return SL::current().function_name(); }

int main() {
  SL loc = SL::current();
  CHECK(loc.line() == __LINE__ - 1);
  CHECK(std::strcmp(loc.file_name(), __FILE__) == 0);
  CHECK(std::strstr(loc.function_name(), "main") != nullptr);
  CHECK(std::strstr(named_function(), "named_function") != nullptr);
#line 4242 "renamed.cpp"
  SL moved = SL::current();
  CHECK(moved.line() == 4242);
  CHECK(std::strcmp(moved.file_name(), "renamed.cpp") == 0);
  CHECK(where() == 4245);
  SL copy = moved;
  CHECK(copy.line() == moved.line() && copy.column() == moved.column());
  CHECK(std::strcmp(copy.function_name(), moved.function_name()) == 0);
  SL def;
  CHECK(def.file_name() != nullptr && def.function_name() != nullptr);  // ntbs
  Member m;
  CHECK(m.loc.file_name() != nullptr);
  return 0;
}
