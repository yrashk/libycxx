// [except.throw]/4: "The points of potential destruction for the exception object are: (4.1)
// when an active handler for the exception exits by any means other than rethrowing,
// immediately after the destruction of the object (if any) declared in the
// exception-declaration in the handler; (4.2) when an object of type std::exception_ptr that
// refers to the exception object is destroyed ..." /9: std::terminate is invoked only "If the
// exception handling mechanism handling an uncaught exception directly invokes a function that
// exits via an exception"; once a handler is active the exception is caught (/8), and
// [except.terminate]/1 lists no case for a destructor of an exception object run at the end of
// a handler (only (1.4): destruction during stack unwinding). So when the exception object's
// destructor (noexcept(false)) exits via an exception as the handler completes, that exception
// propagates from the end of the handler like any other.
// REQUIRES: exceptions
#include <exception>
#include "check.hpp"

int destroyed = 0;
struct ThrowingDtor {
  bool armed = true;
  ~ThrowingDtor() noexcept(false) {
    ++destroyed;
    if (armed) throw 7L;
  }
};

int handler_exits_normally() {
  try {
    try {
      throw ThrowingDtor();
    } catch (ThrowingDtor&) {
      CHECK(std::uncaught_exceptions() == 0);
    }   // the exception object is destroyed here; its destructor throws 7L
    return 1;   // not reached
  } catch (long v) {
    CHECK(v == 7 && std::uncaught_exceptions() == 0);
    return 2;
  }
}

int handler_catch_all() {
  try {
    try {
      throw ThrowingDtor();
    } catch (...) {
    }
    return 1;
  } catch (long v) {
    return v == 7 ? 2 : 3;
  }
}

int handler_exits_by_return() {
  try {
    [] {
      try {
        throw ThrowingDtor();
      } catch (const ThrowingDtor&) {
        return;   // leaving the handler destroys the exception object
      }
    }();
    return 1;
  } catch (long) {
    return 2;
  }
}

int rethrown_then_caught() {
  try {
    try {
      try {
        throw ThrowingDtor();
      } catch (ThrowingDtor&) {
        throw;   // not a point of destruction
      }
    } catch (ThrowingDtor& again) {
      CHECK(destroyed == 3);   // not yet destroyed (3 from the previous cases)
      (void)again;
    }   // destroyed here
    return 1;
  } catch (long v) {
    return v == 7 ? 2 : 3;
  }
}

int disarmed() {
  try {
    throw ThrowingDtor();
  } catch (ThrowingDtor& e) {
    e.armed = false;   // the handler modifies the exception object itself
  }
  return 2;
}

int main() {
  CHECK(handler_exits_normally() == 2);
  CHECK(destroyed == 1);
  CHECK(handler_catch_all() == 2);
  CHECK(destroyed == 2);
  CHECK(handler_exits_by_return() == 2);
  CHECK(destroyed == 3);
  CHECK(rethrown_then_caught() == 2);
  CHECK(destroyed == 4);
  CHECK(disarmed() == 2);
  CHECK(destroyed == 5);
  CHECK(std::uncaught_exceptions() == 0 && std::current_exception() == nullptr);
  return 0;
}
