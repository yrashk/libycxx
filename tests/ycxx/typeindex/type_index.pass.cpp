// [type.index.synopsis]: <typeindex> includes <compare> and <typeinfo>. [type.index]:
// type_index(const type_info&) noexcept; ==, <, >, <=, >= and <=> (strong_ordering) all const
// noexcept; hash_code(), name() noexcept. /3 == is *target == *rhs.target; /4 < is
// target->before(*rhs.target); /5-7 >, <=, >= in terms of before; /8 <=> equal / less /
// greater; /9 hash_code() is target->hash_code(); /10 name() is target->name(). The exposition
// uses a pointer so that copy and assignment "work as expected".
#include <typeindex>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

using std::type_index;

struct A {
  virtual ~A() = default;
};
struct B : A {};

static_assert(std::is_nothrow_constructible_v<type_index, const std::type_info&>);
static_assert(std::is_convertible_v<const std::type_info&, type_index>);  // implicit
static_assert(!std::is_default_constructible_v<type_index>);
static_assert(std::is_nothrow_copy_constructible_v<type_index>);
static_assert(std::is_nothrow_copy_assignable_v<type_index>);
static_assert(std::is_nothrow_move_assignable_v<type_index>);
static_assert(noexcept(std::declval<const type_index&>() == std::declval<const type_index&>()));
static_assert(noexcept(std::declval<const type_index&>() < std::declval<const type_index&>()));
static_assert(noexcept(std::declval<const type_index&>() <=> std::declval<const type_index&>()));
static_assert(noexcept(std::declval<const type_index&>().hash_code()));
static_assert(noexcept(std::declval<const type_index&>().name()));
static_assert(std::is_same_v<decltype(std::declval<type_index&>() <=> std::declval<type_index&>()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::declval<type_index&>().hash_code()), std::size_t>);
static_assert(std::is_same_v<decltype(std::declval<type_index&>().name()), const char*>);
static_assert(std::is_same_v<decltype(std::declval<type_index&>() != std::declval<type_index&>()), bool>);

int main() {
  type_index ti = typeid(int);
  type_index tl(typeid(long));
  type_index ti2 = typeid(const int);  // top-level cv ignored by typeid
  CHECK(ti == ti2 && !(ti != ti2));
  CHECK(ti != tl);
  CHECK(ti.name() == typeid(int).name());
  CHECK(ti.hash_code() == typeid(int).hash_code());
  CHECK(ti.hash_code() == ti2.hash_code());

  // ordering is that of type_info::before
  bool before = typeid(int).before(typeid(long));
  CHECK((ti < tl) == before);
  CHECK((tl > ti) == before);
  CHECK((ti > tl) == typeid(long).before(typeid(int)));
  CHECK((ti <= tl) == !typeid(long).before(typeid(int)));
  CHECK((ti >= tl) == !typeid(int).before(typeid(long)));
  CHECK(ti <= ti2 && ti >= ti2 && !(ti < ti2) && !(ti > ti2));
  CHECK((ti <=> ti2) == std::strong_ordering::equal);
  CHECK((ti <=> tl) == (before ? std::strong_ordering::less : std::strong_ordering::greater));
  CHECK((tl <=> ti) == (before ? std::strong_ordering::greater : std::strong_ordering::less));
  CHECK((ti < tl) != (tl < ti));  // a strict ordering of distinct types

  // dynamic type
  B b;
  A& a = b;
  CHECK(type_index(typeid(a)) == type_index(typeid(B)));
  CHECK(type_index(typeid(a)) != type_index(typeid(A)));

  // copy and assignment
  type_index copy = ti;
  CHECK(copy == ti);
  copy = tl;
  CHECK(copy == tl && copy.name() == typeid(long).name());
  type_index moved = std::move(copy);
  CHECK(moved == tl);
  return 0;
}
