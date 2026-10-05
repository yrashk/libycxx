// [except.nested]/3: nested_exception() "calls current_exception() and stores the returned
// value." /4: rethrow_nested() const "throws the stored exception captured by *this" (each
// call). /5: nested_ptr() returns the stored exception. The stored exception_ptr keeps the
// exception alive after the handler exits ([propagation]/9), so a nested_exception can be
// rethrown later and from copies; the capture is the innermost currently handled exception.
// Copy assignment is defaulted, so it replaces the stored exception_ptr.
// REQUIRES: exceptions
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::declval<const std::nested_exception&>().nested_ptr()),
                             std::exception_ptr>);
static_assert(std::is_same_v<decltype(std::declval<const std::nested_exception&>().rethrow_nested()), void>);

struct Mixin : std::runtime_error, std::nested_exception {
  explicit Mixin(const char* s) : std::runtime_error(s) {}
};

int rethrown_int(const std::nested_exception& n) {
  try {
    n.rethrow_nested();
  } catch (int i) {
    return i;
  }
  return -1;
}

int main() {
  std::nested_exception kept = [] {
    try {
      throw 21;
    } catch (...) {
      return std::nested_exception();
    }
  }();
  CHECK(std::current_exception() == nullptr);
  CHECK(kept.nested_ptr() != nullptr);
  const std::nested_exception& ck = kept;
  CHECK(rethrown_int(ck) == 21);
  CHECK(rethrown_int(ck) == 21);  // can be called repeatedly

  std::nested_exception copy = kept;
  CHECK(copy.nested_ptr() == kept.nested_ptr());
  CHECK(rethrown_int(copy) == 21);

  // innermost handled exception is captured
  std::nested_exception inner_cap = [] {
    try {
      throw 1;
    } catch (...) {
      try {
        throw 2;
      } catch (...) {
        return std::nested_exception();
      }
    }
  }();
  CHECK(rethrown_int(inner_cap) == 2);

  // copy assignment replaces the capture; assigning an empty one clears it
  copy = inner_cap;
  CHECK(copy.nested_ptr() == inner_cap.nested_ptr());
  CHECK(rethrown_int(copy) == 2);
  std::nested_exception empty;
  copy = empty;
  CHECK(copy.nested_ptr() == nullptr);

  // as a mixin, constructed inside a handler
  try {
    try {
      throw std::logic_error("cause");
    } catch (...) {
      throw Mixin("effect");
    }
  } catch (const Mixin& m) {
    CHECK(m.what()[0] == 'e');
    bool cause = false;
    try {
      m.rethrow_nested();
    } catch (const std::logic_error& l) {
      cause = l.what()[0] == 'c';
    }
    CHECK(cause);
  }
  return 0;
}
