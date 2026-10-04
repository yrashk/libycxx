// XFAIL-COMPILER: gcc  GCC 16.2 has no __builtin_is_within_lifetime (STATUS.md)
// [meta.const.eval]: template<class U = void, class T>
//   consteval bool is_within_lifetime(const T* p) noexcept;
// /3 Mandates: static_cast<const volatile U*>(p) is well-formed.
// /4 Returns: true if p is a pointer to an object that is within its lifetime ([basic.life]) and
//    static_cast<const volatile U*>(p) is a constant subexpression; otherwise, false.
// /5 Remarks: ill-formed in a core constant expression E unless p points to an object usable in
//    constant expressions or whose complete object's lifetime began within E.
// (The U parameter: is_within_lifetime_u.pass.cpp.)
// [version.syn]: __cpp_lib_is_within_lifetime 202603L (in <type_traits>).
#include <type_traits>
#include <memory>
#include <version>

#if !defined(__cpp_lib_is_within_lifetime) || __cpp_lib_is_within_lifetime < 202603L
#error "__cpp_lib_is_within_lifetime"
#endif

union U { int i; float f; };

// (A null pointer points to no object, so /5 makes such a call ill-formed; not called here.)
constexpr int g = 0;
static_assert(noexcept(std::is_within_lifetime(&g)));
static_assert(std::is_same_v<decltype(std::is_within_lifetime(&g)), bool>);
static_assert(std::is_within_lifetime(&g));

consteval bool basics() {
  int i = 0;
  if (!std::is_within_lifetime(&i)) return false;
  int a[3] = {};
  if (!std::is_within_lifetime(&a[2])) return false;
  if (!std::is_within_lifetime(&a)) return false;
  const int ci = 3;
  if (!std::is_within_lifetime(&ci)) return false;
  volatile int vi = 0;   // const T* deduces T = volatile int
  if (!std::is_within_lifetime(&vi)) return false;
  return true;
}
static_assert(basics());

consteval bool unions() {
  U u;
  u.i = 1;
  if (!std::is_within_lifetime(&u.i) || std::is_within_lifetime(&u.f)) return false;
  u.f = 2.0f;
  if (std::is_within_lifetime(&u.i) || !std::is_within_lifetime(&u.f)) return false;
  if (!std::is_within_lifetime(&u)) return false;
  return true;
}
static_assert(unions());

consteval bool dynamic() {
  std::allocator<int> al;
  int* p = al.allocate(2);
  bool ok = !std::is_within_lifetime(p);
  std::construct_at(p, 5);
  ok = ok && std::is_within_lifetime(p) && !std::is_within_lifetime(p + 1);
  std::destroy_at(p);
  ok = ok && !std::is_within_lifetime(p);
  al.deallocate(p, 2);
  return ok;
}
static_assert(dynamic());

// [meta.const.eval] Example 2.
struct OptBool {
  union { bool b; char c; };
  constexpr OptBool() : c(2) {}
  constexpr OptBool(bool v) : b(v) {}
  constexpr bool has_value() const {
    if consteval {
      return std::is_within_lifetime(&b);
    } else {
      return c != 2;
    }
  }
};
constexpr OptBool disengaged;
constexpr OptBool engaged(true);
static_assert(!disengaged.has_value());
static_assert(engaged.has_value());

int main() {
  OptBool x, y(false);
  if (x.has_value() || !y.has_value()) return 1;
}
