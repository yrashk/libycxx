// [std.exceptions], [stdexcept.syn]: logic_error, domain_error, invalid_argument, length_error,
// out_of_range, runtime_error, range_error, overflow_error, underflow_error have
// `constexpr explicit E(const string& what_arg)` and `constexpr explicit E(const char*)` with
// postconditions strcmp(what(), what_arg.c_str()) == 0 / strcmp(what(), what_arg) == 0
// ([logic.error]/2-3, [domain.error]/2-3, [invalid.argument]/2-3, [length.error]/2-3,
// [out.of.range]/2-3, [runtime.error]/2-3, [range.error]/2-3, [overflow.error]/2-3,
// [underflow.error]/2-3) (P3068, P3378).
// [exception]/2: each such class has a (non-throwing) copy constructor and copy assignment
// operator with strcmp(lhs.what(), rhs.what()) == 0 for a copy; [exception] declares
// exception's copy operations, destructor and what() constexpr, and the derived classes' copy
// operations are not described separately ([functions.within.classes]), so they behave as the
// implicitly generated ones (constexpr), making copies usable in constant evaluation as P3378
// intends. [exception]/5: what() during constant evaluation is in the ordinary literal encoding.
// This test does not throw, so it is meaningful with Clang too.
#include <stdexcept>
#include <exception>
#include <string>
#include <string_view>
#include <type_traits>
#include "check.hpp"

template <class E, class Base>
constexpr bool test_one() {
  using sv = std::string_view;
  const char* lit = "a literal what_arg";
  E a(lit);
  if (sv(a.what()) != sv(lit)) return false;

  E b("");
  if (sv(b.what()) != "") return false;

  // from a string, longer than any small buffer; the argument may go away afterwards
  E* p = nullptr;
  {
    std::string s = "a string what_arg that is longer than any small-string buffer would be";
    p = new E(s);
    if (sv(p->what()) != sv(s)) return false;
    s[0] = 'X';
    s.clear();
  }
  if (sv(p->what()) != "a string what_arg that is longer than any small-string buffer would be") return false;

  // copy construction
  E c = *p;
  delete p;
  if (sv(c.what()) != "a string what_arg that is longer than any small-string buffer would be") return false;
  E d(a);
  if (sv(d.what()) != sv(lit)) return false;

  // copy assignment, both ways, and self-assignment
  d = c;
  if (sv(d.what()) != sv(c.what())) return false;
  c = a;
  if (sv(c.what()) != sv(lit)) return false;
  if (sv(d.what()) != "a string what_arg that is longer than any small-string buffer would be") return false;
  E& self = c;
  c = self;
  if (sv(c.what()) != sv(lit)) return false;

  // what() is virtual: called through the base classes
  const Base& rb = d;
  const std::exception& re = d;
  if (sv(rb.what()) != sv(d.what())) return false;
  if (sv(re.what()) != sv(d.what())) return false;

  // destroyed through a pointer to std::exception (constexpr virtual destructor)
  std::exception* q = new E("deleted through the base");
  if (sv(q->what()) != "deleted through the base") return false;
  delete q;
  return true;
}

template <class E, class Base>
constexpr bool test_types() {
  static_assert(std::is_base_of_v<Base, E>);
  static_assert(std::is_convertible_v<const E*, const std::exception*>);  // public, unambiguous
  static_assert(std::is_nothrow_copy_constructible_v<E>);
  static_assert(std::is_nothrow_copy_assignable_v<E>);
  static_assert(std::is_constructible_v<E, const char*>);
  static_assert(std::is_constructible_v<E, const std::string&>);
  static_assert(!std::is_convertible_v<const char*, E>);         // explicit
  static_assert(!std::is_convertible_v<const std::string&, E>);  // explicit
  static_assert(std::has_virtual_destructor_v<E>);
  static_assert(noexcept(std::declval<const E&>().what()));
  static_assert(std::is_same_v<decltype(std::declval<const E&>().what()), const char*>);
  return test_one<E, Base>();
}

static_assert(test_types<std::logic_error, std::exception>());
static_assert(test_types<std::domain_error, std::logic_error>());
static_assert(test_types<std::invalid_argument, std::logic_error>());
static_assert(test_types<std::length_error, std::logic_error>());
static_assert(test_types<std::out_of_range, std::logic_error>());
static_assert(test_types<std::runtime_error, std::exception>());
static_assert(test_types<std::range_error, std::runtime_error>());
static_assert(test_types<std::overflow_error, std::runtime_error>());
static_assert(test_types<std::underflow_error, std::runtime_error>());

// the logic and runtime hierarchies are separate
static_assert(!std::is_base_of_v<std::runtime_error, std::out_of_range>);
static_assert(!std::is_base_of_v<std::logic_error, std::range_error>);

int main() {
  CHECK(test_one<std::logic_error, std::exception>());
  CHECK(test_one<std::domain_error, std::logic_error>());
  CHECK(test_one<std::invalid_argument, std::logic_error>());
  CHECK(test_one<std::length_error, std::logic_error>());
  CHECK(test_one<std::out_of_range, std::logic_error>());
  CHECK(test_one<std::runtime_error, std::exception>());
  CHECK(test_one<std::range_error, std::runtime_error>());
  CHECK(test_one<std::overflow_error, std::runtime_error>());
  CHECK(test_one<std::underflow_error, std::runtime_error>());
  return 0;
}
