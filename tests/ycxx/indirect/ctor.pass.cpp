// [indirect.ctor]: indirect() value-initializes... constructs an owned T with an empty argument
// list; indirect(U&&) constructs from std::forward<U>(u) (explicit; constrained on
// is_constructible_v<T, U>, not indirect or in_place_t); indirect(in_place_t, us...) and the
// initializer_list form construct T in place; the copy constructor copies the owned object;
// the move constructor takes ownership and leaves other valueless. [indirect.syn]: value_type,
// allocator_type, pointer and const_pointer; deduction guide indirect(Value) -> indirect<Value>.
#include <memory>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

struct P {
  int a = 1, b = 2;
  P() = default;
  P(int x, int y) : a(x), b(y) {}
  P(std::initializer_list<int> il, int y) : a(static_cast<int>(il.size())), b(y) {}
};

using I = std::indirect<P>;
static_assert(std::is_same_v<I::value_type, P>);
static_assert(std::is_same_v<I::allocator_type, std::allocator<P>>);
static_assert(std::is_same_v<I::pointer, P*> && std::is_same_v<I::const_pointer, const P*>);
static_assert(std::is_default_constructible_v<I> && !std::is_convertible_v<P, I>);
static_assert(std::is_constructible_v<I, P> && std::is_constructible_v<I, std::in_place_t, int, int>);
static_assert(!std::is_constructible_v<I, int>);
static_assert(!std::is_constructible_v<I, std::in_place_t, int>);
static_assert(std::is_nothrow_move_constructible_v<I>);
static_assert(std::is_copy_constructible_v<I>);
static_assert(std::is_same_v<decltype(std::indirect(5)), std::indirect<int>>);
static_assert(std::is_same_v<decltype(std::indirect(std::string("x"))), std::indirect<std::string>>);

int main() {
  I d;
  CHECK(!d.valueless_after_move() && d->a == 1 && d->b == 2);
  std::indirect<int> zero;
  CHECK(*zero == 0);  // value-initialized: "with an empty argument list"
  I fromval(P(3, 4));
  CHECK(fromval->a == 3 && fromval->b == 4);
  I inplace(std::in_place, 5, 6);
  CHECK((*inplace).a == 5 && inplace->b == 6);
  I il(std::in_place, {1, 2, 3}, 9);
  CHECK(il->a == 3 && il->b == 9);
  std::indirect<std::vector<int>> v(std::in_place, {7, 8, 9});
  CHECK(v->size() == 3 && (*v)[2] == 9);
  std::indirect<std::string> s(std::string("hello"));
  CHECK(*s == "hello");
  std::indirect<std::string> s2("conv");  // U = const char(&)[5]
  CHECK(*s2 == "conv");

  // Copy: a distinct owned object with the same value.
  I c(inplace);
  CHECK(c->a == 5 && &*c != &*inplace);
  c->a = 50;
  CHECK(inplace->a == 5);

  // Move: ownership transferred, source valueless.
  P* addr = &*c;
  I m(std::move(c));
  CHECK(&*m == addr && m->a == 50);
  CHECK(c.valueless_after_move());
  // Copying and moving a valueless object gives a valueless object.
  I vc(c);
  CHECK(vc.valueless_after_move());
  I vm(std::move(vc));
  CHECK(vm.valueless_after_move());
  return 0;
}
