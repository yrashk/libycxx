// [propagation]/9: current_exception() "Returns: An exception_ptr object that refers to the
// currently handled exception or a copy of the currently handled exception ... The referenced
// object shall remain valid at least as long as there is an exception_ptr object that refers
// to it." /3: non-null exception_ptrs compare equal iff they refer to the same exception.
// /11: rethrow_exception throws the referenced exception object or a copy.
// REQUIRES: exceptions
#include <exception>
#include <stdexcept>
#include <utility>
#include "check.hpp"

struct Tracked {
  static inline int live = 0;
  int v;
  explicit Tracked(int x) : v(x) { ++live; }
  Tracked(const Tracked& o) : v(o.v) { ++live; }
  ~Tracked() { --live; }
};

int main() {
  std::exception_ptr p;
  try {
    throw Tracked(7);
  } catch (...) {
    p = std::current_exception();
  }
  CHECK(Tracked::live >= 1);  // kept alive by p after the handler exits
  {
    std::exception_ptr q = p;
    std::exception_ptr r = std::move(q);
    CHECK(r == p);
  }
  int seen = 0;
  try {
    std::rethrow_exception(p);
  } catch (const Tracked& t) {
    seen = t.v;
  }
  CHECK(seen == 7);
  // rethrow again: still valid
  try {
    std::rethrow_exception(p);
  } catch (const Tracked& t) {
    seen = t.v + 1;
  }
  CHECK(seen == 8);
  p = nullptr;
  CHECK(Tracked::live == 0);  // the last reference released the exception

  // nested handlers: current_exception refers to the innermost handled exception
  try {
    throw 1;
  } catch (...) {
    std::exception_ptr outer = std::current_exception();
    try {
      throw 2.0;
    } catch (...) {
      std::exception_ptr inner = std::current_exception();
      CHECK(inner != outer);
      bool is_double = false;
      try {
        std::rethrow_exception(inner);
      } catch (double) {
        is_double = true;
      }
      CHECK(is_double);
    }
    // back in the outer handler
    bool is_int = false;
    try {
      std::rethrow_exception(std::current_exception());
    } catch (int i) {
      is_int = i == 1;
    }
    CHECK(is_int);
  }
  CHECK(std::current_exception() == nullptr);

  // current_exception inside a handler entered by rethrow ("throw;")
  std::exception_ptr a, b;
  try {
    try {
      throw std::logic_error("x");
    } catch (...) {
      a = std::current_exception();
      throw;
    }
  } catch (const std::logic_error&) {
    b = std::current_exception();
  }
  CHECK(a != nullptr && b != nullptr);
  bool ok = false;
  try {
    std::rethrow_exception(b);
  } catch (const std::logic_error& e) {
    ok = e.what()[0] == 'x';
  }
  CHECK(ok);
  return 0;
}
