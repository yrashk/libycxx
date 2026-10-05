// variant constructs and assigns its alternatives with exactly the expressions the draft names,
// also when every alternative is trivially copyable:
//   [variant.ctor] variant(T&& t): direct-non-list-initializes with std::forward<T>(t);
//     variant(variant&& w): with GET<j>(std::move(w)); in_place_type/in_place_index: with args.
//   [variant.assign]/13 operator=(T&& t): (13.1) holding Tj, assigns std::forward<T>(t);
//     (13.2) otherwise, if is_nothrow_constructible_v<Tj, T> || !is_nothrow_move_constructible_v
//     <Tj>, emplace<j>(std::forward<T>(t)) (no temporary); (13.3) otherwise
//     emplace<j>(Tj(std::forward<T>(t))).
//   [variant.assign]/8 operator=(variant&& rhs): (8.3) same index: assigns GET<j>(std::move(rhs));
//     (8.4) otherwise emplace<j>(GET<j>(std::move(rhs))): one move construction.
//   [variant.mod] emplace: direct-non-list-initializes with std::forward<Args>(args)...
// S is trivially copyable, but a non-const lvalue selects a constructor/assignment template
// taking U& (+1000) and an rvalue one taking U&& (+2000) over the defaulted copy operations, and
// those templates are not noexcept, so is_nothrow_move_constructible_v<S> is false and 13.2
// applies. variant<int, S>'s move operations are not trivial ([variant.ctor], [variant.assign]/10:
// trivial only if is_trivially_move_constructible_v<Ti> etc.), and its copy operations are.
#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>
#include "check.hpp"

template <bool Trivial>
struct B {
  int v;
  B(int x = 0) : v(x) {}
  B(const B&) = default;
  B& operator=(const B&) = default;
  ~B() = default;
  ~B() requires(!Trivial) {}
  template <class U>
    requires std::same_as<U, B>
  B(U& o) : v(o.v + 1000) {}
  template <class U>
    requires std::same_as<U, B>
  B(U&& o) : v(o.v + 2000) {}
  template <class U>
    requires std::same_as<U, B>
  B& operator=(U& o) {
    v = o.v + 1000;
    return *this;
  }
  template <class U>
    requires std::same_as<U, B>
  B& operator=(U&& o) {
    v = o.v + 2000;
    return *this;
  }
};
using S = B<true>;
// The same with a type that is not trivially copyable.
using N = B<false>;
static_assert(!std::is_trivially_copyable_v<N>);
static_assert(std::is_trivially_copyable_v<S>);
static_assert(!std::is_nothrow_move_constructible_v<S>);
static_assert(!std::is_nothrow_constructible_v<S, S&>);
using V = std::variant<int, S>;
static_assert(std::is_trivially_copy_constructible_v<V> && std::is_trivially_copy_assignable_v<V>);
static_assert(!std::is_trivially_move_constructible_v<V> && !std::is_trivially_move_assignable_v<V>);

int main() {
  S s(3);
  V v(s);
  CHECK(std::get<S>(v).v == 1003);
  v = s;  // 13.1
  CHECK(std::get<S>(v).v == 1003);
  v = 1;
  v = s;  // 13.2: emplace<1>(s), no temporary
  CHECK(std::get<S>(v).v == 1003);
  v = 1;
  v = S(4);  // 13.2: emplace<1>(S&&)
  CHECK(std::get<S>(v).v == 2004);
  v.emplace<S>(s);
  CHECK(std::get<S>(v).v == 1003);
  v.emplace<1>(std::move(s));
  CHECK(std::get<S>(v).v == 2003);
  V w(std::in_place_type<S>, s);
  CHECK(std::get<1>(w).v == 1003);

  V a(std::in_place_index<1>, 1);
  V a2(std::move(a));  // variant(variant&&)
  CHECK(std::get<1>(a2).v == 2001);
  a = std::move(a2);  // 8.3
  CHECK(std::get<1>(a).v == 4001);
  V a3(5);
  a3 = std::move(a);  // 8.4: one move construction
  CHECK(std::get<1>(a3).v == 6001);
  V a4(a3);  // copy constructor: trivial
  CHECK(std::get<1>(a4).v == 6001);
  V a5(7);
  a5 = a4;  // copy assignment: trivial
  CHECK(std::get<1>(a5).v == 6001);

  // three alternatives, all trivially copyable
  std::variant<char, S, long> t(std::in_place_index<0>, 'x');
  S u(9);
  t = u;
  CHECK(std::get<1>(t).v == 1009);
  std::variant<char, S, long> t2(2L);
  t2 = std::move(t);
  CHECK(std::get<1>(t2).v == 3009);

  N n(3);
  std::variant<int, N> vn(1);
  vn = n;
  CHECK(std::get<N>(vn).v == 1003);
  std::variant<int, N> vm(std::in_place_index<1>, 1), vk(5);
  vk = std::move(vm);
  CHECK(std::get<1>(vk).v == 2001);
}
