// [stmt.dcl]/3: "If the initialization exits by throwing an exception, the initialization is
// not complete, so it will be tried again the next time control enters the declaration."
// /4: "An object associated with a block variable with static or thread storage duration
// will be destroyed if and only if it was constructed."
// REQUIRES: exceptions
#include "check.hpp"

static int attempts = 0;
static int ctor_done = 0;

struct Flaky {
  int v;
  explicit Flaky(int fail_until) : v(0) {
    ++attempts;
    if (attempts < fail_until) throw attempts;
    v = attempts;
    ++ctor_done;
  }
};

static int get() {
  static Flaky f(3);
  return f.v;
}

static int counter = 0;
static int next_value() {
  if (++counter < 2) throw counter;
  return counter * 10;
}
static int get_int() {
  static int x = next_value();
  return x;
}

int main() {
  int failures = 0;
  for (int i = 0; i < 2; ++i) {
    try {
      (void)get();
    } catch (int n) {
      CHECK(n == i + 1);
      ++failures;
    }
  }
  CHECK(failures == 2);
  CHECK(get() == 3);
  CHECK(get() == 3);
  CHECK(attempts == 3 && ctor_done == 1);

  // non-class type with a throwing initializer
  bool threw = false;
  try {
    (void)get_int();
  } catch (int) {
    threw = true;
  }
  CHECK(threw);
  CHECK(get_int() == 20);
  CHECK(get_int() == 20 && counter == 2);
  return 0;
}
