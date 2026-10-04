// [stmt.dcl]/4: "An object associated with a block variable with static or thread storage
// duration will be destroyed if and only if it was constructed." [basic.start.term]/1:
// destructors of such objects run on return from main, and /3: a function registered with
// atexit before an object's construction completes is called after its destruction. The
// check function, registered first, therefore runs after every constructed block static is
// destroyed.
#include <cstdlib>
#include "check.hpp"

static int destroyed_a = 0, destroyed_b = 0, destroyed_never = 0, destroyed_failed = 0;
static int dtor_order[4];
static int nd = 0;

struct Tracked {
  int* flag;
  int id;
  Tracked(int* f, int i, bool fail = false) : flag(f), id(i) {
    if (fail) throw 0;
  }
  ~Tracked() {
    ++*flag;
    dtor_order[nd++] = id;
  }
};

static void use_a() { static Tracked a(&destroyed_a, 1); }
static void use_b() { static Tracked b(&destroyed_b, 2); }
[[maybe_unused]] static void use_never() { static Tracked c(&destroyed_never, 3); }
static void use_failed() { static Tracked d(&destroyed_failed, 4, true); }

static void check_at_exit() {
  // a constructed before b: destroyed after b
  if (destroyed_a != 1 || destroyed_b != 1 || destroyed_never != 0 || destroyed_failed != 0 ||
      nd != 2 || dtor_order[0] != 2 || dtor_order[1] != 1)
    std::_Exit(1);
  std::_Exit(0);
}

int main() {
  CHECK(std::atexit(check_at_exit) == 0);
  use_a();
  use_b();
  use_a();
  try {
    use_failed();
  } catch (int) {
  }
  CHECK(destroyed_a == 0 && destroyed_b == 0);
  return 0;
}
