// [exception]: exception's default and copy constructors, copy assignment, destructor and what()
// are constexpr; /2: every library class derived from exception has non-throwing default (unless
// the synopsis shows other constructors) and copy constructors and copy assignment, a copy has
// strcmp(lhs.what(), rhs.what()) == 0, and what() meets exception::what()'s constraints; /5:
// what() returns an ntbs, during constant evaluation in the ordinary literal encoding.
// P3068 and P3378 make these classes usable in constant evaluation: bad_exception
// ([bad.exception]), bad_alloc ([bad.alloc]), bad_array_new_length ([new.badlength]),
// bad_cast ([bad.cast]), bad_typeid ([bad.typeid]), bad_weak_ptr ([util.smartptr.weak.bad]),
// bad_optional_access ([optional.bad.access]), bad_variant_access ([variant.bad.access]),
// bad_expected_access<E> and <void> ([expected.bad], [expected.bad.void]) and format_error
// ([format.error]: constexpr constructors from const string& and const char*, with
// strcmp(what(), what_arg) == 0).
// Nothing here throws, so this holds on compilers that cannot throw during constant evaluation.
#include <exception>
#include <expected>
#include <format>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <variant>
#include "check.hpp"

constexpr bool is_ntbs(const char* s) {
  if (s == nullptr) return false;
  std::size_t n = 0;
  while (s[n] != '\0') ++n; // reads the whole string: a constant evaluation rejects overruns
  return true;
}

constexpr bool same_text(const char* a, const char* b) { return std::string_view(a) == std::string_view(b); }

// Default-constructs, copies and assigns E, all within one constant evaluation, and reads
// what() through E and through exception&.
template <class E>
constexpr bool default_constructible_class() {
  static_assert(std::is_nothrow_default_constructible_v<E>);
  static_assert(std::is_nothrow_copy_constructible_v<E>);
  static_assert(std::is_nothrow_copy_assignable_v<E>);
  static_assert(std::is_base_of_v<std::exception, E>);
  E a;
  if (!is_ntbs(a.what())) return false;
  E b(a);
  if (!same_text(a.what(), b.what())) return false;
  E c;
  c = b;
  if (!same_text(c.what(), a.what())) return false;
  const std::exception& base = c;
  if (!same_text(base.what(), a.what())) return false;
  return true;
}

static_assert(default_constructible_class<std::exception>());
static_assert(default_constructible_class<std::bad_exception>());
static_assert(default_constructible_class<std::bad_alloc>());
static_assert(default_constructible_class<std::bad_array_new_length>());
static_assert(default_constructible_class<std::bad_cast>());
static_assert(default_constructible_class<std::bad_typeid>());
static_assert(default_constructible_class<std::bad_weak_ptr>());
static_assert(default_constructible_class<std::bad_optional_access>());
static_assert(default_constructible_class<std::bad_variant_access>());

// bad_array_new_length is a bad_alloc ([new.badlength]).
constexpr bool bad_array_new_length_is_bad_alloc() {
  std::bad_array_new_length e;
  const std::bad_alloc& b = e;
  return same_text(b.what(), e.what());
}
static_assert(bad_array_new_length_is_bad_alloc());

// [expected.bad]: constexpr explicit bad_expected_access(E); error() in all four forms.
struct Code {
  int v;
};
constexpr bool bad_expected_access_value() {
  std::bad_expected_access<Code> e(Code{7});
  if (!is_ntbs(e.what())) return false;
  if (e.error().v != 7) return false;
  e.error().v = 8;
  const auto& ce = e;
  if (ce.error().v != 8) return false;
  if (std::move(e).error().v != 8) return false;
  if (std::move(ce).error().v != 8) return false;
  std::bad_expected_access<Code> copy(e);
  if (copy.error().v != 8 || !same_text(copy.what(), e.what())) return false;
  const std::bad_expected_access<void>& base = copy; // [expected.bad]: derives from <void>
  const std::exception& ex = base;
  return same_text(ex.what(), e.what());
}
static_assert(bad_expected_access_value());

// [expected.bad.void]: the special members are protected and constexpr; a derived class uses them.
struct my_access : std::bad_expected_access<void> {
  constexpr my_access() noexcept = default;
  constexpr my_access(const my_access&) noexcept = default;
  constexpr my_access& operator=(const my_access&) noexcept = default;
};
constexpr bool bad_expected_access_void() {
  my_access a;
  my_access b(a);
  a = b;
  return is_ntbs(a.what()) && same_text(a.what(), b.what());
}
static_assert(bad_expected_access_void());

// [format.error]: both constructors, the postconditions, copies.
constexpr bool format_error_class() {
  std::format_error a("from a literal");
  if (!same_text(a.what(), "from a literal")) return false;
  std::string s = "from a string long enough to need an allocation of its own";
  std::format_error b(s);
  s[0] = 'X'; // the message is a copy
  if (!same_text(b.what(), "from a string long enough to need an allocation of its own")) return false;
  std::format_error c(b);
  if (!same_text(c.what(), b.what())) return false;
  c = a;
  if (!same_text(c.what(), "from a literal")) return false;
  const std::runtime_error& r = c;
  return same_text(r.what(), "from a literal");
}
static_assert(format_error_class());

int main() {
  CHECK(default_constructible_class<std::exception>());
  CHECK(default_constructible_class<std::bad_alloc>());
  CHECK(default_constructible_class<std::bad_weak_ptr>());
  CHECK(bad_expected_access_value());
  CHECK(bad_expected_access_void());
  CHECK(format_error_class());
  return 0;
}
