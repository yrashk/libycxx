// [variant.mod]: emplace<T>/emplace<I>, with and without initializer_list.
// Returns a reference to the new contained value; index() is I afterwards; old value destroyed.
// emplace<T> Constraints: T occurs exactly once and is constructible.
#include <variant>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct IL {
  int sum = 0, extra = 0;
  constexpr IL(std::initializer_list<int> il, int e = 0) : extra(e) { for (int x : il) sum += x; }
};
struct Dtor {
  int* count;
  constexpr Dtor(int* c) : count(c) {}
  constexpr ~Dtor() { ++*count; }
};

template <class V, class T, class... A>
concept can_emplace_type = requires(V v, A... a) { v.template emplace<T>(a...); };
template <class V, std::size_t I, class... A>
concept can_emplace_index = requires(V v, A... a) { v.template emplace<I>(a...); };

using VD = std::variant<int, int, long>;
static_assert(!can_emplace_type<VD, int, int>);   // duplicated
static_assert(can_emplace_type<VD, long, int>);
static_assert(!can_emplace_type<VD, char, int>);  // not an alternative
static_assert(can_emplace_index<VD, 1, int>);
static_assert(!can_emplace_index<std::variant<int, IL>, 1, int*>);  // not constructible

constexpr bool test() {
  std::variant<int, long, IL> v;
  long& r = v.emplace<long>(5L);
  if (v.index() != 1 || &r != std::get_if<1>(&v) || r != 5) return false;
  int& r0 = v.emplace<0>(7);
  if (v.index() != 0 || r0 != 7) return false;
  IL& il = v.emplace<IL>({1, 2, 3}, 4);
  if (v.index() != 2 || il.sum != 6 || il.extra != 4) return false;
  IL& il2 = v.emplace<2>({10});
  if (il2.sum != 10) return false;
  static_assert(std::is_same_v<decltype(v.emplace<1>(1L)), long&>);

  std::variant<int, int> dup;
  dup.emplace<1>(3);
  if (dup.index() != 1 || std::get<1>(dup) != 3) return false;

  int destroyed = 0;
  {
    std::variant<Dtor, int> d(std::in_place_index<0>, &destroyed);
    d.emplace<1>(1);
    if (destroyed != 1) return false;
    // emplacing the same alternative destroys the old value too
    d.emplace<0>(&destroyed);
    d.emplace<0>(&destroyed);
    if (destroyed != 2) return false;
  }
  if (destroyed != 3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
