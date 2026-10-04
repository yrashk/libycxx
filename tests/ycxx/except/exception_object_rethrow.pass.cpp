// [expr.throw]/3: "A throw-expression with no operand rethrows the currently handled
// exception. ... the exception is reactivated with the existing exception object; no new
// exception object is created." [except.throw]/4: "If a handler exits by rethrowing, control
// is passed to another handler for the same exception object." The object is destroyed only
// when the last handler exits other than by rethrowing.
#include "check.hpp"

static int live = 0, copies = 0, dtors = 0;

struct Probe {
  int v;
  explicit Probe(int i) : v(i) { ++live; }
  Probe(const Probe& o) : v(o.v) {
    ++live;
    ++copies;
  }
  ~Probe() {
    --live;
    ++dtors;
  }
};

struct Base {
  int tag = 1;
  virtual ~Base() = default;
};
struct Derived : Base {
  int extra = 2;
};

static const void* first = nullptr;

static void level2() {
  try {
    throw Probe(10);
  } catch (Probe& p) {
    first = &p;
    p.v = 11;
    throw;
  }
}

static void level1() {
  try {
    level2();
  } catch (Probe& p) {
    CHECK(&p == first);
    CHECK(p.v == 11);
    CHECK(dtors == 0);
    p.v = 12;
    throw;
  }
}

int main() {
  try {
    level1();
  } catch (const Probe& p) {
    CHECK(&p == first);
    CHECK(p.v == 12);
    CHECK(live == 1);
    CHECK(copies == 0);
  }
  CHECK(live == 0 && dtors == 1);

  // rethrow from a base-class handler keeps the dynamic type
  int which = 0;
  try {
    try {
      throw Derived();
    } catch (Base& b) {
      b.tag = 5;
      throw;
    }
  } catch (Derived& d) {
    which = d.tag + d.extra;
  }
  CHECK(which == 7);

  // rethrow from catch(...) for a non-class type
  which = 0;
  try {
    try {
      throw 41;
    } catch (...) {
      throw;
    }
  } catch (int i) {
    which = i + 1;
  }
  CHECK(which == 42);

  // rethrow from a by-value handler rethrows the original object, not the copy
  copies = 0;
  try {
    try {
      throw Probe(20);
    } catch (Probe p) {
      p.v = 99;  // changes the handler's variable only
      throw;
    }
  } catch (Probe& p) {
    CHECK(p.v == 20);
  }
  CHECK(live == 0);
  return 0;
}
