// [except.ctor]/3: "If the initialization of an object other than by delegating constructor
// is terminated by an exception, the destructor is invoked for each of the object's subobjects
// that were known to be initialized by the object's initialization and whose initialization
// has completed. ... The subobjects are destroyed in the reverse order of the completion of
// their construction." The object's own destructor does not run. [Note 2]: this includes
// virtual base class subobjects for a complete object.
#include "check.hpp"

static int log_[32];
static int n = 0;
static int fail_at = -1;

struct Part {
  int id;
  explicit Part(int i) : id(i) {
    if (i == fail_at) throw i;
  }
  ~Part() { log_[n++] = id; }
};

struct VB {
  Part p{1};
};
struct B1 : virtual VB {
  Part p{2};
};
struct B2 {
  Part p{3};
};
struct Whole : B1, B2 {
  Part m1{4};
  Part m2{5};
  Part m3{6};
  Whole() {
    if (fail_at == 7) throw 7;
  }
  ~Whole() { log_[n++] = 100; }
};

int main() {
  // construction order: VB(1), B1(2), B2(3), m1(4), m2(5), m3(6), body(7)
  for (int f = 1; f <= 7; ++f) {
    fail_at = f;
    n = 0;
    int caught = 0;
    try {
      Whole w;
      (void)w;
    } catch (int i) {
      caught = i;
    }
    CHECK(caught == f);
    CHECK(n == f - 1);  // every completed subobject destroyed, the Whole destructor not run
    for (int i = 0; i < n; ++i) CHECK(log_[i] == f - 1 - i);  // reverse order
  }

  // aggregate initialization: initialized elements destroyed in reverse
  struct Agg {
    Part a, b, c;
  };
  fail_at = 12;
  n = 0;
  try {
    Agg g{Part(11), Part(12), Part(13)};
    (void)g;
  } catch (int) {
  }
  CHECK(n == 1 && log_[0] == 11);
  return 0;
}
