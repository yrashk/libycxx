// [propagation]/12: template<class E> constexpr exception_ptr make_exception_ptr(E e) noexcept;
// "Creates an exception_ptr object that refers to a copy of e, as if: try { throw e; }
// catch(...) { return current-exception(); }". E is deduced from a by-value parameter, so an
// array argument decays to a pointer and a function to a function pointer; non-class types
// are fine. Handler matching follows [except.handle]/3, e.g. (3.4) a handler of pointer type
// matches an exception of type std::nullptr_t, and no arithmetic conversions apply.
// REQUIRES: exceptions
#include <exception>
#include <cstddef>
#include <optional>
#include <type_traits>
#include "check.hpp"

enum class Color { red = 4, green };
int fn() { return 11; }

static_assert(std::is_same_v<decltype(std::make_exception_ptr("abc")), std::exception_ptr>);

template <class Catch, class E>
bool caught_as(E e) {
  try {
    std::rethrow_exception(std::make_exception_ptr(e));
  } catch (Catch) {
    return true;
  } catch (...) {
  }
  return false;
}

int main() {
  // integral: no conversions in handler matching
  CHECK(caught_as<int>(5));
  CHECK(!caught_as<long>(5));
  CHECK(!caught_as<unsigned>(5));
  CHECK(!caught_as<double>(5));
  CHECK(caught_as<const int&>(5));

  // enumeration
  const std::exception_ptr pc = std::make_exception_ptr(Color::green);
  auto oc = std::exception_ptr_cast<Color>(pc);
  CHECK(oc.has_value() && *oc == Color::green);
  CHECK(!std::exception_ptr_cast<int>(pc).has_value());

  // string literal: E is const char*
  bool lit = false;
  try {
    std::rethrow_exception(std::make_exception_ptr("abc"));
  } catch (const char* s) {
    lit = s[0] == 'a' && s[2] == 'c';
  } catch (...) {
  }
  CHECK(lit);

  // an array lvalue decays to a pointer to its first element
  char buf[4] = {'x', 'y', 'z', 0};
  bool arr = false;
  try {
    std::rethrow_exception(std::make_exception_ptr(buf));
  } catch (char* p) {
    arr = p == buf;
  } catch (...) {
  }
  CHECK(arr);

  // a function decays to a function pointer
  int fv = 0;
  try {
    std::rethrow_exception(std::make_exception_ptr(fn));
  } catch (int (*f)()) {
    fv = f();
  } catch (...) {
  }
  CHECK(fv == 11);

  // nullptr_t: caught by nullptr_t and by any pointer handler
  CHECK(caught_as<std::nullptr_t>(nullptr));
  CHECK(caught_as<int*>(nullptr));
  CHECK(caught_as<const char*>(nullptr));
  const std::exception_ptr pn = std::make_exception_ptr(nullptr);
  CHECK(std::exception_ptr_cast<std::nullptr_t>(pn).has_value());

  // floating point
  const std::exception_ptr pd = std::make_exception_ptr(2.5);
  auto od = std::exception_ptr_cast<double>(pd);
  CHECK(od.has_value() && *od == 2.5);
  CHECK(!std::exception_ptr_cast<float>(pd).has_value());

  // each call creates a distinct exception object
  CHECK(std::make_exception_ptr(1) != std::make_exception_ptr(1));

  // an exception_ptr can itself be the exception object
  const std::exception_ptr inner = std::make_exception_ptr(3);
  int got = 0;
  try {
    std::rethrow_exception(std::make_exception_ptr(inner));
  } catch (const std::exception_ptr& e) {
    CHECK(e == inner);
    try {
      std::rethrow_exception(e);
    } catch (int i) {
      got = i;
    }
  }
  CHECK(got == 3);
  return 0;
}
