// [except.handle]/4: "The handlers for a try block are tried in order of appearance."
// /5: "A ... in a handler's exception-declaration specifies a match for any exception."
// /6: "If no match is found among the handlers for a try block, the search for a matching
// handler continues in a dynamically surrounding try block of the same thread."
// REQUIRES: exceptions
#include "check.hpp"

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

static int depth3(int k) {
  try {
    if (k == 0) throw Derived();
    if (k == 1) throw 42;
    throw 'c';
  } catch (double) {
    return -1;
  }
}

static int depth2(int k) {
  try {
    return depth3(k);
  } catch (const char*) {
    return -2;
  }
}

int main() {
  int which = 0;
  // the base handler appears first and wins, even though a better match follows
  try {
    throw Derived();
  } catch (Base&) {
    which = 1;
  } catch (Derived&) {
    which = 2;
  } catch (...) {
    which = 3;
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw Derived();
  } catch (int) {
    which = 1;
  } catch (Derived&) {
    which = 2;
  } catch (Base&) {
    which = 3;
  }
  CHECK(which == 2);

  which = 0;
  try {
    throw 3.0f;
  } catch (int) {
    which = 1;
  } catch (...) {
    which = 2;
  }
  CHECK(which == 2);

  // search continues outward through intervening frames whose handlers do not match
  for (int k = 0; k < 3; ++k) {
    which = 0;
    try {
      depth2(k);
      which = -1;
    } catch (Base&) {
      which = 10;
    } catch (int i) {
      which = i;
    } catch (...) {
      which = 99;
    }
    CHECK(which == (k == 0 ? 10 : k == 1 ? 42 : 99));
  }

  // the nearest handler (most recently entered try block) is chosen
  which = 0;
  try {
    try {
      throw 1;
    } catch (int) {
      which = 1;
    }
  } catch (int) {
    which = 2;
  }
  CHECK(which == 1);
  return 0;
}
