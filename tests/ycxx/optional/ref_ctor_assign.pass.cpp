// [optional.optional.ref], [optional.ref.ctor], [optional.ref.assign], [optional.ref.mod],
// [optional.ref.swap]: optional<T&> holds a pointer; constructing from an lvalue binds;
// explicit(!is_convertible_v<U, T&>); converting from optional<U>& / const& / && / const&&;
// copy assignment *rebinds* (never assigns through); emplace rebinds and returns T&;
// reset / = nullopt disengage; swap exchanges bindings; value_type is T.
#include <optional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Base { int v = 1; };
struct Derived : Base { int w = 2; };
struct ConvToRef {
  int* p;
  constexpr operator int&() const { return *p; }
};

static_assert(std::is_same_v<std::optional<int&>::value_type, int>);
static_assert(std::is_same_v<std::optional<const int&>::value_type, const int>);
static_assert(std::is_constructible_v<std::optional<int&>, int&>);
static_assert(!std::is_constructible_v<std::optional<int&>, const int&>);
static_assert(!std::is_constructible_v<std::optional<int&>, int>);
static_assert(std::is_convertible_v<int&, std::optional<int&>>);
static_assert(std::is_convertible_v<int&, std::optional<const int&>>);
static_assert(std::is_convertible_v<Derived&, std::optional<Base&>>);
static_assert(!std::is_constructible_v<std::optional<Derived&>, Base&>);
static_assert(std::is_nothrow_constructible_v<std::optional<int&>, int&>);
static_assert(std::is_convertible_v<std::optional<Derived&>&, std::optional<Base&>>);
static_assert(std::is_convertible_v<std::optional<int>&, std::optional<int&>>);
static_assert(std::is_convertible_v<std::optional<int>&, std::optional<const int&>>);
static_assert(std::is_constructible_v<std::optional<const int&>, const std::optional<int>&>);
static_assert(!std::is_constructible_v<std::optional<int&>, const std::optional<int>&>);
// in_place constructor is explicit
static_assert(std::is_constructible_v<std::optional<int&>, std::in_place_t, int&>);
static_assert(!std::is_convertible_v<std::in_place_t, std::optional<int&>>);
// assignment is only nullopt / copy (plus implicit conversion); noexcept
static_assert(std::is_nothrow_assignable_v<std::optional<int&>&, std::nullopt_t>);
static_assert(noexcept(std::declval<std::optional<int&>&>().reset()));
static_assert(noexcept(std::declval<std::optional<int&>&>().swap(std::declval<std::optional<int&>&>())));
static_assert(std::is_same_v<decltype(std::declval<std::optional<int&>&>().emplace(std::declval<int&>())), int&>);

constexpr bool test() {
  int a = 1, b = 2;
  std::optional<int&> e;
  if (e.has_value() || e) return false;
  std::optional<int&> n(std::nullopt);
  if (n) return false;
  std::optional<int&> o(a);
  if (!o || &*o != &a) return false;
  *o = 10;  // assigns through
  if (a != 10) return false;
  std::optional<int&> p = b;
  o = p;  // rebinds, does not assign to a
  if (&*o != &b || a != 10) return false;
  o = a;  // via implicit conversion, rebinds
  if (&*o != &a || b != 2) return false;
  int& r = o.emplace(b);
  if (&r != &b || &*o != &b || a != 10) return false;
  o.reset();
  if (o) return false;
  o = a;
  o = std::nullopt;
  if (o) return false;
  // swap
  std::optional<int&> x(a), y;
  x.swap(y);
  if (x || &*y != &a) return false;
  swap(x, y);
  if (&*x != &a || y) return false;
  // in_place
  std::optional<const int&> ci(std::in_place, a);
  if (&*ci != &a) return false;
  // derived to base
  Derived d;
  std::optional<Base&> ob(d);
  if (&*ob != static_cast<Base*>(&d) || ob->v != 1) return false;
  // from optional<U>&: refers to the contained value of the source
  std::optional<int> src(5);
  std::optional<int&> fr(src);
  if (&*fr != &*src) return false;
  std::optional<int> empty;
  std::optional<int&> fe(empty);
  if (fe) return false;
  const std::optional<int>& csrc = src;
  std::optional<const int&> fc(csrc);
  if (&*fc != &*src) return false;
  // from optional<U&>&& (lvalue reference stays valid)
  std::optional<const int&> fm{std::optional<int&>(a)};
  if (&*fm != &a) return false;
  // conversion operator returning a reference
  ConvToRef c{&b};
  std::optional<int&> cv(c);
  if (&*cv != &b) return false;
  // copy
  std::optional<int&> cp(cv);
  if (&*cp != &b) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
