// XFAIL-COMPILER: gcc  GCC 16.2 has no __builtin_is_within_lifetime (STATUS.md)
// [meta.const.eval]: template<class U = void, class T>
//   consteval bool is_within_lifetime(const T* p) noexcept;
// /3 Mandates: static_cast<const volatile U*>(p) is well-formed.
// /4 Returns: true if p is a pointer to an object that is within its lifetime ([basic.life]) and
//    static_cast<const volatile U*>(p) is a constant subexpression; otherwise, false.
// [expr.static.cast]/12: a downcast of a pointer to a B that is not a base class subobject of a D
// has undefined behavior, so it is not a constant subexpression ([expr.const]).
// [version.syn]: __cpp_lib_is_within_lifetime 202603L.
#include <type_traits>
#include <version>

#if !defined(__cpp_lib_is_within_lifetime) || __cpp_lib_is_within_lifetime < 202603L
#error "__cpp_lib_is_within_lifetime"
#endif

struct B { int b = 1; };
struct D : B { int d = 2; };
struct D2 : B {};

// The U parameter: true only when the cast to const volatile U* is a constant subexpression.
consteval bool with_u() {
  D d;
  const B* pb = &d;
  if (!std::is_within_lifetime<D>(pb)) return false;          // valid downcast
  if (!std::is_within_lifetime<B>(pb)) return false;
  if (!std::is_within_lifetime<const volatile B>(pb)) return false;
  if (!std::is_within_lifetime<void>(pb)) return false;
  if (std::is_within_lifetime<D2>(pb)) return false;          // B subobject of a D, not of a D2
  B b;
  if (std::is_within_lifetime<D>(&b)) return false;           // complete object is a B
  if (!std::is_within_lifetime<B>(&d)) return false;          // upcast
  return true;
}
static_assert(with_u());


// U is explicitly specifiable and T is deduced.
constexpr int g = 0;
static_assert(std::is_within_lifetime<const int>(&g));
static_assert(std::is_within_lifetime<void>(&g));
static_assert(noexcept(std::is_within_lifetime<void>(&g)));

int main() {}
