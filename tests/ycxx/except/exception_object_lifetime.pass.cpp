// [except.throw]/4: "The points of potential destruction for the exception object are: when
// an active handler for the exception exits by any means other than rethrowing, immediately
// after the destruction of the object (if any) declared in the exception-declaration in the
// handler; ..." [except.handle]/15: "The lifetime of the variable ends when the handler
// exits, after the destruction of any objects with automatic storage duration initialized
// within the handler." So: locals of the handler, then the handler variable, then the
// exception object.
#include "check.hpp"

static int log_[16];
static int nlog = 0;
static int live = 0;

struct Probe {
  int id;
  explicit Probe(int i) : id(i) { ++live; }
  Probe(const Probe& o) : id(o.id + 100) { ++live; }
  ~Probe() {
    --live;
    log_[nlog++] = id;
  }
};

int main() {
  try {
    throw Probe(1);
  } catch (Probe p) {  // copy id 101 (unless elided, see below)
    CHECK(live >= 1);
    Probe local(2);
    (void)local;
  }
  CHECK(live == 0);
  // local(2) first, then handler variable, then exception object
  CHECK(log_[0] == 2);
  if (nlog == 3) {
    CHECK(log_[1] == 101);
    CHECK(log_[2] == 1);
  } else {
    CHECK(nlog == 2);  // copy elided ([class.copy.elision]/1.4): the variable is the object
    CHECK(log_[1] == 1);
  }

  // by reference: the exception object survives until the handler exits
  nlog = 0;
  try {
    throw Probe(3);
  } catch (Probe& p) {
    CHECK(live == 1 && nlog == 0 && p.id == 3);
    {
      Probe inner(4);
      (void)inner;
    }
    CHECK(nlog == 1 && log_[0] == 4);
    CHECK(p.id == 3);  // still alive
  }
  CHECK(live == 0 && nlog == 2 && log_[1] == 3);

  // catch(...) behaves the same
  nlog = 0;
  try {
    throw Probe(5);
  } catch (...) {
    CHECK(live == 1 && nlog == 0);
  }
  CHECK(live == 0 && nlog == 1 && log_[0] == 5);

  // a handler exited by return/break-like transfer also ends the exception
  nlog = 0;
  for (int k = 0; k < 2; ++k) {
    try {
      throw Probe(6);
    } catch (const Probe&) {
      if (k == 0) continue;
      break;
    }
  }
  CHECK(live == 0 && nlog == 2 && log_[0] == 6 && log_[1] == 6);

  auto f = []() -> int {
    try {
      throw Probe(7);
    } catch (const Probe& p) {
      return p.id;  // value computed before the exception object is destroyed
    }
  };
  nlog = 0;
  CHECK(f() == 7);
  CHECK(live == 0 && nlog == 1 && log_[0] == 7);
  return 0;
}
