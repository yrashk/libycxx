// [propagation]/9: current_exception() returns "an exception_ptr object that refers to the
// currently handled exception or a copy of the currently handled exception". [except.handle]/10:
// "The exception with the most recently activated handler that is still active is called the
// currently handled exception." So inside a nested handler it is the inner exception, and
// after that handler exits it is the outer one again; after both exit, there is none.
// [except.nested]/3: nested_exception() captures current_exception() at construction.
// REQUIRES: exceptions
#include <exception>
#include "check.hpp"

template <class T>
static int value_of(const std::exception_ptr& p) {
  try {
    std::rethrow_exception(p);
  } catch (T v) {
    return static_cast<int>(v);
  } catch (...) {
    return -1;
  }
}

struct Wrapped : std::nested_exception {};

int main() {
  CHECK(std::current_exception() == nullptr);
  try {
    throw 1;
  } catch (int) {
    std::exception_ptr outer = std::current_exception();
    CHECK(value_of<int>(outer) == 1);
    try {
      throw 2L;
    } catch (long) {
      std::exception_ptr inner = std::current_exception();
      CHECK(value_of<long>(inner) == 2);
      CHECK(inner != outer);
      Wrapped w;  // captures the inner exception
      CHECK(value_of<long>(w.nested_ptr()) == 2);
    }
    CHECK(value_of<int>(std::current_exception()) == 1);
    Wrapped w2;  // captures the outer one again
    CHECK(value_of<int>(w2.nested_ptr()) == 1);
  }
  CHECK(std::current_exception() == nullptr);

  // during unwinding (no handler active), there is no currently handled exception
  struct Probe {
    bool* null_seen;
    ~Probe() { *null_seen = std::current_exception() == nullptr; }
  };
  bool null_seen = false;
  try {
    Probe p{&null_seen};
    throw 3;
  } catch (int) {
  }
  CHECK(null_seen);
  return 0;
}
