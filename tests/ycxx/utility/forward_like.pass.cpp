// [forward]/5-8: forward_like<T>(x) returns static_cast<V>(x) with
// V = OVERRIDE_REF(T&&, COPY_CONST(remove_reference_t<T>, remove_reference_t<U>)):
// COPY_CONST(A, B) is const B if A is const, otherwise B; OVERRIDE_REF(A, B) is
// remove_reference_t<B>&& if A is an rvalue reference type, otherwise B&.
#include <type_traits>
#include <utility>
#include "check.hpp"

struct X { int v; };

template <class T, class U>
using FL = decltype(std::forward_like<T>(std::declval<U>()));

// U is a non-const object (lvalue X&, or rvalue X).
static_assert(std::is_same_v<FL<int, X&>, X&&>);
static_assert(std::is_same_v<FL<int&, X&>, X&>);
static_assert(std::is_same_v<FL<int&&, X&>, X&&>);
static_assert(std::is_same_v<FL<const int, X&>, const X&&>);
static_assert(std::is_same_v<FL<const int&, X&>, const X&>);
static_assert(std::is_same_v<FL<const int&&, X&>, const X&&>);
static_assert(std::is_same_v<FL<int, X>, X&&>);
static_assert(std::is_same_v<FL<int&, X>, X&>);
static_assert(std::is_same_v<FL<int&&, X>, X&&>);
static_assert(std::is_same_v<FL<const int, X>, const X&&>);
static_assert(std::is_same_v<FL<const int&, X>, const X&>);
static_assert(std::is_same_v<FL<const int&&, X>, const X&&>);
// U is const: constness of the argument is kept even when T is not const.
static_assert(std::is_same_v<FL<int, const X&>, const X&&>);
static_assert(std::is_same_v<FL<int&, const X&>, const X&>);
static_assert(std::is_same_v<FL<int&&, const X&>, const X&&>);
static_assert(std::is_same_v<FL<const int, const X&>, const X&&>);
static_assert(std::is_same_v<FL<const int&, const X&>, const X&>);
static_assert(std::is_same_v<FL<const int&&, const X&>, const X&&>);
static_assert(std::is_same_v<FL<int, const X>, const X&&>);
static_assert(std::is_same_v<FL<int&, const X&&>, const X&>);
// volatile on T is not propagated (only const is, per COPY_CONST).
static_assert(std::is_same_v<FL<volatile int&, X&>, X&>);
static_assert(std::is_same_v<FL<const volatile int, X&>, const X&&>);
// T may be any referenceable type, including class, array and function types.
static_assert(std::is_same_v<FL<X&, int&>, int&>);
static_assert(std::is_same_v<FL<const X[2], int&>, const int&&>);
static_assert(std::is_same_v<FL<void(), int&>, int&&>);
// noexcept.
static_assert(noexcept(std::forward_like<int>(std::declval<X&>())));

constexpr bool test() {
  X x{1};
  const X cx{2};
  // Returns a reference to the same object.
  if (&std::forward_like<int&>(x) != &x) return false;
  if (&std::forward_like<const int&>(x) != &x) return false;
  if (&std::forward_like<const int&>(cx) != &cx) return false;
  X&& r = std::forward_like<int>(x);
  if (&r != &x) return false;
  const X&& cr = std::forward_like<const int&&>(x);
  if (&cr != &x) return false;
  // Member access through the forwarded reference.
  int&& m = std::forward_like<X>(x.v);
  m = 5;
  if (x.v != 5) return false;
  const int& cm = std::forward_like<const X&>(x.v);
  if (&cm != &x.v) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
