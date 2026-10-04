// [coro.generator.class]/1.3: Mandates: reference is either a reference type, or a
// cv-unqualified object type that models copy_constructible.
// generator<const int, int>: reference = Ref = const int, a cv-qualified object type.
#include <generator>

#ifdef YCXX_CONTROL
std::generator<int, int> g() { co_yield 1; }
#else
std::generator<const int, int> g() { co_yield 1; }
#endif

int main() {
  for (int x : g()) (void)x;
}
