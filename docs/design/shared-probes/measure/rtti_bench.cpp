// The cost of the per-image __cxxabiv1 vtables (DECISIONS §20.6): in shared mode the runtime in
// libycxx.so meets type_info objects whose ABI classes are another image's (libycxx_nonshared.a's
// copies) and classifies them by name. Times dynamic_cast across a hierarchy with a virtual base,
// and throw/catch through a base-class handler. Prints nanoseconds per operation.
#include <chrono>
#include <cstdio>
struct A { virtual ~A() = default; int a = 1; };
struct B : virtual A { int b = 2; };
struct C : virtual A { int c = 3; };
struct D : B, C { int d = 4; };
struct E : D {};
struct Err { virtual ~Err() = default; };
struct Err2 : Err {};
[[gnu::noinline]] A* make(int i) { static E e; static B b; return i & 1 ? static_cast<A*>(static_cast<B*>(&e)) : &b; }
[[gnu::noinline]] void thrower() { throw Err2{}; }
int main() {
  using clk = std::chrono::steady_clock;
  long hits = 0;
  constexpr int n = 5'000'000;
  auto t0 = clk::now();
  for (int i = 0; i < n; ++i) hits += dynamic_cast<C*>(make(i)) != nullptr;
  auto t1 = clk::now();
  constexpr int m = 100'000;
  for (int i = 0; i < m; ++i) {
    try { thrower(); } catch (const Err&) { ++hits; }
  }
  auto t2 = clk::now();
  std::printf("dynamic_cast %.1f ns, throw/catch %.0f ns (%ld)\n",
              std::chrono::duration<double, std::nano>(t1 - t0).count() / n,
              std::chrono::duration<double, std::nano>(t2 - t1).count() / m, hits);
  return 0;
}
