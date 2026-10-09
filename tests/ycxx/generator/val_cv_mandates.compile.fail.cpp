// EXPECT-ERROR: error: static assertion failed[^\n]*std::generator: the value type must be a cv\-unqualified object type
// [coro.generator.class]/1.2: Mandates: value is a cv-unqualified object type.
// generator<int, const int>: value = Val = const int.
#include <generator>

#ifdef YCXX_CONTROL
std::generator<int, int> g() { co_yield 1; }
#else
std::generator<int, const int> g() { co_yield 1; }
#endif

int main() {
  for (int x : g()) (void)x;
}
