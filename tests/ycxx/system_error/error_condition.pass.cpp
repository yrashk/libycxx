// [syserr.errcondition.constructors]/1: error_condition() initializes val_ with 0 and cat_ with
// &generic_category(); /2: (val, cat); /3-4: template<class ErrorConditionEnum>
// error_condition(ErrorConditionEnum e), constrained on is_error_condition_enum_v, equivalent to
// `error_condition ec = make_error_condition(e); assign(ec.value(), ec.category());`.
// [syserr.errcondition.modifiers]/1-5: assign, operator= (constrained, returns *this), clear()
// (value() == 0, category() == generic_category()).
// [syserr.errcondition.observers]/1-4: value, category, message() returns
// category().message(value()), explicit operator bool returns value() != 0.
// [syserr.errcondition.nonmembers]/1: make_error_condition(errc e) returns
// error_condition(static_cast<int>(e), generic_category()).
// [system.error.syn]: is_error_condition_enum<errc> is true_type.
#include <system_error>
#include <cerrno>
#include <string>
#include <type_traits>
#include "check.hpp"

namespace lib {
struct Cat : std::error_category {
  const char* name() const noexcept override { return "libcond"; }
  std::string message(int ev) const override { return ev == 4 ? "four" : "other"; }
};
inline const Cat& cat() {
  static const Cat c;
  return c;
}
enum class Cond { ok = 0, slow = 2 };
inline std::error_condition make_error_condition(Cond c) noexcept { return {static_cast<int>(c) * 2, cat()}; }
struct Wrapped {
  int v;
  operator std::error_condition() const noexcept { return {v, cat()}; }
};
enum class Conv { x = 4 };
// copy-initialization: `error_condition ec = make_error_condition(e);`
inline Wrapped make_error_condition(Conv e) noexcept { return {static_cast<int>(e) + 1}; }
enum class CodeOnly { a = 1 };
inline std::error_code make_error_code(CodeOnly) noexcept { return {1, cat()}; }
inline std::error_condition make_error_condition(CodeOnly) noexcept { return {1, cat()}; }
}  // namespace lib

template <>
struct std::is_error_condition_enum<lib::Cond> : std::true_type {};
template <>
struct std::is_error_condition_enum<lib::Conv> : std::true_type {};
template <>
struct std::is_error_code_enum<lib::CodeOnly> : std::true_type {};

static_assert(std::is_error_condition_enum<std::errc>::value);
static_assert(std::is_base_of_v<std::true_type, std::is_error_condition_enum<std::errc>>);
static_assert(std::is_base_of_v<std::false_type, std::is_error_code_enum<std::errc>>);
static_assert(std::is_base_of_v<std::false_type, std::is_error_condition_enum<int>>);
static_assert(std::is_same_v<decltype(std::is_error_condition_enum_v<std::errc>), const bool>);
static_assert(std::is_same_v<decltype(std::is_error_code_enum_v<std::errc>), const bool>);

static_assert(std::is_nothrow_constructible_v<std::error_condition, std::errc>);
static_assert(std::is_convertible_v<std::errc, std::error_condition>);
static_assert(std::is_convertible_v<lib::Cond, std::error_condition>);
static_assert(!std::is_constructible_v<std::error_condition, lib::CodeOnly>);
static_assert(!std::is_constructible_v<std::error_condition, int>);
static_assert(!std::is_constructible_v<std::error_condition, std::error_code>);
static_assert(!std::is_assignable_v<std::error_condition&, lib::CodeOnly>);
static_assert(!std::is_assignable_v<std::error_condition&, int>);
static_assert(std::is_nothrow_assignable_v<std::error_condition&, std::errc>);
static_assert(std::is_same_v<decltype(std::declval<std::error_condition&>() = std::errc::io_error),
                             std::error_condition&>);
static_assert(std::is_nothrow_default_constructible_v<std::error_condition>);
static_assert(std::is_nothrow_constructible_v<std::error_condition, int, const std::error_category&>);
static_assert(std::is_nothrow_copy_constructible_v<std::error_condition>);
static_assert(std::is_nothrow_copy_assignable_v<std::error_condition>);
static_assert(!std::is_convertible_v<std::error_condition, bool>);
static_assert(noexcept(std::declval<std::error_condition&>().assign(0, std::generic_category())));
static_assert(noexcept(std::declval<std::error_condition&>().clear()));
static_assert(noexcept(std::declval<const std::error_condition&>().value()));
static_assert(noexcept(std::declval<const std::error_condition&>().category()));
static_assert(noexcept(static_cast<bool>(std::declval<const std::error_condition&>())));
static_assert(noexcept(std::make_error_condition(std::errc::io_error)));
static_assert(std::is_same_v<decltype(std::declval<const std::error_condition&>().message()), std::string>);
static_assert(std::is_same_v<decltype(std::make_error_condition(std::errc::io_error)), std::error_condition>);

int main() {
  std::error_condition d;
  CHECK(d.value() == 0 && &d.category() == &std::generic_category());
  CHECK(!d);

  std::error_condition a(4, lib::cat());
  CHECK(a.value() == 4 && &a.category() == &lib::cat() && a);
  CHECK(a.message() == "four");

  std::error_condition e = std::errc::permission_denied;
  CHECK(e.value() == EACCES && &e.category() == &std::generic_category());
  CHECK(e.message() == std::generic_category().message(EACCES));

  std::error_condition u = lib::Cond::slow;
  CHECK(u.value() == 4 && &u.category() == &lib::cat());

  std::error_condition w = lib::Conv::x;
  CHECK(w.value() == 5 && &w.category() == &lib::cat());
  w.clear();
  w = lib::Conv::x;
  CHECK(w.value() == 5 && &w.category() == &lib::cat());

  std::error_condition& r = (d = std::errc::timed_out);
  CHECK(&r == &d);
  CHECK(d.value() == ETIMEDOUT && &d.category() == &std::generic_category());
  d = lib::Cond::ok;
  CHECK(d.value() == 0 && &d.category() == &lib::cat() && !d);
  d.assign(-5, std::system_category());
  CHECK(d.value() == -5 && &d.category() == &std::system_category() && d);
  d.clear();
  CHECK(d.value() == 0 && d.category() == std::generic_category());

  std::error_condition mk = std::make_error_condition(std::errc::not_a_directory);
  CHECK(mk.value() == ENOTDIR && &mk.category() == &std::generic_category());
  std::error_condition mk2 = make_error_condition(std::errc::is_a_directory);  // ADL
  CHECK(mk2.value() == EISDIR);
  return 0;
}
