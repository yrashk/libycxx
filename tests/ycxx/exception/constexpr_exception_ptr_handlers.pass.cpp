// [propagation] during constant evaluation (P3068): /8 all members of exception_ptr are
// constexpr; /3 two non-null exception_ptrs compare equal iff they refer to the same exception;
// [exception.syn] make_exception_ptr, rethrow_exception and exception_ptr_cast are constexpr.
// /11-12: rethrow_exception throws the referenced exception object (or a copy), which a handler
// for a public base class catches ([except.handle]/3); /9 the referenced object stays valid
// as long as an exception_ptr refers to it, also after the handler that created it has exited;
// exception_ptr_cast<E> returns the object if a handler of type const E& would match it, with
// [propagation]/13's example: a base class E matches a derived object.
// make_exception_ptr(e) inside a handler makes an exception_ptr to a new exception, not to the
// handled one ([propagation]/12: as if `try { throw e; } catch(...) { return current-exception(); }`).
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <exception>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include "check.hpp"

constexpr bool same_text(const char* a, const char* b) { return std::string_view(a) == std::string_view(b); }

constexpr std::exception_ptr make_out_of_range() {
  // The temporary out_of_range and the handler inside make_exception_ptr are gone on return.
  return std::make_exception_ptr(std::out_of_range("index 7"));
}

constexpr bool base_class_handlers() {
  std::exception_ptr p = make_out_of_range();
  if (!p) return false;
  auto as_logic = std::exception_ptr_cast<std::logic_error>(p);
  if (!as_logic || !same_text(as_logic->what(), "index 7")) return false;
  auto as_exc = std::exception_ptr_cast<std::exception>(p);
  if (!as_exc || !same_text(as_exc->what(), "index 7")) return false;
  if (std::exception_ptr_cast<std::runtime_error>(p)) return false;
  if (std::exception_ptr_cast<std::length_error>(p)) return false;
  int hits = 0;
  try {
    std::rethrow_exception(p);
  } catch (const std::runtime_error&) {
    return false;
  } catch (const std::logic_error& e) {
    hits += same_text(e.what(), "index 7");
  }
  try {
    std::rethrow_exception(p); // a second rethrow of the same exception
  } catch (const std::exception& e) {
    hits += same_text(e.what(), "index 7");
  }
  return hits == 2;
}
static_assert(base_class_handlers());

constexpr bool identity_and_copies() {
  std::exception_ptr a = std::make_exception_ptr(1);
  std::exception_ptr b = std::make_exception_ptr(1);
  if (a == b) return false; // two exceptions
  std::exception_ptr c = a;
  if (!(c == a) || c != a) return false;
  std::exception_ptr d;
  d = c;
  if (d != a) return false;
  const std::exception_ptr second = b;
  swap(d, b);
  if (d != second || b != a) return false;
  d.swap(b);
  if (d != a || b != second) return false;
  c = std::exception_ptr(); // drop references in every order
  a = nullptr;
  if (a || !d) return false;
  return *std::exception_ptr_cast<int>(d) == 1;
}
static_assert(identity_and_copies());

struct Counted {
  int* live;
  constexpr explicit Counted(int* l) : live(l) { ++*live; }
  constexpr Counted(const Counted& o) : live(o.live) { ++*live; }
  constexpr ~Counted() { --*live; }
};

// The exception object lives while an exception_ptr refers to it, and no longer: every object
// is destroyed by the end of the evaluation (a leak would make it non-constant).
constexpr bool lifetime() {
  int live = 0;
  {
    std::exception_ptr p = std::make_exception_ptr(Counted(&live));
    if (live < 1) return false;
    std::exception_ptr q = p;
    p = nullptr;
    if (live < 1) return false; // q still refers to it
    try {
      std::rethrow_exception(q);
    } catch (const Counted& c) {
      if (c.live != &live) return false;
    }
  }
  return live == 0;
}
static_assert(lifetime());

// make_exception_ptr within a handler refers to a new exception; rethrowing it does not end the
// handled one, which `throw;` still rethrows afterwards.
constexpr bool inside_a_handler() {
  int result = 0;
  try {
    try {
      throw std::runtime_error("outer");
    } catch (const std::runtime_error&) {
      std::exception_ptr inner = std::make_exception_ptr(std::length_error("inner"));
      try {
        std::rethrow_exception(inner);
      } catch (const std::length_error& e) {
        result += same_text(e.what(), "inner");
      }
      throw;
    }
  } catch (const std::runtime_error& e) {
    result += same_text(e.what(), "outer");
  }
  return result == 2;
}
static_assert(inside_a_handler());

// An exception_ptr passed through value parameters and returned keeps the exception.
constexpr std::exception_ptr pass_through(std::exception_ptr p) { return p; }
constexpr bool by_value() {
  std::exception_ptr p = pass_through(std::make_exception_ptr(std::invalid_argument("arg")));
  std::exception_ptr q = pass_through(p);
  auto e = std::exception_ptr_cast<std::invalid_argument>(q);
  return p == q && e && same_text(e->what(), "arg");
}
static_assert(by_value());

int main() {
  CHECK(base_class_handlers());
  CHECK(identity_and_copies());
  CHECK(lifetime());
  CHECK(inside_a_handler());
  CHECK(by_value());
  return 0;
}
