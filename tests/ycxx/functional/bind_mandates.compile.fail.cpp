// [func.bind.bind]/2: "Mandates: is_constructible_v<FD, F> is true. For each Ti in
// BoundArgs, is_constructible_v<TDi, Ti> is true." (Here a bound argument is a const lvalue
// of a type whose copy constructor is deleted.)
#include <functional>

struct NoCopy {
  NoCopy() = default;
  NoCopy(NoCopy&&) = default;
  NoCopy(const NoCopy&) = delete;
};
int f(const NoCopy&) { return 0; }

void test() {
  const NoCopy nc;
  auto g = std::bind(f, nc);
  (void)g;
}
