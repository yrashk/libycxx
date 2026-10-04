// [propagation]/14: exception_ptr_cast<E>(p) "Returns: An optional containing a reference to
// the exception object referred to by p, if p is not null and a handler of type const E& would
// be a match ([except.handle]) for that exception object. Otherwise, nullopt."
// [except.handle]/3: a handler of type cv T& matches an exception object of type E if T and E
// are the same type (ignoring top-level cv), or "T is an unambiguous public base class of E".
// So private, protected and ambiguous bases do not match; the exception object created by
// throw_with_nested ([except.nested]/8) is "publicly derived from both U and
// nested_exception"; copies of an exception_ptr refer to the same object ([propagation]/3).
#include <exception>
#include <functional>
#include <optional>
#include <stdexcept>
#include "check.hpp"

struct B {
  int v = 3;
};
struct PrivateD : private B {};
struct ProtectedD : protected B {};
struct L : B {};
struct R : B {};
struct Ambiguous : L, R {};
struct Poly {
  int k = 5;
  virtual ~Poly() = default;
};

int main() {
  // exception_ptr_cast(const exception_ptr&&) is deleted, so name the exception_ptrs
  const std::exception_ptr pb = std::make_exception_ptr(B{});
  const std::exception_ptr ppriv = std::make_exception_ptr(PrivateD{});
  const std::exception_ptr pprot = std::make_exception_ptr(ProtectedD{});
  const std::exception_ptr pamb = std::make_exception_ptr(Ambiguous{});
  CHECK(std::exception_ptr_cast<B>(pb).has_value());
  CHECK(!std::exception_ptr_cast<B>(ppriv).has_value());
  CHECK(!std::exception_ptr_cast<B>(pprot).has_value());
  CHECK(!std::exception_ptr_cast<B>(pamb).has_value());
  CHECK(std::exception_ptr_cast<L>(pamb).has_value());

  // standard exception hierarchy
  std::exception_ptr bc = std::make_exception_ptr(std::bad_function_call());
  CHECK(std::exception_ptr_cast<std::exception>(bc).has_value());
  CHECK(std::exception_ptr_cast<std::bad_function_call>(bc).has_value());
  CHECK(!std::exception_ptr_cast<std::runtime_error>(bc).has_value());

  // the object thrown by throw_with_nested
  std::exception_ptr nested;
  try {
    try {
      throw 1;
    } catch (...) {
      std::throw_with_nested(Poly());
    }
  } catch (...) {
    nested = std::current_exception();
  }
  auto as_poly = std::exception_ptr_cast<Poly>(nested);
  auto as_nested = std::exception_ptr_cast<std::nested_exception>(nested);
  CHECK(as_poly.has_value() && as_poly->k == 5);
  CHECK(as_nested.has_value() && as_nested->nested_ptr() != nullptr);
  const std::exception_ptr inner_ptr = as_nested->nested_ptr();
  auto inner = std::exception_ptr_cast<int>(inner_ptr);
  CHECK(inner.has_value() && *inner == 1);

  // copies refer to the same exception object
  std::exception_ptr copy = nested;
  CHECK(&*std::exception_ptr_cast<Poly>(copy) == &*as_poly);

  // inside a handler: current_exception() refers to the handled exception or a copy of it;
  // either way the value is that of the thrown object
  try {
    throw std::out_of_range("range");
  } catch (...) {
    const std::exception_ptr cur = std::current_exception();
    auto e = std::exception_ptr_cast<std::logic_error>(cur);
    CHECK(e.has_value() && e->what()[0] == 'r');
  }
  return 0;
}
