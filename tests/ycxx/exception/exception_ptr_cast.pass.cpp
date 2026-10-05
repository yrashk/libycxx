// [propagation] (C++26, P2927): template<class E> constexpr optional<const E&>
// exception_ptr_cast(const exception_ptr& p) noexcept; "Returns: An optional containing a
// reference to the exception object referred to by p, if p is not null and a handler of type
// const E& would be a match for that exception object. Otherwise, nullopt."
// [exception.syn]: "template<class E> void exception_ptr_cast(const exception_ptr&&) = delete;"
// COUNTERPART: libstdcxx:18_support/exception_ptr/exception_ptr_cast.cc
#include <exception>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

struct Base {
  int v = 1;
  virtual ~Base() = default;
};
struct Derived : Base {
  Derived() { v = 2; }
};

static_assert(std::is_same_v<decltype(std::exception_ptr_cast<int>(std::declval<const std::exception_ptr&>())),
                             std::optional<const int&>>);
static_assert(noexcept(std::exception_ptr_cast<int>(std::declval<const std::exception_ptr&>())));
template <class E>
concept cast_rvalue = requires { std::exception_ptr_cast<E>(std::exception_ptr()); };
static_assert(!cast_rvalue<int>);

int main() {
  std::exception_ptr null;
  CHECK(!std::exception_ptr_cast<int>(null).has_value());

  std::exception_ptr pi = std::make_exception_ptr(42);
  auto oi = std::exception_ptr_cast<int>(pi);
  CHECK(oi.has_value() && *oi == 42);
  CHECK(!std::exception_ptr_cast<long>(pi).has_value());  // no conversions

  std::exception_ptr pd = std::make_exception_ptr(Derived());
  auto ob = std::exception_ptr_cast<Base>(pd);  // derived-to-base handler match
  CHECK(ob.has_value() && ob->v == 2);
  CHECK(&*ob == &*std::exception_ptr_cast<Derived>(pd));  // refers to the exception object
  CHECK(!std::exception_ptr_cast<std::exception>(pd).has_value());

  std::exception_ptr pe = std::make_exception_ptr(std::out_of_range("x"));
  CHECK(std::exception_ptr_cast<std::logic_error>(pe).has_value());
  CHECK(std::exception_ptr_cast<std::exception>(pe).has_value());
  CHECK(!std::exception_ptr_cast<std::runtime_error>(pe).has_value());
  return 0;
}
