// [indirect.syn]: every member of indirect is constexpr, so it can be used (with
// std::allocator) during constant evaluation.
#include <memory>
#include <utility>

struct Point {
  int x, y;
  constexpr bool operator==(const Point&) const = default;
};

constexpr bool run() {
  std::indirect<Point> a(Point{1, 2});
  std::indirect<Point> b(std::in_place, 3, 4);
  if (a->x != 1 || (*b).y != 4) return false;
  std::indirect<Point> c = a;
  c->x = 10;
  if (a->x != 1 || c->x != 10) return false;
  a = b;
  if (!(a == b) || a == c) return false;
  std::indirect<Point> d(std::move(c));
  if (!c.valueless_after_move() || d->x != 10) return false;
  c = d;
  d.swap(a);
  if (d->x != 3 || a->x != 10) return false;
  a = Point{7, 7};
  return *a == Point{7, 7} && a == Point{7, 7};
}
static_assert(run());

int main() { return run() ? 0 : 1; }
