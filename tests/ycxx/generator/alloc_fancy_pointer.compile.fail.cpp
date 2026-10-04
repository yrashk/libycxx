// [coro.generator.promise]/17-18: B is allocator_traits<A>::rebind_alloc<U>; "Mandates:
// allocator_traits<B>::pointer is a pointer type." An Allocator whose pointer is a class
// type (a fancy pointer) makes the coroutine's operator new ill-formed. Control
// (-DYCXX_CONTROL): the same coroutine with std::allocator<int>.
#include <generator>
#include <memory>
#include "fancy_ptr.hpp"

#ifdef YCXX_CONTROL
using A = std::allocator<int>;
#else
using A = FancyAlloc<int>;
#endif

std::generator<int, void, A> g() { co_yield 1; }

int main() {
  int s = 0;
  for (int x : g()) s += x;
  return s == 1 ? 0 : 1;
}
