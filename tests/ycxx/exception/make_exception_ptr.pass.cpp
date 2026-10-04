// [propagation]/12: make_exception_ptr(E e) noexcept "Creates an exception_ptr object that
// refers to a copy of e, as if: try { throw e; } catch(...) { return current-exception(); }"
// -- so the dynamic type is the static type E (slicing), and matching follows the usual handler
// rules. [propagation]/14 (exception_ptr_cast) observes the referenced object.
#include <exception>
#include <optional>
#include <stdexcept>
#include "check.hpp"

struct Base {
  virtual ~Base() = default;
  virtual int id() const { return 1; }
};
struct Derived : Base {
  int id() const override { return 2; }
};

int main() {
  Derived d;
  const Base& br = d;
  std::exception_ptr p = std::make_exception_ptr(br);  // E = Base: throws a Base copy
  bool caught_derived = false;
  int id = 0;
  try {
    std::rethrow_exception(p);
  } catch (const Derived&) {
    caught_derived = true;
  } catch (const Base& b) {
    id = b.id();
  }
  CHECK(!caught_derived && id == 1);
  CHECK(!std::exception_ptr_cast<Derived>(p).has_value());

  std::exception_ptr pd = std::make_exception_ptr(d);
  CHECK(std::exception_ptr_cast<Derived>(pd).has_value());
  CHECK(std::exception_ptr_cast<Base>(pd)->id() == 2);

  // the referenced object is a copy: modifying the original does not affect it
  std::runtime_error e("first");
  std::exception_ptr pe = std::make_exception_ptr(e);
  e = std::runtime_error("second");
  CHECK(std::exception_ptr_cast<std::runtime_error>(pe)->what()[0] == 'f');

  // two calls create distinct exception objects
  CHECK(std::make_exception_ptr(1) != std::make_exception_ptr(1));
  // arrays and functions decay (E is deduced by value)
  const char msg[] = "m";
  std::exception_ptr pa = std::make_exception_ptr(msg);
  bool got_ptr = false;
  try {
    std::rethrow_exception(pa);
  } catch (const char* s) {
    got_ptr = s[0] == 'm';
  }
  CHECK(got_ptr);
  return 0;
}
