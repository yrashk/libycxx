// [except.nested]/3: nested_exception() "calls current_exception() and stores the returned
// value"; copies are defaulted (copy the stored exception_ptr). /8: throw_with_nested throws
// "an exception of unspecified type that is publicly derived from both U and
// nested_exception and constructed from std::forward<T>(t)". /9: rethrow_if_nested performs
// dynamic_cast<const nested_exception*>(addressof(e)) for polymorphic E with an accessible
// unambiguous nested_exception base, and has no effect otherwise.
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>
#include "check.hpp"

struct Payload {
  int v;
  explicit Payload(int x) : v(x) {}
  Payload(const Payload&) = default;
  Payload(Payload&& o) noexcept : v(o.v) { o.v = -1; }
  virtual ~Payload() = default;
};
struct WithNested : std::runtime_error, std::nested_exception {
  WithNested() : std::runtime_error("with") {}
};
struct PrivateNested : private std::nested_exception {
  virtual ~PrivateNested() = default;
};
struct NestedA : std::nested_exception {};
struct NestedB : std::nested_exception {};
struct AmbiguousNested : NestedA, NestedB {};
struct NonPolyBase {
  int x = 0;
};
struct Overloaded {  // operator& must not be used (addressof)
  virtual ~Overloaded() = default;
  void operator&() const = delete;
};

int main() {
  // copies share the stored exception
  try {
    throw 3;
  } catch (...) {
    std::nested_exception n1;
    std::nested_exception n2 = n1;
    CHECK(n2.nested_ptr() == n1.nested_ptr());
    std::nested_exception n3;
    n3 = n1;
    CHECK(n3.nested_ptr() == n1.nested_ptr());
  }

  // the thrown object is constructed from std::forward<T>(t): an rvalue is moved from
  Payload p(4);
  try {
    try {
      throw 1;
    } catch (...) {
      std::throw_with_nested(std::move(p));
    }
  } catch (const Payload& got) {
    CHECK(got.v == 4);
    CHECK(dynamic_cast<const std::nested_exception*>(&got) != nullptr);
  }
  CHECK(p.v == -1);

  // an lvalue is copied
  Payload q(5);
  try {
    try {
      throw 1;
    } catch (...) {
      std::throw_with_nested(q);
    }
  } catch (const std::nested_exception& ne) {  // catchable as nested_exception (public base)
    CHECK(ne.nested_ptr() != nullptr);
    const Payload* pp = dynamic_cast<const Payload*>(&ne);
    CHECK(pp != nullptr && pp->v == 5);
  }
  CHECK(q.v == 5);

  // throw_with_nested outside a handler: nested_ptr() is null
  try {
    std::throw_with_nested(Payload(6));
  } catch (const std::nested_exception& ne) {
    CHECK(ne.nested_ptr() == nullptr);
  }

  // a type that is already a nested_exception is thrown as-is and keeps its own capture
  try {
    try {
      throw 7;
    } catch (...) {
      WithNested w;  // captures 7
      std::throw_with_nested(w);
    }
  } catch (const WithNested& w) {
    int inner = 0;
    try {
      std::rethrow_if_nested(w);
    } catch (int i) {
      inner = i;
    }
    CHECK(inner == 7);
  }

  // multi-level nesting
  int depth = 0;
  try {
    try {
      try {
        throw std::out_of_range("leaf");
      } catch (...) {
        std::throw_with_nested(std::runtime_error("mid"));
      }
    } catch (...) {
      std::throw_with_nested(std::logic_error("top"));
    }
  } catch (const std::logic_error& top) {
    ++depth;
    try {
      std::rethrow_if_nested(top);
    } catch (const std::runtime_error& mid) {
      ++depth;
      try {
        std::rethrow_if_nested(mid);
      } catch (const std::out_of_range& leaf) {
        ++depth;
        std::rethrow_if_nested(leaf);  // no nested exception: no effect
      }
    }
  }
  CHECK(depth == 3);

  // no effect: private or ambiguous base, non-polymorphic type
  try {
    throw 1;
  } catch (...) {
    PrivateNested pn;
    std::rethrow_if_nested(pn);
    AmbiguousNested an;
    std::rethrow_if_nested(an);
    NonPolyBase npb;
    std::rethrow_if_nested(npb);
    Overloaded ov;
    std::rethrow_if_nested(ov);
  }
  return 0;
}
