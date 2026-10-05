// [func.bind.partial]/2: bind_front(f, args...): "Mandates: is_constructible_v<FD, F> &&
// is_move_constructible_v<FD> && (is_constructible_v<BoundArgs, Args> && ...) &&
// (is_move_constructible_v<BoundArgs> && ...) is true." A bound argument that is not move
// constructible violates the Mandates.
// COUNTERPART: libcxx:utilities/function.objects/func.bind_front/bind_front.pass.cpp
// EXPECT-ERROR: static assertion failed.*std::bind_front: the target and bound arguments must be constructible and move constructible
#include <functional>

struct Pinned {
  Pinned() = default;
  Pinned(const Pinned&);  // copyable from lvalues...
  Pinned(Pinned&&) = delete;  // ...but not move constructible
};

void f(const Pinned&) {}
void g() {
  Pinned p;
  (void)std::bind_front(f, p);
}
