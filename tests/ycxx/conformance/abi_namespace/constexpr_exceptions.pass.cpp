// The inline ABI namespace std::__y1 (DECISIONS §20.4-20.5): a construct the compilers handle
// specially still works with libycxx's declarations.
// REQUIRES: gcc, exceptions
// Constant evaluation throws the library's exceptions (P3068): GCC 16 evaluates throw expressions
// itself and std::bad_cast from a failed dynamic_cast, and holds an exception in
// std::exception_ptr (make_exception_ptr through __builtin_current_exception, DECISIONS §4).
#include <exception>
#include <stdexcept>
#include <typeinfo>
#include <cstdio>
struct B { virtual ~B() = default; };
struct D : B {};
consteval int f() {
  int r = 0;
  try { throw std::out_of_range("x"); } catch (const std::logic_error&) { r |= 1; }
  B b;
  try { (void)dynamic_cast<D&>(b); } catch (const std::bad_cast&) { r |= 2; }
  std::exception_ptr p = std::make_exception_ptr(5);
  try { std::rethrow_exception(p); } catch (int v) { r |= v == 5 ? 4 : 0; }
  return r;
}
int main() {
  constexpr int r = f();
  std::puts(r == 7 ? "ok" : "FAIL");
  return r == 7 ? 0 : 1;
}
