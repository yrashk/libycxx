// [std.exceptions] with P3068 ([expr.const]: a throw-expression whose exception is caught within
// the evaluation may appear in a constant expression): each <stdexcept> class has constexpr
// constructors ([logic.error]/2-3 ... [underflow.error]/2-3), and [exception] makes exception's
// copy operations, destructor and what() constexpr, so the classes can be thrown and caught -
// by a handler for the class itself, for a base, or by value (a copy, [exception]/2) - during
// constant evaluation. [except.handle]/3: a handler of type cv T or const T& matches a public
// base class of the thrown type.
// XFAIL-COMPILER: clang  no constexpr exception support (P3068) in clang yet
#include <stdexcept>
#include <exception>
#include <string>
#include <string_view>
#include "check.hpp"

template <class E, class Base>
constexpr bool test_one() {
  using sv = std::string_view;
  int hits = 0;
  // caught as itself
  try {
    throw E("thrown as itself");
  } catch (const E& e) {
    if (sv(e.what()) == "thrown as itself") ++hits;
  }
  // caught by its direct base, then std::exception
  try {
    std::string s = "built from a string that is longer than a small buffer";
    throw E(s);
  } catch (const Base& e) {
    if (sv(e.what()) == "built from a string that is longer than a small buffer") ++hits;
  }
  try {
    throw E("via exception");
  } catch (const std::exception& e) {
    if (sv(e.what()) == "via exception") ++hits;
  }
  // caught by value (copy-initializes the handler's parameter)
  try {
    throw E("by value");
  } catch (E e) {
    if (sv(e.what()) == "by value") ++hits;
    e = E("modified copy");
    if (sv(e.what()) == "modified copy") ++hits;
  }
  // rethrown and caught again; handler order selects the first match
  try {
    try {
      throw E("rethrown");
    } catch (const std::exception&) {
      throw;
    }
  } catch (const E& e) {
    if (sv(e.what()) == "rethrown") ++hits;
  } catch (...) {
    return false;
  }
  // not caught by an unrelated handler
  try {
    throw E("unrelated");
  } catch (int) {
    return false;
  } catch (const E&) {
    ++hits;
  }
  return hits == 7;
}

static_assert(test_one<std::logic_error, std::exception>());
static_assert(test_one<std::domain_error, std::logic_error>());
static_assert(test_one<std::invalid_argument, std::logic_error>());
static_assert(test_one<std::length_error, std::logic_error>());
static_assert(test_one<std::out_of_range, std::logic_error>());
static_assert(test_one<std::runtime_error, std::exception>());
static_assert(test_one<std::range_error, std::runtime_error>());
static_assert(test_one<std::overflow_error, std::runtime_error>());
static_assert(test_one<std::underflow_error, std::runtime_error>());

// the logic/runtime split: a logic_error handler does not catch a runtime_error
constexpr bool split() {
  try {
    try {
      throw std::overflow_error("o");
    } catch (const std::logic_error&) {
      return false;
    }
  } catch (const std::runtime_error& e) {
    return std::string_view(e.what()) == "o";
  }
  return false;
}
static_assert(split());

int main() {
  CHECK(test_one<std::logic_error, std::exception>());
  CHECK(test_one<std::out_of_range, std::logic_error>());
  CHECK(test_one<std::underflow_error, std::runtime_error>());
  CHECK(split());
  return 0;
}
