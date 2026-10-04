// [support.types.nullptr]/2: "The macro NULL is an implementation-defined null pointer
// constant that is a literal" (footnote: "Possible definitions include nullptr, 0 and 0L, but
// not (void*)0.")
// [support.types.layout]/1: offsetof "accepts a restricted set of type arguments"; on a
// standard-layout class it is required. "The expression offsetof(type, member-designator) is
// never type-dependent and it is value-dependent if and only if type is dependent." "No
// operation invoked by the offsetof macro shall throw an exception and
// noexcept(offsetof(type, member-designator)) shall be true." Footnote: "offsetof is required
// to work as specified even if unary operator& is overloaded for any of the types involved."
// [cstddef.syn]/1: <cstddef> "does not define the macro unreachable".
#include <cstddef>
#include <type_traits>
#include "check.hpp"

#ifndef NULL
#  error "NULL is not defined"
#endif
#ifndef offsetof
#  error "offsetof is not defined"
#endif
#ifdef unreachable
#  error "<cstddef> must not define the macro unreachable"
#endif

using NullT = decltype(NULL);
static_assert(std::is_integral_v<NullT> || std::is_null_pointer_v<NullT>);
static_assert(!std::is_pointer_v<NullT>);  // not (void*)0
int* const np = NULL;
void (*const fp)() = NULL;

struct Evil {
  void operator&() const = delete;  // overloaded (and deleted) unary &
};
struct SL {
  char a;
  int b;
  Evil e;
  double c[3];
};
static_assert(std::is_standard_layout_v<SL>);
static_assert(offsetof(SL, a) == 0);
static_assert(offsetof(SL, b) >= sizeof(char) && offsetof(SL, b) % alignof(int) == 0);
static_assert(offsetof(SL, e) >= offsetof(SL, b) + sizeof(int));
static_assert(offsetof(SL, c) % alignof(double) == 0);
static_assert(offsetof(SL, c[2]) == offsetof(SL, c) + 2 * sizeof(double));  // C member-designator
static_assert(noexcept(offsetof(SL, b)));
static_assert(std::is_same_v<decltype(offsetof(SL, b)), std::size_t>);

template <class T>
struct Holder {
  // never type-dependent: the type is size_t even inside a template with dependent T
  static constexpr std::size_t off = offsetof(T, b);
  static_assert(std::is_same_v<decltype(offsetof(T, b)), std::size_t>);
};
static_assert(Holder<SL>::off == offsetof(SL, b));

int main() {
  CHECK(np == nullptr && fp == nullptr);
  SL s{};
  CHECK(reinterpret_cast<char*>(&s.b) - reinterpret_cast<char*>(&s) == static_cast<std::ptrdiff_t>(offsetof(SL, b)));
  CHECK(reinterpret_cast<char*>(&s.c[1]) - reinterpret_cast<char*>(&s) == static_cast<std::ptrdiff_t>(offsetof(SL, c[1])));
  return 0;
}
