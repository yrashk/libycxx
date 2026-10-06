// Built with libycxx (the shared library `mine`, and part of the program `prog`). Returns 7 when
// both its exceptions are caught by the right handlers, current_exception sees the first, and an
// allocation failure goes through its own new_handler and bad_alloc.
#include <cstddef>
#include <exception>
#include <new>
#include <stdexcept>
extern "C" int mine_check() {
  int r = 0;
  try {
    throw std::runtime_error("mine");
  } catch (const std::exception&) {
    if (std::current_exception())
      r |= 1;
  }
  try {
    throw 42;
  } catch (int v) {
    if (v == 42)
      r |= 2;
  }
  // An allocation failure reaches this library's own new_handler and throws its own runtime's
  // bad_alloc: the operator new called here is this library's runtime's, not another's.
  static int handler_calls;
  handler_calls = 0;
  std::set_new_handler([] {
    ++handler_calls;
    std::set_new_handler(nullptr);
  });
  try {
    void* volatile p = ::operator new(static_cast<std::size_t>(-1) / 4);
    ::operator delete(p);
  } catch (const std::bad_alloc&) {
    if (handler_calls == 1)
      r |= 4;
  } catch (...) {
  }
  return r;
}
