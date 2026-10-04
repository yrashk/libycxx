// [polymorphic.ctor], [polymorphic.obs]: polymorphic<Base> owns an object of Base or of a type
// derived from it. polymorphic() constructs a T; polymorphic(U&&) constructs an owned object of
// type remove_cvref_t<U> (constrained: derived_from<UU, T>, copy constructible, not
// polymorphic or in_place_type_t); polymorphic(in_place_type<U>, ts...) constructs a U.
// Copying copies the owned object with its dynamic type (no slicing); operator* and -> give
// access through T; destruction destroys the owned object as its own type U.
#include <memory>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include "check.hpp"

int live = 0;
struct Shape {
  Shape() { ++live; }
  Shape(const Shape&) { ++live; }
  ~Shape() { --live; }  // not virtual: polymorphic still destroys the derived object as U
  virtual int sides() const { return 0; }
};
struct Tri : Shape {
  int tag = 3;
  Tri() = default;
  explicit Tri(int t) : tag(t) {}
  int sides() const override { return 3; }
};
struct Poly : Shape {
  int n;
  int extra;
  Poly(std::initializer_list<int> il, int e) : n(static_cast<int>(il.size())), extra(e) {}
  int sides() const override { return n; }
};
struct Unrelated {};
struct NoCopy : Shape {
  NoCopy() = default;
  NoCopy(const NoCopy&) = delete;
};

using P = std::polymorphic<Shape>;
static_assert(std::is_same_v<P::value_type, Shape> && std::is_same_v<P::allocator_type, std::allocator<Shape>>);
static_assert(std::is_same_v<P::pointer, Shape*> && std::is_same_v<P::const_pointer, const Shape*>);
static_assert(std::is_constructible_v<P, Tri> && !std::is_convertible_v<Tri, P>);
static_assert(!std::is_constructible_v<P, Unrelated>);
static_assert(!std::is_constructible_v<P, NoCopy>);
static_assert(std::is_constructible_v<P, std::in_place_type_t<Tri>, int>);
static_assert(!std::is_constructible_v<P, std::in_place_type_t<Unrelated>>);
static_assert(!std::is_constructible_v<P, std::in_place_type_t<const Tri>>);
static_assert(std::is_nothrow_move_constructible_v<P>);
static_assert(std::is_same_v<decltype(*std::declval<P&>()), Shape&>);
static_assert(std::is_same_v<decltype(*std::declval<const P&>()), const Shape&>);
static_assert(noexcept(*std::declval<P&>()) && noexcept(std::declval<P&>().operator->()));

int main() {
  {
    P base;
    CHECK(base->sides() == 0 && !base.valueless_after_move());
    P t(Tri{});
    CHECK(t->sides() == 3);
    P t5(std::in_place_type<Tri>, 5);
    CHECK(t5->sides() == 3 && static_cast<const Tri&>(*t5).tag == 5);
    P poly(std::in_place_type<Poly>, {1, 2, 3, 4}, 7);
    CHECK(poly->sides() == 4);

    // Copy keeps the dynamic type.
    P copy(t5);
    CHECK(copy->sides() == 3 && &*copy != &*t5 && static_cast<Tri&>(*copy).tag == 5);
    P pcopy = poly;
    CHECK(pcopy->sides() == 4 && static_cast<Poly&>(*pcopy).extra == 7);

    // Copy assignment replaces the owned object, possibly with another type.
    copy = poly;
    CHECK(copy->sides() == 4);
    copy = base;
    CHECK(copy->sides() == 0);

    // Move.
    P moved(std::move(t5));
    CHECK(moved->sides() == 3);
    P target;
    target = std::move(moved);
    CHECK(target->sides() == 3);

    // swap
    P s1(std::in_place_type<Tri>), s2;
    Shape* a1 = &*s1;
    s1.swap(s2);
    CHECK(s1->sides() == 0 && s2->sides() == 3 && &*s2 == a1);
    swap(s1, s2);
    CHECK(s1->sides() == 3);
    CHECK(s1.get_allocator() == std::allocator<Shape>());
  }
  CHECK(live == 0);  // every owned object destroyed with its own type
  return 0;
}
