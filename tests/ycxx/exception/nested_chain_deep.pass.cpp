// [except.nested]: a long chain of nested exceptions built while unwinding a deep recursion.
// /3: nested_exception() "calls current_exception() and stores the returned value"; /8:
// throw_with_nested(t) throws "an exception of unspecified type that is publicly derived from
// both U and nested_exception" when U is a non-final class not derived from nested_exception,
// "otherwise std::forward<T>(t)" (a class already derived from nested_exception captured the
// exception being handled when it was constructed, so the chain continues through it); /9:
// rethrow_if_nested(e) has no effect when E is not polymorphic, otherwise rethrows through
// dynamic_cast<const nested_exception*>(addressof(e)). Each level also rethrows with `throw;`
// and through an exception_ptr ([propagation]/7-/10) before wrapping, and the chain is walked
// twice in two different ways.
// REQUIRES: exceptions
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include "check.hpp"

struct Val {  // not polymorphic, not final: wrapped
  int v;
};
struct OwnNested : std::exception, std::nested_exception {  // thrown as is
  int v;
  explicit OwnNested(int x) : v(x) {}
};
struct Fin final {  // final: thrown as is
  int v;
};

constexpr int depth = 60;

[[noreturn]] static void level(int i) {
  if (i == depth) throw Fin{i};
  try {
    try {
      level(i + 1);
    } catch (...) {
      if (i % 4 == 1) {
        std::exception_ptr p = std::current_exception();
        std::rethrow_exception(p);
      }
      throw;
    }
  } catch (...) {
    switch (i % 3) {
      case 0: std::throw_with_nested(std::runtime_error(std::to_string(i)));
      case 1: std::throw_with_nested(Val{i});
      default: std::throw_with_nested(OwnNested(i));
    }
  }
}

// kind * 1000 + level
static int identify(const std::nested_exception& ne) {
  if (auto r = dynamic_cast<const std::runtime_error*>(&ne)) return 0 * 1000 + std::stoi(r->what());
  if (auto v = dynamic_cast<const Val*>(&ne)) return 1000 + v->v;
  if (auto o = dynamic_cast<const OwnNested*>(&ne)) return 2000 + o->v;
  return -1;
}

static std::vector<int> walk_iterative(std::exception_ptr p) {
  std::vector<int> out;
  while (p) {
    std::exception_ptr next;
    try {
      std::rethrow_exception(p);
    } catch (const std::nested_exception& ne) {
      out.push_back(identify(ne));
      next = ne.nested_ptr();
    } catch (const Fin& f) {
      out.push_back(3000 + f.v);
    } catch (...) {
      out.push_back(-2);
    }
    p = next;
  }
  return out;
}

static void walk_recursive(std::vector<int>& out) {
  try {
    throw;
  } catch (const std::runtime_error& e) {
    out.push_back(std::stoi(e.what()));
    try {
      std::rethrow_if_nested(e);
      out.push_back(-3);  // not reached: e has a nested exception
    } catch (...) {
      walk_recursive(out);
    }
  } catch (const Val& v) {
    out.push_back(1000 + v.v);
    std::rethrow_if_nested(v);  // Val is not polymorphic: no effect
    try {
      throw;
    } catch (const std::nested_exception& ne) {
      try {
        ne.rethrow_nested();
      } catch (...) {
        walk_recursive(out);
      }
    }
  } catch (const OwnNested& o) {
    out.push_back(2000 + o.v);
    try {
      std::rethrow_if_nested(o);
    } catch (...) {
      walk_recursive(out);
    }
  } catch (const Fin& f) {
    out.push_back(3000 + f.v);
  }
}

int main() {
  std::vector<int> expected;
  for (int i = 0; i < depth; ++i) expected.push_back((i % 3) * 1000 + i);
  expected.push_back(3000 + depth);

  for (int round = 0; round < 3; ++round) {
    std::exception_ptr top;
    try {
      level(0);
    } catch (...) {
      top = std::current_exception();
      std::vector<int> rec;
      walk_recursive(rec);
      CHECK(rec == expected);
      CHECK(std::uncaught_exceptions() == 0);
    }
    CHECK(top != nullptr);
    CHECK(walk_iterative(top) == expected);
    // the chain survives the handlers that built it; walk it again
    CHECK(walk_iterative(top) == expected);
  }
  return 0;
}
