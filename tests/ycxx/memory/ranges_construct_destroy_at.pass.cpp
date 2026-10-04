// [memory.syn], [specialized.construct], [specialized.destroy]: <memory> declares
// std::ranges::construct_at and std::ranges::destroy_at, with the same effects as the
// std:: versions; ranges::destroy_at requires destructible<T> and is noexcept.
#include <memory>
#include <type_traits>
#include "check.hpp"

struct Pt {
  int x, y;
  constexpr Pt(int a, int b) : x(a), y(b) {}
};
struct Tracked {
  int* live;
  constexpr explicit Tracked(int* l) : live(l) { ++*live; }
  constexpr ~Tracked() { --*live; }
};

static_assert(std::is_same_v<decltype(std::ranges::construct_at(std::declval<Pt*>(), 1, 2)), Pt*>);
static_assert(std::is_same_v<decltype(std::ranges::destroy_at(std::declval<Pt*>())), void>);
static_assert(noexcept(std::ranges::destroy_at(std::declval<Pt*>())));
template <class T, class... A>
concept RCA = requires(T* p, A&&... a) { std::ranges::construct_at(p, static_cast<A&&>(a)...); };
static_assert(RCA<Pt, int, int> && !RCA<Pt, int> && !RCA<int[]>);

constexpr bool test() {
  int live = 0;
  std::allocator<Tracked> a;
  Tracked* p = a.allocate(2);
  Tracked* r = std::ranges::construct_at(p, &live);
  std::ranges::construct_at(p + 1, &live);
  if (r != p || live != 2) return false;
  std::ranges::destroy_at(p);
  std::ranges::destroy_at(p + 1);
  if (live != 0) return false;
  a.deallocate(p, 2);
  std::allocator<Pt> b;
  Pt* q = b.allocate(1);
  std::ranges::construct_at(q, 3, 4);
  if (q->x != 3 || q->y != 4) return false;
  std::ranges::destroy_at(q);
  b.deallocate(q, 1);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  alignas(int) unsigned char buf[sizeof(int[3])];
  int (*p)[3] = std::ranges::construct_at(reinterpret_cast<int(*)[3]>(buf));
  CHECK((*p)[0] == 0 && (*p)[2] == 0);
  std::ranges::destroy_at(p);
  return 0;
}
