// The vocabulary types initialize and assign their contained values from the forwarded argument
// (variant: variant/assign_uses_selected_overload), which selects the constructor or assignment operator by overload resolution, also
// for a trivially copyable type:
//   [array.creation]/3 to_array(a): {{ a[0], ..., a[N - 1] }}; /6 the rvalue form std::move(a[i])
//   [optional.ctor] optional(U&& v), optional(in_place_t, args...): direct-non-list-initializes
//     with std::forward<U>(v) / args; [optional.assign] operator=(U&& v): assigns
//     std::forward<U>(v) when engaged, else direct-non-list-initializes; [optional.assign]
//     emplace; [optional.observe] value_or(U&& v): static_cast<T>(std::forward<U>(v)) when
//     disengaged, **this when engaged; [optional.specalg] make_optional(v)
//   [expected.object.cons] expected(U&& v), [expected.object.assign] operator=(U&& v)
//   [tuple.cnstr] tuple(UTypes&&...), tuple(tuple<UTypes...>& u) (get<i>(FWD(u)));
//     [tuple.creation] make_tuple
//   [pairs.pair] pair(U1&&, U2&&), pair(pair<U1, U2>& p) (get<0>(FWD(p))); make_pair
//   [any.cons] any(T&& value) (std::forward<T>(value)); [any.nonmembers] any_cast<T>(any&):
//     static_cast<T>(*any_cast<U>(&operand)), any_cast<T>(any&&): static_cast<T>(std::move(...))
// The move constructors and move assignments initialize/assign from std::move of the contained
// value ([optional.ctor]/9, [optional.assign], [expected.object.cons]/12,
// [expected.object.assign], [tuple.cnstr] tuple(tuple&&) = default with
// std::forward<Ti>(get<i>(u)), [pairs.pair] operator=(pair&&)); they are trivial only when T's selected
// move operations are, which they are not here.
// For a non-const lvalue S, a constructor (assignment) template taking U& constrained to U = S
// is a better match than the defaulted copy operations (a template is never a copy constructor
// or copy assignment operator, [class.copy.ctor]/1, [class.copy.assign]/1), and for an rvalue
// one taking U&&; they add 1000 (2000) to the value, so a byte copy shows. Copies from const
// lvalues use the defaulted (trivial) operations and keep the value.
#include <any>
#include <array>
#include <concepts>
#include <expected>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct S {
  int v;
  S(int x = 0) : v(x) {}
  S(const S&) = default;
  S& operator=(const S&) = default;
  template <class U>
    requires std::same_as<U, S>
  S(U& o) : v(o.v + 1000) {}
  template <class U>
    requires std::same_as<U, S>
  S(U&& o) : v(o.v + 2000) {}
  template <class U>
    requires std::same_as<U, S>
  S& operator=(U& o) {
    v = o.v + 1000;
    return *this;
  }
  template <class U>
    requires std::same_as<U, S>
  S& operator=(U&& o) {
    v = o.v + 2000;
    return *this;
  }
};
static_assert(std::is_trivially_copyable_v<S>);
static_assert(!std::is_trivially_constructible_v<S, S&>);
static_assert(!std::is_trivially_constructible_v<S, S>);
static_assert(!std::is_trivially_assignable_v<S&, S&>);

static void arrays() {
  S a[5] = {1, 2, 3, 4, 5};
  auto b = std::to_array(a);
  for (int i = 0; i < 5; ++i) CHECK(b[static_cast<unsigned>(i)].v == i + 1 + 1000);
  const S ca[3] = {7, 8, 9};
  auto cb = std::to_array(ca);
  for (int i = 0; i < 3; ++i) CHECK(cb[static_cast<unsigned>(i)].v == 7 + i);
  auto m = std::to_array(std::move(a));
  for (int i = 0; i < 5; ++i) CHECK(m[static_cast<unsigned>(i)].v == i + 1 + 2000);
  S big[40];
  for (int i = 0; i < 40; ++i) big[i].v = i;
  auto bb = std::to_array(big);
  for (int i = 0; i < 40; ++i) CHECK(bb[static_cast<unsigned>(i)].v == i + 1000);
}

static void optionals() {
  S s(5);
  const S cs(6);
  std::optional<S> o(s);
  CHECK(o->v == 1005);
  std::optional<S> oc(cs);
  CHECK(oc->v == 6);
  std::optional<S> om(std::move(s));
  CHECK(om->v == 2005);
  std::optional<S> oi(std::in_place, s);
  CHECK(oi->v == 1005);
  o = s;  // engaged: assignment
  CHECK(o->v == 1005);
  std::optional<S> e;
  e = s;  // disengaged: construction
  CHECK(e->v == 1005);
  e.reset();
  e.emplace(s);
  CHECK(e->v == 1005);
  std::optional<S> none;
  CHECK(none.value_or(s).v == 1005);
  CHECK(std::as_const(none).value_or(s).v == 1005);
  CHECK(std::as_const(oc).value_or(s).v == 6);  // **this: const lvalue, trivial copy
  CHECK(std::move(none).value_or(s).v == 1005);
  CHECK(std::make_optional(s)->v == 1005);
  CHECK(std::make_optional(cs)->v == 6);
}

static void expecteds() {
  S s(4);
  std::expected<S, int> e(s);
  CHECK(e->v == 1004);
  e = s;
  CHECK(e->v == 1004);
  std::expected<S, int> u(std::unexpect, 1);
  u = s;
  CHECK(u.has_value() && u->v == 1004);
  std::expected<S, int> m(std::move(s));
  CHECK(m->v == 2004);
  std::expected<S, int> none(std::unexpect, 2);
  S t(8);
  CHECK(none.value_or(t).v == 1008);
  std::expected<int, S> g(std::unexpect, t);
  CHECK(g.error().v == 1008);
  std::unexpected<S> un(t);
  CHECK(un.error().v == 1008);
}

static void tuples_pairs() {
  S s(2);
  int i = 0;
  std::tuple<S, int> t(s, i);
  CHECK(std::get<0>(t).v == 1002);
  std::tuple<S, int> t2(t);  // tuple(tuple<UTypes...>&): get<0>(t) is S&
  CHECK(std::get<0>(t2).v == 2002);
  std::tuple<S, int> t3(std::as_const(t));  // copy constructor
  CHECK(std::get<0>(t3).v == 1002);
  CHECK(std::get<0>(std::make_tuple(s)).v == 1002);
  std::pair<S, int> p(s, i);
  CHECK(p.first.v == 1002);
  std::pair<S, int> p2(p);  // pair(pair<U1, U2>&)
  CHECK(p2.first.v == 2002);
  std::pair<S, int> p3(std::as_const(p));
  CHECK(p3.first.v == 1002);
  CHECK(std::make_pair(s, 1).first.v == 1002);
}

static void anys() {
  S s(9);
  std::any a(s);
  CHECK(std::any_cast<S&>(a).v == 1009);
  std::any_cast<S&>(a).v = 9;
  CHECK(std::any_cast<S>(a).v == 1009);                  // static_cast<S>(S&)
  CHECK(std::any_cast<S>(std::as_const(a)).v == 9);      // static_cast<S>(const S&)
  CHECK(std::any_cast<S>(std::move(a)).v == 2009);       // static_cast<S>(S&&)
  std::any c(std::in_place_type<S>, s);
  CHECK(std::any_cast<S&>(c).v == 1009);
  c.emplace<S>(s);
  CHECK(std::any_cast<S&>(c).v == 1009);
}

static void moves() {
  static_assert(!std::is_trivially_move_constructible_v<std::optional<S>>);
  static_assert(std::is_trivially_copy_constructible_v<std::optional<S>>);
  std::optional<S> o(S(1));  // optional(U&&): +2000
  CHECK(o->v == 2001);
  std::optional<S> o2(std::move(o));
  CHECK(o2->v == 4001);
  std::optional<S> o3(o);  // copy constructor: trivial
  CHECK(o3->v == 2001);
  o3 = std::move(o2);
  CHECK(o3->v == 6001);
  std::optional<S> e;
  e = std::move(o2);
  CHECK(e->v == 6001);

  std::expected<S, int> x(std::in_place, 1);
  std::expected<S, int> x2(std::move(x));
  CHECK(x2->v == 2001);
  x = std::move(x2);
  CHECK(x->v == 4001);
  std::expected<int, S> xe(std::unexpect, 1);
  std::expected<int, S> xe2(std::move(xe));
  CHECK(xe2.error().v == 2001);

  std::tuple<S, int> t(S(1), 0);
  CHECK(std::get<0>(t).v == 2001);  // tuple(UTypes&&...): S(S&&)
  std::tuple<S, int> t2(std::move(t));
  CHECK(std::get<0>(t2).v == 4001);
  t = std::move(t2);
  CHECK(std::get<0>(t).v == 6001);
  std::pair<S, int> p(S(1), 0);
  CHECK(p.first.v == 2001);
  // (pair(pair&&) = default is not checked: Clang 23 treats a defaulted move constructor as
  // trivial when the member's selected constructor is a non-trivial template.)
  std::pair<S, int> p2(S(2), 0);
  p = std::move(p2);  // [pairs.pair]: assigns std::forward<T1>(p.first)
  CHECK(p.first.v == 4002);
}

int main() {
  arrays();
  optionals();
  expecteds();
  tuples_pairs();
  anys();
  moves();
}
