// [uncaught.exceptions]/1: "Returns: The number of uncaught exceptions ([except.throw])."
// [except.throw]/7: "An exception is considered uncaught after completing the initialization
// of the exception object until completing the activation of a handler for the exception
// ([except.handle])." [except.handle]/7: "the exception is considered uncaught again when a
// handler rethrows it ("throw;", [expr.throw])"; [propagation]/10: rethrow_exception throws the
// exception object again.
// Destructors run during stack unwinding may themselves throw and catch exceptions, so the count
// can exceed one: each level of nesting adds one while its exception is being propagated.
#include <exception>
#include "check.hpp"

int during_ctor = -1;
struct Thrown {
  Thrown() { during_ctor = std::uncaught_exceptions(); }   // exception object not yet initialized
};

int depth3 = -1, depth2 = -1, depth2_after = -1, depth1 = -1;
struct Inner {
  ~Inner() { depth3 = std::uncaught_exceptions(); }
};
struct Middle {
  ~Middle() {
    depth2 = std::uncaught_exceptions();
    try {
      Inner in;
      throw 3;   // unwinding destroys `in` while 3 levels are uncaught
    } catch (int) {
      depth2_after = std::uncaught_exceptions();
    }
  }
};
struct Outer {
  ~Outer() {
    depth1 = std::uncaught_exceptions();
    try {
      Middle m;
      throw 2;
    } catch (int) {
    }
  }
};

int at_rethrow = -1;
struct ProbeRethrow {
  ~ProbeRethrow() { at_rethrow = std::uncaught_exceptions(); }
};

int main() {
  try {
    throw Thrown();
  } catch (const Thrown&) {
  }
  CHECK(during_ctor == 0);

  try {
    Outer o;
    throw 1;
  } catch (int) {
    CHECK(std::uncaught_exceptions() == 0);
  }
  CHECK(depth1 == 1);
  CHECK(depth2 == 2);
  CHECK(depth3 == 3);
  CHECK(depth2_after == 2);

  // throw; makes the exception uncaught again.
  try {
    try {
      throw 5;
    } catch (int) {
      CHECK(std::uncaught_exceptions() == 0);
      ProbeRethrow p;
      throw;
    }
  } catch (int) {
  }
  CHECK(at_rethrow == 1);

  // rethrow_exception likewise.
  std::exception_ptr ep;
  try {
    throw 6;
  } catch (...) {
    ep = std::current_exception();
  }
  at_rethrow = -1;
  try {
    ProbeRethrow p;
    std::rethrow_exception(ep);
  } catch (int v) {
    CHECK(v == 6);
    CHECK(std::uncaught_exceptions() == 0);
  }
  CHECK(at_rethrow == 1);

  // A handler that catches and then exits by another throw: the first exception is caught, the
  // second is uncaught while it propagates.
  at_rethrow = -1;
  try {
    try {
      throw 7;
    } catch (int) {
      ProbeRethrow p;
      throw 8L;
    }
  } catch (long) {
  }
  CHECK(at_rethrow == 1);
  CHECK(std::uncaught_exceptions() == 0);
}
