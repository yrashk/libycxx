// [pointer.traits.functions]/1-4: "static constexpr pointer pointer_traits::pointer_to(see below
// r);" "Mandates: For the first member function, Ptr::pointer_to(r) is well-formed." "Returns:
// The first member function returns Ptr::pointer_to(r). The second member function [for T*]
// returns addressof(r)." "If element_type is cv void, the type of r is unspecified; otherwise,
// it is element_type&."
#include <memory>
#include <type_traits>
#include "check.hpp"

template <class T>
struct Fancy {
  using element_type = T;
  T* raw;
  int tag;
  static constexpr Fancy pointer_to(T& r) { return Fancy{&r, 42}; }
};
struct AmpOverloaded {
  int v;
  constexpr const AmpOverloaded* operator&() const { return nullptr; }
};

static_assert(std::is_same_v<decltype(std::pointer_traits<Fancy<int>>::pointer_to(std::declval<int&>())),
                             Fancy<int>>);
static_assert(std::is_same_v<decltype(std::pointer_traits<const int*>::pointer_to(std::declval<const int&>())),
                             const int*>);
static_assert(noexcept(std::pointer_traits<int*>::pointer_to(std::declval<int&>())));

constexpr bool test() {
  int x = 1;
  Fancy<int> f = std::pointer_traits<Fancy<int>>::pointer_to(x);
  if (f.raw != &x || f.tag != 42) return false;
  const AmpOverloaded a{3};
  const AmpOverloaded* pa = std::pointer_traits<const AmpOverloaded*>::pointer_to(a);
  if (pa == nullptr || pa->v != 3) return false;  // addressof, not operator&
  int* px = std::pointer_traits<int*>::pointer_to(x);
  return px == &x;
}

int main() {
  static_assert(test());
  CHECK(test());
  return 0;
}
