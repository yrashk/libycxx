// [syserr.errcode.constructors]/1: error_code() initializes val_ with 0 and cat_ with
// &system_category(); /2: error_code(val, cat); /3-4: template<class ErrorCodeEnum>
// error_code(ErrorCodeEnum e) is constrained on is_error_code_enum_v<ErrorCodeEnum> and is
// equivalent to `error_code ec = make_error_code(e); assign(ec.value(), ec.category());` (an
// unqualified call: make_error_code is found by argument-dependent lookup).
// [syserr.errcode.modifiers]/1-5: assign, operator=(ErrorCodeEnum) (constrained, returns *this),
// clear() (value() == 0, category() == system_category()).
// [syserr.errcode.observers]/1-5: value, category, default_error_condition() returns
// category().default_error_condition(value()), message() returns category().message(value()),
// explicit operator bool returns value() != 0.
// [syserr.errcode.nonmembers]/1: make_error_code(errc e) returns
// error_code(static_cast<int>(e), generic_category()).
// [system.error.syn]/2: is_error_code_enum may be specialized for program-defined types.
#include <system_error>
#include <cerrno>
#include <string>
#include <type_traits>
#include "check.hpp"

namespace lib {
struct Cat : std::error_category {
  const char* name() const noexcept override { return "lib"; }
  std::string message(int ev) const override { return "lib error " + std::to_string(ev); }
};
inline const Cat& cat() {
  static const Cat c;
  return c;
}
enum class Err { none = 0, bad = 3, worse = 7 };
// found by argument-dependent lookup only
inline std::error_code make_error_code(Err e) noexcept { return {static_cast<int>(e) * 10, cat()}; }

// an unscoped enumeration
enum Plain { plain_a = 1, plain_b = 2 };
inline std::error_code make_error_code(Plain e) noexcept { return {-static_cast<int>(e), cat()}; }

// a class type: the constraint is the trait, not "is an enumeration"
struct Token {
  int v;
};
inline std::error_code make_error_code(Token t) noexcept { return {t.v + 1000, cat()}; }

// make_error_code may return any type convertible to error_code: the effects are
// `error_code ec = make_error_code(e);` (copy-initialization)
struct Wrapped {
  int v;
  operator std::error_code() const noexcept { return {v, cat()}; }
};
enum class Conv { x = 4 };
inline Wrapped make_error_code(Conv e) noexcept { return {static_cast<int>(e) + 1}; }

// not registered as an error code enum
enum class Unregistered { x = 1 };
inline std::error_code make_error_code(Unregistered) noexcept { return {1, cat()}; }
}  // namespace lib

template <>
struct std::is_error_code_enum<lib::Err> : std::true_type {};
template <>
struct std::is_error_code_enum<lib::Plain> : std::true_type {};
template <>
struct std::is_error_code_enum<lib::Token> : std::true_type {};
template <>
struct std::is_error_code_enum<lib::Conv> : std::true_type {};

static_assert(std::is_error_code_enum_v<lib::Err>);
static_assert(!std::is_error_code_enum_v<lib::Unregistered>);
static_assert(!std::is_error_code_enum_v<int>);
static_assert(!std::is_error_code_enum_v<std::errc>);

// constraints ([syserr.errcode.constructors]/3, [syserr.errcode.modifiers]/2)
static_assert(std::is_nothrow_constructible_v<std::error_code, lib::Err>);
static_assert(std::is_convertible_v<lib::Err, std::error_code>);  // not explicit
static_assert(std::is_convertible_v<lib::Token, std::error_code>);
static_assert(!std::is_constructible_v<std::error_code, lib::Unregistered>);
static_assert(!std::is_constructible_v<std::error_code, std::errc>);  // a condition enum
static_assert(!std::is_constructible_v<std::error_code, int>);
static_assert(!std::is_assignable_v<std::error_code&, lib::Unregistered>);
static_assert(!std::is_assignable_v<std::error_code&, std::errc>);
static_assert(!std::is_assignable_v<std::error_code&, int>);
static_assert(std::is_nothrow_assignable_v<std::error_code&, lib::Err>);
static_assert(std::is_same_v<decltype(std::declval<std::error_code&>() = lib::Err::bad), std::error_code&>);

// signatures
static_assert(std::is_nothrow_default_constructible_v<std::error_code>);
static_assert(std::is_nothrow_constructible_v<std::error_code, int, const std::error_category&>);
static_assert(std::is_nothrow_copy_constructible_v<std::error_code>);
static_assert(std::is_nothrow_copy_assignable_v<std::error_code>);
static_assert(!std::is_convertible_v<std::error_code, bool>);       // explicit operator bool
static_assert(std::is_constructible_v<bool, std::error_code>);
static_assert(noexcept(std::declval<std::error_code&>().assign(0, std::generic_category())));
static_assert(noexcept(std::declval<std::error_code&>().clear()));
static_assert(noexcept(std::declval<const std::error_code&>().value()));
static_assert(noexcept(std::declval<const std::error_code&>().category()));
static_assert(noexcept(std::declval<const std::error_code&>().default_error_condition()));
static_assert(noexcept(static_cast<bool>(std::declval<const std::error_code&>())));
static_assert(noexcept(std::make_error_code(std::errc::io_error)));
static_assert(std::is_same_v<decltype(std::declval<const std::error_code&>().category()), const std::error_category&>);
static_assert(std::is_same_v<decltype(std::declval<const std::error_code&>().value()), int>);
static_assert(std::is_same_v<decltype(std::declval<const std::error_code&>().message()), std::string>);
static_assert(std::is_same_v<decltype(std::declval<std::error_code&>().assign(0, std::generic_category())), void>);
static_assert(std::is_same_v<decltype(std::declval<std::error_code&>().clear()), void>);
static_assert(std::is_same_v<decltype(std::declval<const std::error_code&>().default_error_condition()),
                             std::error_condition>);
static_assert(std::is_same_v<decltype(std::make_error_code(std::errc::io_error)), std::error_code>);

int main() {
  // default construction
  std::error_code d;
  CHECK(d.value() == 0);
  CHECK(&d.category() == &std::system_category());
  CHECK(!d);
  CHECK(!static_cast<bool>(d));

  // (val, cat)
  std::error_code e(5, lib::cat());
  CHECK(e.value() == 5 && &e.category() == &lib::cat());
  CHECK(static_cast<bool>(e));
  CHECK(std::error_code(-1, std::generic_category()));  // any nonzero value is true

  // from a user enum: make_error_code found by ADL
  std::error_code f = lib::Err::bad;
  CHECK(f.value() == 30 && &f.category() == &lib::cat());
  std::error_code g(lib::plain_b);
  CHECK(g.value() == -2 && &g.category() == &lib::cat());
  std::error_code h = lib::Token{5};
  CHECK(h.value() == 1005 && &h.category() == &lib::cat());
  std::error_code z = lib::Err::none;
  CHECK(z.value() == 0 && !z && &z.category() == &lib::cat());

  std::error_code w = lib::Conv::x;
  CHECK(w.value() == 5 && &w.category() == &lib::cat());
  w.clear();
  w = lib::Conv::x;
  CHECK(w.value() == 5 && &w.category() == &lib::cat());

  // assign / operator= / clear
  d.assign(EINVAL, std::generic_category());
  CHECK(d.value() == EINVAL && &d.category() == &std::generic_category());
  std::error_code& r = (d = lib::Err::worse);
  CHECK(&r == &d);
  CHECK(d.value() == 70 && &d.category() == &lib::cat());
  d = lib::plain_a;
  CHECK(d.value() == -1);
  d.clear();
  CHECK(d.value() == 0 && d.category() == std::system_category());
  CHECK(!d);
  e.clear();
  CHECK(e == std::error_code());

  // copy
  std::error_code c1(9, lib::cat());
  std::error_code c2 = c1;
  CHECK(c2.value() == 9 && &c2.category() == &lib::cat());
  c2 = std::error_code(1, std::generic_category());
  CHECK(c1.value() == 9 && c2.value() == 1);

  // observers
  std::error_code m(12, lib::cat());
  CHECK(m.message() == "lib error 12");
  std::error_condition dc = m.default_error_condition();
  CHECK(dc.value() == 12 && &dc.category() == &lib::cat());
  std::error_code s(ENOENT, std::system_category());
  CHECK(s.default_error_condition() == std::error_condition(ENOENT, std::generic_category()));
  CHECK(s.message() == std::system_category().message(ENOENT));

  // make_error_code(errc)
  std::error_code mk = std::make_error_code(std::errc::no_such_file_or_directory);
  CHECK(mk.value() == ENOENT && &mk.category() == &std::generic_category());
  // make_error_code(errc) is found unqualified through ADL too
  std::error_code mk2 = make_error_code(std::errc::invalid_argument);
  CHECK(mk2.value() == EINVAL && &mk2.category() == &std::generic_category());
  return 0;
}
