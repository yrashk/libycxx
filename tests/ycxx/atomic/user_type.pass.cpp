// [atomics.types.generic]: atomic<T> for a trivially copyable class type: load/store/
// exchange/compare_exchange on the whole value; compare_exchange compares value
// representations ([atomics.types.operations]/23) and updates expected only on failure.
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <cstring>
#include "check.hpp"

struct Point {
  int x, y;
};
struct Big {
  long long v[6];
};

static bool same(const Point& a, const Point& b) { return a.x == b.x && a.y == b.y; }

int main() {
  std::atomic<Point> a;
  CHECK(same(a.load(), Point{0, 0}));  // initialized with T()
  std::atomic<Point> p(Point{1, 2});
  CHECK(same(p.load(), Point{1, 2}));
  p.store(Point{3, 4});
  CHECK(same(p, Point{3, 4}));
  CHECK(same(p = Point{5, 6}, Point{5, 6}));
  CHECK(same(p.exchange(Point{7, 8}), Point{5, 6}));
  Point e{0, 0};
  CHECK(!p.compare_exchange_strong(e, Point{9, 9}));
  CHECK(same(e, Point{7, 8}));
  CHECK(p.compare_exchange_strong(e, Point{9, 9}));
  CHECK(same(e, Point{7, 8}));  // unchanged on success
  CHECK(same(p.load(), Point{9, 9}));
  e = Point{9, 9};
  while (!p.compare_exchange_weak(e, Point{10, 11}, std::memory_order::acq_rel)) {
    CHECK(same(e, Point{9, 9}));  // spurious failure stores back the same value
  }
  CHECK(same(p.load(), Point{10, 11}));

  // a type larger than any lock-free width
  Big b1{}, b2{};
  for (int i = 0; i < 6; ++i) { b1.v[i] = i; b2.v[i] = 100 + i; }
  std::atomic<Big> big(b1);
  Big cur = big.load();
  CHECK(std::memcmp(&cur.v, &b1.v, sizeof b1.v) == 0);
  Big old = big.exchange(b2);
  CHECK(std::memcmp(&old.v, &b1.v, sizeof b1.v) == 0);
  Big ex = b1;
  CHECK(!big.compare_exchange_strong(ex, b1));
  CHECK(std::memcmp(&ex.v, &b2.v, sizeof b2.v) == 0);
  CHECK(big.compare_exchange_strong(ex, b1));
  CHECK(big.load().v[5] == 5);
  CHECK(big.is_lock_free() == big.is_lock_free());
  return 0;
}
