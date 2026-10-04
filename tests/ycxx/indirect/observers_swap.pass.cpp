// [indirect.obs]: operator* (const &, &, const &&, &&) returns *p or std::move(*p); operator->
// returns p; all noexcept; get_allocator() returns alloc. [indirect.swap]: swap exchanges
// owned objects (or valueless states) without swapping the owned objects themselves; the
// non-member swap is equivalent.
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

using I = std::indirect<int>;
I x(1);
const I cx(2);
static_assert(std::is_same_v<decltype(*x), int&>);
static_assert(std::is_same_v<decltype(*cx), const int&>);
static_assert(std::is_same_v<decltype(*std::move(x)), int&&>);
static_assert(std::is_same_v<decltype(*std::move(cx)), const int&&>);
static_assert(std::is_same_v<decltype(x.operator->()), int*>);
static_assert(std::is_same_v<decltype(cx.operator->()), const int*>);
static_assert(noexcept(*x) && noexcept(*cx) && noexcept(x.operator->()) && noexcept(x.valueless_after_move()));
static_assert(noexcept(x.get_allocator()) && std::is_same_v<decltype(x.get_allocator()), std::allocator<int>>);
static_assert(noexcept(x.swap(x)) && noexcept(swap(x, x)));

int main() {
  I a(10), b(20);
  int* pa = &*a;
  int* pb = &*b;
  CHECK(a.operator->() == pa);
  a.swap(b);
  CHECK(*a == 20 && *b == 10 && &*a == pb && &*b == pa);  // pointers exchanged
  swap(a, b);
  CHECK(*a == 10 && &*a == pa);
  I moved(std::move(b));
  CHECK(b.valueless_after_move());
  a.swap(b);  // exchange with a valueless object
  CHECK(a.valueless_after_move() && *b == 10);
  int&& r = *std::move(b);
  CHECK(&r == pa);
  return 0;
}
