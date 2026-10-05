// [except.ctor]/2: "Each object with automatic storage duration is destroyed if it has been
// constructed, but not yet destroyed, since the try block was entered. ... The objects are
// destroyed in the reverse order of the completion of their construction." This spans
// several stack frames; objects declared after the throw point are not destroyed, nor are
// objects outside the try block.
// REQUIRES: exceptions
#include "check.hpp"

static int log_[32];
static int n = 0;

struct Obj {
  int id;
  explicit Obj(int i) : id(i) {}
  ~Obj() { log_[n++] = id; }
};

static void inner(int depth) {
  Obj a(depth * 10 + 1);
  if (depth == 3) {
    Obj b(depth * 10 + 2);
    throw depth;
  }
  Obj c(depth * 10 + 3);
  inner(depth + 1);
  Obj never(999);
}

int main() {
  {
    Obj outside(0);
    try {
      Obj t(1);
      inner(1);
    } catch (int d) {
      CHECK(d == 3);
      // everything constructed inside the try block is already destroyed
      int expect[] = {32, 31, 23, 21, 13, 11, 1};
      CHECK(n == 7);
      for (int i = 0; i < 7; ++i) CHECK(log_[i] == expect[i]);
    }
    CHECK(n == 7);
  }
  CHECK(n == 8 && log_[7] == 0);

  // temporaries of the full-expression containing the call are destroyed too
  n = 0;
  auto thrower = [](const Obj&, const Obj&) { throw 1; };
  try {
    thrower(Obj(100), Obj(200));
  } catch (int) {
    CHECK(n == 2);
  }
  CHECK(n == 2);
  CHECK((log_[0] == 100 && log_[1] == 200) || (log_[0] == 200 && log_[1] == 100));

  // a lifetime-extended temporary bound to a reference is destroyed as the reference's
  // block is left
  n = 0;
  try {
    const Obj& r = Obj(300);
    (void)r;
    throw 2;
  } catch (int) {
    CHECK(n == 1 && log_[0] == 300);
  }
  return 0;
}
