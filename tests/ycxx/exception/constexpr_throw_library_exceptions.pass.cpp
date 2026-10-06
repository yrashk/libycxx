// P3068: a throw-expression whose exception is caught within the same evaluation can appear in
// a constant expression ([expr.const]), and the library's exception classes are usable there
// ([exception], [bad.exception], [bad.alloc], [new.badlength], [bad.cast], [bad.typeid],
// [util.smartptr.weak.bad], [optional.bad.access], [variant.bad.access], [expected.bad],
// [format.error]). The library functions that report errors with them are constexpr and throw
// during constant evaluation as at run time:
// [optional.observe]: value() throws bad_optional_access if has_value() is false;
// [variant.get]: get<I>/get<T> throw bad_variant_access if the alternative is not active;
// [variant.visit]: visit throws bad_variant_access if a variant is valueless (not reachable
//   here: emplace that throws is needed);
// [expected.object.obs], [expected.void.obs]: value() throws bad_expected_access(error());
// [format.err.report]: format functions report errors by throwing format_error.
// [except.handle]/3: a handler for const B& catches a thrown D publicly derived from B.
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <exception>
#include <expected>
#include <format>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <variant>
#include "check.hpp"

constexpr bool same_text(const char* a, const char* b) { return std::string_view(a) == std::string_view(b); }

// Throws a default-constructed E and catches it as E, as std::exception and by value.
template <class E>
constexpr bool throw_default() {
  int hits = 0;
  const char* expected = E().what();
  try {
    throw E();
  } catch (const E& e) {
    hits += same_text(e.what(), expected);
  }
  try {
    throw E();
  } catch (const std::exception& e) {
    hits += same_text(e.what(), expected);
  }
  try {
    throw E();
  } catch (E e) {
    hits += same_text(e.what(), expected);
  }
  try {
    throw E();
  } catch (...) {
    ++hits;
  }
  return hits == 4;
}
static_assert(throw_default<std::exception>());
static_assert(throw_default<std::bad_exception>());
static_assert(throw_default<std::bad_alloc>());
static_assert(throw_default<std::bad_array_new_length>());
static_assert(throw_default<std::bad_cast>());
static_assert(throw_default<std::bad_typeid>());
static_assert(throw_default<std::bad_weak_ptr>());
static_assert(throw_default<std::bad_optional_access>());
static_assert(throw_default<std::bad_variant_access>());

constexpr bool bad_array_new_length_caught_as_bad_alloc() {
  try {
    throw std::bad_array_new_length();
  } catch (const std::bad_alloc&) {
    return true;
  }
  return false;
}
static_assert(bad_array_new_length_caught_as_bad_alloc());

constexpr bool optional_value() {
  std::optional<int> o;
  try {
    (void)o.value();
    return false;
  } catch (const std::bad_optional_access& e) {
    if (!same_text(e.what(), std::bad_optional_access().what())) return false;
  }
  o = 3;
  try {
    return o.value() == 3; // engaged: no exception
  } catch (...) {
    return false;
  }
}
static_assert(optional_value());

constexpr bool variant_get() {
  std::variant<int, double> v(1.5);
  int caught = 0;
  try {
    (void)std::get<int>(v);
  } catch (const std::bad_variant_access&) {
    ++caught;
  }
  try {
    (void)std::get<0>(v);
  } catch (const std::exception&) {
    ++caught;
  }
  try {
    (void)std::get<double>(std::as_const(v));
  } catch (...) {
    return false;
  }
  return caught == 2;
}
static_assert(variant_get());

struct Code {
  int v;
};
constexpr bool expected_value() {
  std::expected<int, Code> e = std::unexpected(Code{42});
  try {
    (void)e.value();
    return false;
  } catch (const std::bad_expected_access<Code>& x) {
    if (x.error().v != 42) return false;
  }
  try {
    (void)std::move(e).value();
    return false;
  } catch (const std::bad_expected_access<void>&) { // [expected.bad]: the public base
  }
  std::expected<void, Code> ev = std::unexpected(Code{9});
  try {
    ev.value();
    return false;
  } catch (const std::bad_expected_access<Code>& x) {
    return x.error().v == 9;
  }
}
static_assert(expected_value());

constexpr bool format_errors() {
  int caught = 0;
  try {
    // A runtime format string is checked when formatting: an unterminated replacement field.
    (void)std::format(std::runtime_format("{"), 1);
  } catch (const std::format_error&) {
    ++caught;
  }
  try {
    // A format specification the argument's formatter rejects ([format.string.std]: type d is
    // not valid for strings). (An argument index past the arguments is not a constant
    // expression rather than a format_error: [format.parse.ctx]/13.)
    (void)std::format(std::runtime_format("{:d}"), "text");
  } catch (const std::exception&) {
    ++caught;
  }
  try {
    throw std::format_error("user message");
  } catch (const std::runtime_error& e) {
    caught += same_text(e.what(), "user message");
  }
  return caught == 3;
}
static_assert(format_errors());

// A handler can rethrow (`throw;`) to an outer handler, and a new exception thrown in a handler
// replaces the handled one once that handler exits ([except.throw], [except.handle]).
constexpr bool rethrow_and_replace() {
  int seen = 0;
  try {
    try {
      throw std::bad_cast();
    } catch (const std::exception&) {
      ++seen;
      throw;
    }
  } catch (const std::bad_cast&) {
    ++seen;
  }
  try {
    try {
      throw std::bad_typeid();
    } catch (const std::bad_typeid&) {
      throw std::bad_exception();
    }
  } catch (const std::bad_typeid&) {
    return false;
  } catch (const std::bad_exception&) {
    ++seen;
  }
  return seen == 3;
}
static_assert(rethrow_and_replace());

int main() {
  CHECK(throw_default<std::bad_alloc>());
  CHECK(optional_value());
  CHECK(variant_get());
  CHECK(expected_value());
  CHECK(format_errors());
  CHECK(rethrow_and_replace());
  return 0;
}
