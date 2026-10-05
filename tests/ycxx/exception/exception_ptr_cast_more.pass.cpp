// [propagation]/14: exception_ptr_cast<E>(p) "Returns: An optional containing a reference to
// the exception object referred to by p, if p is not null and a handler of type const E& would
// be a match ([except.handle]) for that exception object. Otherwise, nullopt."
// [except.handle]/3: "T is an unambiguous public base class of E" -- a virtual base reached
// along several paths is still a single, unambiguous base. The object referred to by
// current_exception() can be observed as well, and the reference is to the same object for
// copies of the exception_ptr ([propagation]/3).
// REQUIRES: exceptions
// COUNTERPART: libstdcxx:18_support/exception_ptr/exception_ptr_cast.cc
#include <exception>
#include <optional>
#include <stdexcept>
#include "check.hpp"

struct V {
  int v = 8;
  virtual ~V() = default;
};
struct L : virtual V {};
struct R : virtual V {};
struct Diamond : L, R {};

int main() {
  const std::exception_ptr pd = std::make_exception_ptr(Diamond{});
  auto ov = std::exception_ptr_cast<V>(pd);
  CHECK(ov.has_value() && ov->v == 8);
  CHECK(std::exception_ptr_cast<L>(pd).has_value());
  CHECK(std::exception_ptr_cast<R>(pd).has_value());
  const std::exception_ptr copy = pd;
  CHECK(&*std::exception_ptr_cast<Diamond>(copy) == &*std::exception_ptr_cast<Diamond>(pd));

  // from current_exception()
  try {
    throw std::invalid_argument("ia");
  } catch (...) {
    const std::exception_ptr cur = std::current_exception();
    auto ol = std::exception_ptr_cast<std::logic_error>(cur);
    CHECK(ol.has_value() && ol->what()[0] == 'i');
    CHECK(!std::exception_ptr_cast<std::runtime_error>(cur).has_value());
  }

  // throw_with_nested results are nested_exceptions and still the original type
  try {
    try {
      throw 5;
    } catch (...) {
      std::throw_with_nested(std::runtime_error("outer"));
    }
  } catch (...) {
    const std::exception_ptr cur = std::current_exception();
    auto on = std::exception_ptr_cast<std::nested_exception>(cur);
    CHECK(on.has_value());
    const std::exception_ptr inner = on->nested_ptr();
    auto oi = std::exception_ptr_cast<int>(inner);
    CHECK(oi.has_value() && *oi == 5);
    auto orr = std::exception_ptr_cast<std::runtime_error>(cur);
    CHECK(orr.has_value() && orr->what()[0] == 'o');
  }
  return 0;
}
