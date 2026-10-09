// EXPECT-ERROR: error: static assertion failed[^\n]*std::bind_back: the target and bound arguments must be constructible and move constructible
// [func.bind.partial]/2: bind_back(f, args...): "Mandates: is_constructible_v<FD, F> &&
// is_move_constructible_v<FD> && ..." Here the target type is copyable from a const lvalue but
// not move constructible.
#include <functional>

struct Pinned {
  Pinned() = default;
  Pinned(const Pinned&) = default;
  Pinned(Pinned&&) = delete;
  int operator()(int x) const { return x; }
};

void test() {
  const Pinned p;
  (void)std::bind_back(p, 1);
}
