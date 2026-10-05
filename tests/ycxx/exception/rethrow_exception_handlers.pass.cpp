// [propagation]/11: rethrow_exception(p): "Let u be the exception object to which p refers, or
// a copy of that exception object. ... throws u." The thrown object takes part in ordinary
// handler matching ([except.handle]) and can be rethrown with "throw;" ([expr.throw]);
// [uncaught.exceptions]: during the stack unwinding it starts, uncaught_exceptions() is 1.
// [propagation]/9: the referenced object stays valid as long as an exception_ptr refers to it;
// once the last exception_ptr and the last handler are gone, it is destroyed.
// REQUIRES: exceptions
#include <exception>
#include <stdexcept>
#include <utility>
#include "check.hpp"

struct Tracked {
  static inline int live = 0;
  int v;
  explicit Tracked(int x) : v(x) { ++live; }
  Tracked(const Tracked& o) : v(o.v) { ++live; }
  virtual ~Tracked() { --live; }
};
struct DerivedTracked : Tracked {
  explicit DerivedTracked(int x) : Tracked(x) {}
};

struct Witness {
  int* out;
  ~Witness() { *out = std::uncaught_exceptions(); }
};

[[noreturn]] void rethrow_with_witness(std::exception_ptr p, int* out) {
  Witness w{out};
  std::rethrow_exception(std::move(p));
}

int main() {
  // matched by a base-class handler, then "throw;" rethrows the same dynamic type
  {
    std::exception_ptr p = std::make_exception_ptr(DerivedTracked(4));
    int stage = 0;
    try {
      try {
        std::rethrow_exception(p);
      } catch (const Tracked& t) {
        CHECK(t.v == 4);
        stage = 1;
        throw;
      }
    } catch (const DerivedTracked& d) {
      CHECK(d.v == 4);
      stage = 2;
    }
    CHECK(stage == 2);
  }
  CHECK(Tracked::live == 0);

  // standard exception hierarchy
  {
    std::exception_ptr p = std::make_exception_ptr(std::out_of_range("oor"));
    bool ok = false;
    try {
      std::rethrow_exception(p);
    } catch (const std::exception& e) {
      ok = e.what()[0] == 'o';
    }
    CHECK(ok);
  }

  // uncaught_exceptions during the unwinding started by rethrow_exception
  {
    int during = -1;
    try {
      rethrow_with_witness(std::make_exception_ptr(1), &during);
    } catch (int) {
      CHECK(std::uncaught_exceptions() == 0);
    }
    CHECK(during == 1);
  }

  // the only exception_ptr is moved into rethrow_exception: the object survives until the
  // handler exits, then is destroyed
  {
    std::exception_ptr p = std::make_exception_ptr(Tracked(9));
    CHECK(Tracked::live >= 1);
    int seen = 0;
    try {
      std::rethrow_exception(std::move(p));
    } catch (const Tracked& t) {
      seen = t.v;
      CHECK(Tracked::live >= 1);
    }
    CHECK(seen == 9);
    p = nullptr;
    CHECK(Tracked::live == 0);
  }

  // rethrowing inside a handler of another exception; the outer one is current again after
  {
    std::exception_ptr inner = std::make_exception_ptr(2.0);
    try {
      throw 1;
    } catch (int) {
      bool got = false;
      try {
        std::rethrow_exception(inner);
      } catch (double d) {
        got = d == 2.0;
      }
      CHECK(got);
      int again = 0;
      try {
        throw;
      } catch (int i) {
        again = i;
      }
      CHECK(again == 1);
    }
  }
  return 0;
}
