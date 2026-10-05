// [refwrap.general]: reference_wrapper<T> "is a trivially copyable type"; T "may be an
// incomplete type"; deduction guide reference_wrapper(T&) -> reference_wrapper<T>.
// [refwrap.const]/1-4: template<class U> reference_wrapper(U&&) with "Constraints: The
// expression FUN(declval<U>()) is well-formed and is_same_v<remove_cvref_t<U>,
// reference_wrapper> is false", FUN(T&) noexcept / FUN(T&&) = delete; noexcept(FUN(...)).
// [refwrap.assign], [refwrap.access], [refwrap.invoke].
// COUNTERPART: libstdcxx:20_util/reference_wrapper/invoke-2.cc
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Incomplete;
Incomplete& get_incomplete();
// T may be incomplete: forming the type and wrapping a reference are fine.
using RWI = std::reference_wrapper<Incomplete>;
inline RWI wrap_incomplete() { return std::ref(get_incomplete()); }

struct ConvToIntRef {
  int* p;
  constexpr operator int&() const noexcept { return *p; }
};
struct ConvToIntRefThrows {
  int* p;
  operator int&() const { return *p; }
};
struct Fn {
  constexpr int operator()(int x) const noexcept { return x + 1; }
  int operator()(double) const { return 0; }
};
int free_fn(int x) { return x * 3; }

using RW = std::reference_wrapper<int>;
static_assert(std::is_trivially_copyable_v<RW>);
static_assert(std::is_same_v<RW::type, int>);
static_assert(std::is_nothrow_constructible_v<RW, int&>);
static_assert(!std::is_constructible_v<RW, int>);
static_assert(!std::is_constructible_v<RW, int&&>);
static_assert(!std::is_constructible_v<RW, const int&>);
static_assert(std::is_convertible_v<int&, RW>);
static_assert(std::is_constructible_v<RW, ConvToIntRef>);
static_assert(std::is_nothrow_constructible_v<RW, ConvToIntRef>);
static_assert(std::is_constructible_v<RW, ConvToIntRefThrows>);
static_assert(!std::is_nothrow_constructible_v<RW, ConvToIntRefThrows>);
static_assert(std::is_nothrow_copy_assignable_v<RW>);
static_assert(std::is_same_v<decltype(std::declval<const RW&>().get()), int&>);
static_assert(noexcept(std::declval<const RW&>().get()));
static_assert(std::is_convertible_v<const RW&, int&>);
static_assert(noexcept(std::declval<const std::reference_wrapper<Fn>&>()(1)));
static_assert(!noexcept(std::declval<const std::reference_wrapper<Fn>&>()(1.0)));
static_assert(std::is_same_v<std::invoke_result_t<std::reference_wrapper<int(int)>, int>, int>);

constexpr bool test() {
  int a = 1, b = 2;
  std::reference_wrapper ra(a);
  static_assert(std::is_same_v<decltype(ra), RW>);
  if (&ra.get() != &a) return false;
  int& ref = ra;
  if (&ref != &a) return false;
  std::reference_wrapper rb = b;
  ra = rb;  // rebinds, does not assign through
  if (&ra.get() != &b || a != 1) return false;
  ra.get() = 5;
  if (b != 5) return false;
  RW copy(ra);
  if (&copy.get() != &b) return false;
  RW via_conv(ConvToIntRef{&a});
  if (&via_conv.get() != &a) return false;
  Fn f;
  std::reference_wrapper rf(f);
  if (rf(1) != 2) return false;
  const int c = 3;
  std::reference_wrapper rc(c);
  static_assert(std::is_same_v<decltype(rc), std::reference_wrapper<const int>>);
  if (rc.get() != 3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::reference_wrapper<int(int)> rfn(free_fn);
  CHECK(rfn(2) == 6);
  CHECK(&rfn.get() == &free_fn);
  // a reference_wrapper to a pointer to member is invoked via INVOKE
  struct M {
    int v = 4;
    int get() const { return v; }
  } m;
  int (M::*pm)() const = &M::get;
  std::reference_wrapper<int (M::*)() const> rpm(pm);
  CHECK(rpm(m) == 4);
  return 0;
}
