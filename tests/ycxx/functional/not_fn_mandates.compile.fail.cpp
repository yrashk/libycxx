// [func.not.fn]/2: not_fn(f): "Mandates: is_constructible_v<FD, F> &&
// is_move_constructible_v<FD> is true." Here FD can be copied from a const lvalue but is not
// move constructible.
#include <functional>

struct Pinned {
  Pinned() = default;
  Pinned(const Pinned&) = default;
  Pinned(Pinned&&) = delete;
  bool operator()() const { return true; }
};

void test() {
  const Pinned p;
  (void)std::not_fn(p);
}
