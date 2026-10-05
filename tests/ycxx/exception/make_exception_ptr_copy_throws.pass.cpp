// [propagation]/12: make_exception_ptr(E e) noexcept "Effects: Creates an exception_ptr object
// that refers to a copy of e, as if: try { throw e; } catch(...) { return current-exception(); }"
// In that code, `throw e` copies the parameter e (a function parameter is not eligible for copy
// elision or implicit move into the exception object, [class.copy.elision]); if that copy
// throws, the catch(...) catches the copy constructor's exception, so the result refers to it.
// REQUIRES: exceptions
#include <exception>
#include <optional>
#include "check.hpp"

struct CopyThrows {
  CopyThrows() = default;
  CopyThrows(CopyThrows&&) noexcept = default;
  CopyThrows(const CopyThrows&) { throw 42; }
};

int main() {
  std::exception_ptr p = std::make_exception_ptr(CopyThrows());  // parameter is move-constructed
  CHECK(p != nullptr);
  auto i = std::exception_ptr_cast<int>(p);
  CHECK(i.has_value() && *i == 42);
  CHECK(!std::exception_ptr_cast<CopyThrows>(p).has_value());
  return 0;
}
