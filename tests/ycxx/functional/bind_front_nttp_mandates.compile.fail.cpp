// [func.bind.partial]/7.1: template<auto f, class... Args> bind_front(args...): "Mandates:
// (is_constructible_v<BoundArgs, Args> && ...) is true". The bound argument is a const lvalue
// of a type whose copy constructor is deleted.
#include <functional>

struct NoCopy {
  NoCopy() = default;
  NoCopy(NoCopy&&) = default;
  NoCopy(const NoCopy&) = delete;
};
int f(const NoCopy&) { return 0; }

void test() {
  const NoCopy nc;
  (void)std::bind_front<f>(nc);
}
