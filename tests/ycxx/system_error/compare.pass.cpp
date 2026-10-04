// [syserr.compare]/1: error_code == error_code: same category and value.
// [syserr.compare]/2: error_code == error_condition returns
//   lhs.category().equivalent(lhs.value(), rhs) || rhs.category().equivalent(lhs, rhs.value())
// (so both categories are consulted; error_condition == error_code is the rewritten form).
// [syserr.compare]/3: error_condition == error_condition: same category and value.
// [syserr.compare]/4-5: <=> orders by category (error_category::operator<=>, i.e. address
// order, [syserr.errcat.nonvirtuals]/2) first, then by value.
// [syserr.errcat.derived]/3-4: a program-defined category may override both equivalent overloads.
#include <system_error>
#include <cerrno>
#include <compare>
#include <functional>
#include <string>
#include <type_traits>
#include "check.hpp"

// A "library" category whose codes are equivalent to some generic conditions through its
// equivalent(int, const error_condition&) override.
struct LibCat : std::error_category {
  const char* name() const noexcept override { return "lib"; }
  std::string message(int) const override { return "lib"; }
  bool equivalent(int code, const std::error_condition& cond) const noexcept override {
    // code 1 means "file missing", code 2 means "bad argument"
    if (code == 1) return cond == std::errc::no_such_file_or_directory;
    if (code == 2) return cond == std::errc::invalid_argument;
    return false;
  }
};
// A "condition" category that recognises codes of other categories through its
// equivalent(const error_code&, int) override.
struct CondCat : std::error_category {
  const char* name() const noexcept override { return "cond"; }
  std::string message(int) const override { return "cond"; }
  bool equivalent(const std::error_code& code, int condition) const noexcept override {
    // condition 100: "any I/O-ish problem"
    if (condition == 100)
      return code == std::errc::io_error || code == std::errc::no_space_on_device ||
             code == std::error_code(1, lib_cat());
    return false;
  }
  static const std::error_category& lib_cat();
};
const LibCat lib_cat_obj;
const CondCat cond_cat_obj;
const std::error_category& CondCat::lib_cat() { return lib_cat_obj; }

static_assert(noexcept(std::error_code() == std::error_code()));
static_assert(noexcept(std::error_code() == std::error_condition()));
static_assert(noexcept(std::error_condition() == std::error_code()));
static_assert(noexcept(std::error_condition() == std::error_condition()));
static_assert(noexcept(std::error_code() <=> std::error_code()));
static_assert(noexcept(std::error_condition() <=> std::error_condition()));
static_assert(std::is_same_v<decltype(std::error_code() <=> std::error_code()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::error_condition() <=> std::error_condition()), std::strong_ordering>);
// no ordering between a code and a condition
template <class A, class B>
concept three_way = requires(const A& a, const B& b) { a <=> b; };
static_assert(!three_way<std::error_code, std::error_condition>);
static_assert(!three_way<std::error_condition, std::error_code>);

int main() {
  const std::error_category& g = std::generic_category();
  const std::error_category& s = std::system_category();
  const std::error_category& lib = lib_cat_obj;
  const std::error_category& cond = cond_cat_obj;

  // code == code ([syserr.compare]/1)
  CHECK(std::error_code(1, lib) == std::error_code(1, lib));
  CHECK(std::error_code(1, lib) != std::error_code(2, lib));
  CHECK(std::error_code(1, lib) != std::error_code(1, g));
  CHECK(std::error_code(ENOENT, s) != std::error_code(ENOENT, g));  // codes do not map
  CHECK(std::error_code() == std::error_code(0, s));
  CHECK(std::error_code() != std::error_code(0, g));

  // condition == condition ([syserr.compare]/3)
  CHECK(std::error_condition(3, g) == std::error_condition(3, g));
  CHECK(std::error_condition(3, g) != std::error_condition(3, s));
  CHECK(std::error_condition(3, g) != std::error_condition(4, g));
  CHECK(std::error_condition() == std::error_condition(0, g));

  // code == condition via the code's category ([syserr.compare]/2, first operand)
  CHECK(std::error_code(1, lib) == std::errc::no_such_file_or_directory);
  CHECK(std::errc::no_such_file_or_directory == std::error_code(1, lib));
  CHECK(std::error_code(2, lib) == std::make_error_condition(std::errc::invalid_argument));
  CHECK(std::error_code(1, lib) != std::errc::invalid_argument);
  CHECK(std::errc::invalid_argument != std::error_code(1, lib));
  // LibCat's override says no for code 3, but the condition's category is LibCat too and its
  // inherited equivalent(code, int) compares category and value ([syserr.errcat.virtuals]/4)
  CHECK(std::error_code(3, lib) == std::error_condition(3, lib));
  CHECK(std::error_code(3, lib) != std::error_condition(4, lib));
  // via the condition's category ([syserr.compare]/2, second operand)
  std::error_condition io_ish(100, cond);
  CHECK(std::make_error_code(std::errc::io_error) == io_ish);
  CHECK(io_ish == std::make_error_code(std::errc::no_space_on_device));
  CHECK(std::error_code(1, lib) == io_ish);
  CHECK(io_ish == std::error_code(1, lib));
  CHECK(std::make_error_code(std::errc::invalid_argument) != io_ish);
  CHECK(std::error_code(2, lib) != io_ish);
  // CondCat's own codes: its inherited equivalent(int, cond) is default_error_condition(100) ==
  // cond ([syserr.errcat.virtuals]/3), true, although its override equivalent(code, int) says no
  CHECK(std::error_code(100, cond) == io_ish);
  CHECK(std::error_code(101, cond) != io_ish);
  // the system category maps to generic conditions
  CHECK(std::error_code(EIO, s) == io_ish);
  CHECK(std::error_code(EIO, s) == std::errc::io_error);
  CHECK(std::errc::io_error == std::error_code(EIO, s));
  CHECK(!(std::error_code(EIO, s) != std::errc::io_error));
  // a generic code equals the generic condition with the same value
  CHECK(std::make_error_code(std::errc::bad_address) == std::errc::bad_address);
  CHECK(std::make_error_code(std::errc::bad_address) == std::error_condition(EFAULT, g));
  CHECK(std::make_error_code(std::errc::bad_address) != std::error_condition(EFAULT, s));

  // <=>: category order first, then value ([syserr.compare]/4)
  std::strong_ordering cat_order = lib <=> g;
  CHECK(cat_order != 0);
  CHECK((std::error_code(1, lib) <=> std::error_code(99, g)) == cat_order);
  CHECK((std::error_code(99, lib) <=> std::error_code(1, g)) == cat_order);
  CHECK((std::error_code(5, lib) <=> std::error_code(5, lib)) == std::strong_ordering::equal);
  CHECK((std::error_code(4, lib) <=> std::error_code(5, lib)) == std::strong_ordering::less);
  CHECK((std::error_code(-4, lib) <=> std::error_code(-5, lib)) == std::strong_ordering::greater);
  CHECK(std::error_code(4, lib) < std::error_code(5, lib));
  CHECK((std::error_code(1, lib) < std::error_code(99, g)) == std::less<const void*>()(&lib, &g));
  // ([syserr.compare]/5)
  CHECK((std::error_condition(1, lib) <=> std::error_condition(99, g)) == cat_order);
  CHECK((std::error_condition(7, g) <=> std::error_condition(7, g)) == 0);
  CHECK((std::error_condition(6, g) <=> std::error_condition(7, g)) < 0);
  CHECK(std::error_condition(8, g) > std::error_condition(7, g));
  CHECK(std::error_condition(7, g) >= std::error_condition(7, g));
  return 0;
}
