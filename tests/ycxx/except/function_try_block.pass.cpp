// [except.handle]/14: "The currently handled exception is rethrown if control reaches the end
// of a handler of the function-try-block of a constructor or destructor. Otherwise, flowing
// off the end of the compound-statement of a handler of a function-try-block is equivalent to
// flowing off the end of the compound-statement of that function." [except.ctor]/3: the
// subobjects are destroyed before the handler of the constructor's function-try-block is
// entered. A function-try-block catches exceptions from the ctor-initializer too.
#include "check.hpp"

static int member_dtors = 0, seen_dtors_in_handler = -1;

struct Member {
  Member(int v) {
    if (v) throw v;
  }
  ~Member() { ++member_dtors; }
};

struct Ctor {
  Member a;
  Member b;
  Ctor(int v) try : a(0), b(v) {
    if (v == 0) throw 'c';
  } catch (int) {
    seen_dtors_in_handler = member_dtors;  // a already destroyed
  } catch (char) {
    seen_dtors_in_handler = member_dtors;  // a and b destroyed
    throw 2.5;  // replaces the exception
  }
};

static int dtor_handler_ran = 0;
struct Dtor {
  ~Dtor() noexcept(false) try { throw 7; } catch (int) {
    ++dtor_handler_ran;
  }  // rethrown here
};

static int plain(int v) try {
  if (v) throw v;
  return 0;
} catch (int i) {
  return i * 2;
}

static int void_runs = 0;
static void plain_void() try { throw 1; } catch (int) {
  ++void_runs;
}  // flowing off: returns normally

int main() {
  int which = 0;
  try {
    Ctor c(9);
  } catch (int i) {
    which = i;  // automatically rethrown
  }
  CHECK(which == 9);
  CHECK(seen_dtors_in_handler == 1);

  member_dtors = 0;
  double d = 0;
  try {
    Ctor c(0);
  } catch (double x) {
    d = x;
  }
  CHECK(d == 2.5);
  CHECK(seen_dtors_in_handler == 2);

  which = 0;
  try {
    Dtor x;
  } catch (int i) {
    which = i;
  }
  CHECK(which == 7 && dtor_handler_ran == 1);

  CHECK(plain(0) == 0);
  CHECK(plain(21) == 42);
  plain_void();
  CHECK(void_runs == 1);
  return 0;
}
