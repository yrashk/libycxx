// [expr.throw]/2: "The exception object is copy-initialized from the (possibly converted)
// operand." From a prvalue of the same type no copy or move constructor runs ([dcl.init]
// guaranteed elision). [class.copy.elision]/3 (Example 2): `throw t;` for a local t not
// enclosing a try-block uses the move constructor (or elides); Example 3: an object declared
// outside the innermost try-block "does not move". [Note 2]: "There cannot be a move from the
// exception object because it is always an lvalue." Catching by reference and rethrowing make
// no copies.
#include "check.hpp"

static int copies = 0, moves = 0, live = 0;

struct Probe {
  int v;
  explicit Probe(int i) : v(i) { ++live; }
  Probe(const Probe& o) : v(o.v) {
    ++copies;
    ++live;
  }
  Probe(Probe&& o) : v(o.v) {
    o.v = -1;
    ++moves;
    ++live;
  }
  ~Probe() { --live; }
};

static Probe global(5);

static void throw_local() {
  Probe p(2);
  throw p;  // implicitly movable
}

int main() {
  try {
    throw Probe(1);
  } catch (Probe& p) {
    CHECK(p.v == 1);
    CHECK(copies == 0 && moves == 0);
  }

  copies = moves = 0;
  try {
    throw_local();
  } catch (const Probe& p) {
    CHECK(p.v == 2);
    CHECK(copies == 0);
    CHECK(moves <= 1);
  }

  copies = moves = 0;
  try {
    throw global;  // static storage duration: copied
  } catch (Probe& p) {
    CHECK(copies == 1 && moves == 0);
    CHECK(p.v == 5);
    CHECK(&p != &global);
    p.v = 6;
  }
  CHECK(global.v == 5);

  // object declared outside the innermost try-block: copied, not moved
  copies = moves = 0;
  Probe outer(7);
  try {
    throw outer;
  } catch (Probe& p) {
    CHECK(p.v == 7);
    CHECK(copies == 1 && moves == 0);
  }
  CHECK(outer.v == 7);

  // catching by value copies at most once (and never moves)
  copies = moves = 0;
  try {
    throw Probe(8);
  } catch (Probe p) {
    CHECK(p.v == 8);
    CHECK(copies <= 1 && moves == 0);
  }

  // rethrow copies nothing
  copies = moves = 0;
  try {
    try {
      throw Probe(9);
    } catch (Probe&) {
      throw;
    }
  } catch (Probe& p) {
    CHECK(p.v == 9 && copies == 0 && moves == 0);
  }

  CHECK(live == 2);  // global and outer
  return 0;
}
