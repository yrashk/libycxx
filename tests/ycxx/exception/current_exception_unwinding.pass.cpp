// [propagation]/9: current_exception() returns "an exception_ptr object that refers to the
// currently handled exception or a copy ..., or a null exception_ptr object if no exception
// is being handled." [except.handle]/9: "A handler is considered active when initialization
// is complete for the parameter (if any) of the catch clause. [Note 4: The stack will have
// been unwound at that point.] ... A handler is no longer considered active when the catch
// clause exits." /10: "The exception with the most recently activated handler that is still
// active is called the currently handled exception." So destructors run by stack unwinding
// (before any handler of the new exception is active) observe the previously handled
// exception, or null when there is none; a function-try-block handler is an ordinary handler.
// REQUIRES: exceptions
#include <exception>
#include <stdexcept>
#include "check.hpp"

struct Probe {
  std::exception_ptr* out;
  int* uncaught;
  ~Probe() {
    *out = std::current_exception();
    *uncaught = std::uncaught_exceptions();
  }
};

int kind(const std::exception_ptr& p) {
  try {
    std::rethrow_exception(p);
  } catch (const std::logic_error&) {
    return 1;
  } catch (int) {
    return 2;
  } catch (double) {
    return 3;
  }
  return 0;
}

struct Ctor {
  std::exception_ptr seen;
  int got = 0;
  Ctor() try : got(thrower()) {
  } catch (...) {
    // handler of a constructor's function-try-block; the exception is rethrown at its end
    seen = std::current_exception();
  }
  static int thrower() { throw 2.5; }
};

int main() {
  std::exception_ptr seen = std::make_exception_ptr(99);  // non-null sentinel
  int uncaught = -1;

  // unwinding with no handler active: null
  try {
    Probe p{&seen, &uncaught};
    throw 7;
  } catch (int) {
  }
  CHECK(seen == nullptr);
  CHECK(uncaught == 1);

  // unwinding a second exception inside a handler: the handled exception is still current
  try {
    throw std::logic_error("outer");
  } catch (...) {
    seen = nullptr;
    try {
      Probe p{&seen, &uncaught};
      throw 7;
    } catch (int) {
      CHECK(kind(std::current_exception()) == 2);  // the inner handler is the most recent
    }
    CHECK(seen != nullptr);
    CHECK(kind(seen) == 1);
    CHECK(uncaught == 1);
    CHECK(kind(std::current_exception()) == 1);  // back to the outer exception
  }
  CHECK(std::current_exception() == nullptr);

  // a function-try-block handler of a constructor
  bool rethrown = false;
  try {
    Ctor c;
  } catch (double) {
    rethrown = true;
    CHECK(kind(std::current_exception()) == 3);
  }
  CHECK(rethrown);
  CHECK(std::current_exception() == nullptr);
  return 0;
}
