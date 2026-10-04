// [uninitialized.move]/2: uninitialized_move(first, last, result) is
// "for (; first != last; (void)++result, ++first) ::new (voidify(*result))
// iterator_traits<NoThrowForwardIterator>::value_type(deref-move(first)); return result;"
// /7: uninitialized_move_n returns {first, result} (a pair) after n elements.
// [specialized.algorithms.general]/4: deref-move(it) is std::move(*it) if *it is an lvalue,
// otherwise *it.
#include <memory>
#include <iterator>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct M {
  int v;
  bool moved_from = false;
  constexpr M(int x) : v(x) {}
  constexpr M(M&& o) : v(o.v) { o.moved_from = true; }
  constexpr M(const M& o) : v(o.v + 1000) {}
};

// an input iterator whose reference is a prvalue: deref-move must not std::move a temporary
// into a dangling reference; it uses *it as-is
struct Gen {
  using iterator_category = std::input_iterator_tag;
  using value_type = M;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = M;
  int i;
  constexpr M operator*() const { return M(i); }
  constexpr Gen& operator++() {
    ++i;
    return *this;
  }
  constexpr Gen operator++(int) {
    Gen t = *this;
    ++i;
    return t;
  }
  constexpr bool operator==(const Gen&) const = default;
};

static_assert(std::is_same_v<decltype(std::uninitialized_move_n(std::declval<M*>(), 1, std::declval<M*>())),
                             std::pair<M*, M*>>);
static_assert(std::is_same_v<decltype(std::uninitialized_move(std::declval<M*>(), std::declval<M*>(), std::declval<M*>())),
                             M*>);

constexpr bool test() {
  M src[3] = {1, 2, 3};
  std::allocator<M> a;
  M* p = a.allocate(3);
  M* e = std::uninitialized_move(src, src + 3, p);
  if (e != p + 3) return false;
  for (int i = 0; i < 3; ++i)
    if (p[i].v != i + 1 || !src[i].moved_from) return false;
  std::destroy(p, p + 3);

  M src2[3] = {4, 5, 6};
  auto [in, out] = std::uninitialized_move_n(src2, 2, p);
  if (in != src2 + 2 || out != p + 2) return false;
  if (p[0].v != 4 || p[1].v != 5 || !src2[1].moved_from || src2[2].moved_from) return false;
  std::destroy(p, p + 2);

  e = std::uninitialized_move(Gen{10}, Gen{13}, p);
  if (e != p + 3 || p[0].v != 10 || p[2].v != 12) return false;  // moved, not copied
  std::destroy(p, p + 3);
  auto r = std::uninitialized_move_n(Gen{20}, 2, p);
  if (r.first != Gen{22} || r.second != p + 2 || p[1].v != 21) return false;
  std::destroy(p, p + 2);
  a.deallocate(p, 3);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
