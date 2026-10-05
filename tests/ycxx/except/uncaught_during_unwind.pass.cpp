// [except.throw]/6: "An exception is considered uncaught after completing the initialization
// of the exception object until completing the activation of a handler for the exception."
// /7: "If an exception is rethrown, it is considered uncaught from the point of rethrow until
// the rethrown exception is caught." /8: caught when a handler becomes active.
// [uncaught.exceptions]: uncaught_exceptions() returns the number of uncaught exceptions.
// A destructor run by unwinding may itself throw and catch internally: then two exceptions
// are uncaught at once.
// REQUIRES: exceptions
#include <exception>
#include "check.hpp"

static int in_dtor = -1, in_nested_dtor = -1, in_nested_handler = -1, after_nested = -1;
static int in_copy = -1;

struct Inner {
  ~Inner() { in_nested_dtor = std::uncaught_exceptions(); }
};

struct Guard {
  ~Guard() {
    in_dtor = std::uncaught_exceptions();
    try {
      Inner i;
      throw 2;
    } catch (int) {
      in_nested_handler = std::uncaught_exceptions();
    }
    after_nested = std::uncaught_exceptions();
  }
};

struct Copied {
  Copied() = default;
  Copied(const Copied&) { in_copy = std::uncaught_exceptions(); }
};
struct FromBase : Copied {};

static int rethrow_dtor = -1;
struct RethrowProbe {
  ~RethrowProbe() { rethrow_dtor = std::uncaught_exceptions(); }
};

int main() {
  try {
    Guard g;
    throw 1;
  } catch (int) {
    CHECK(std::uncaught_exceptions() == 0);
  }
  CHECK(in_dtor == 1);
  CHECK(in_nested_dtor == 2);
  CHECK(in_nested_handler == 1);
  CHECK(after_nested == 1);

  // the handler's parameter is initialized while the exception is still uncaught
  try {
    throw FromBase();
  } catch (Copied c) {  // copy from the base subobject: cannot be elided
    (void)c;
    CHECK(std::uncaught_exceptions() == 0);
  }
  CHECK(in_copy == 1);

  // rethrow: uncaught again during the unwinding it causes
  try {
    try {
      throw 3;
    } catch (int) {
      CHECK(std::uncaught_exceptions() == 0);
      RethrowProbe rp;
      throw;
    }
  } catch (int) {
    CHECK(std::uncaught_exceptions() == 0);
  }
  CHECK(rethrow_dtor == 1);

  // throw inside a handler: the handled one is caught, the new one uncaught
  int during = -1;
  struct P2 {
    int* out;
    ~P2() { *out = std::uncaught_exceptions(); }
  };
  try {
    try {
      throw 4;
    } catch (int) {
      P2 p{&during};
      throw 5L;
    }
  } catch (long) {
  }
  CHECK(during == 1);
  CHECK(std::uncaught_exceptions() == 0);
  return 0;
}
