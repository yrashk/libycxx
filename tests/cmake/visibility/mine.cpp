// Built with libycxx (the shared library `mine`, and part of the program `prog`). Returns 3 when
// both its exceptions are caught by the right handlers and current_exception sees the first.
#include <exception>
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
  return r;
}
