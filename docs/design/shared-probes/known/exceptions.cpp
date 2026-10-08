// The exception classes the runtime throws for the compiler: std::bad_array_new_length
// (__cxa_throw_bad_array_new_length, GCC) and std::bad_alloc; std::terminate from a noexcept function
// is checked by nm only (the compilers call it by name).
#include <new>
#include <exception>
#include <stdexcept>
#include <cstdio>
#include <cstddef>
void may_throw();
void wrapper() noexcept { may_throw(); }   // the compiler's terminate call
int main(int argc, char**) {
  int r = 0;
  volatile std::ptrdiff_t n = -argc;
  // GCC throws bad_array_new_length (__cxa_throw_bad_array_new_length); Clang passes SIZE_MAX to
  // operator new, which throws bad_alloc.
  try { int* volatile p = new int[n]; delete[] p; } catch (const std::bad_alloc&) { r |= 1; } catch (...) {}  // volatile: not elided
  try { void* volatile p = ::operator new(static_cast<std::size_t>(-1) / 4); ::operator delete(p); }
  catch (const std::bad_alloc&) { r |= 2; } catch (...) {}
  try { throw std::runtime_error("x"); } catch (const std::exception& e) { r |= std::current_exception() && e.what()[0] == 'x' ? 4 : 0; }
  if (argc > 5) wrapper();
  std::puts(r == 7 ? "ok" : "FAIL");
  return r == 7 ? 0 : 1;
}
void may_throw() { throw 1; }
