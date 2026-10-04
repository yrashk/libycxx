// [cwchar.syn]: "using mbstate_t = see below; // freestanding". /1: "The contents and
// meaning of the header <cwchar> are the same as the C standard library header <wchar.h>".
// C (ISO/IEC 9899:2024, 7.31.1): mbstate_t is "a complete object type other than an array
// type" that can hold the conversion state; 7.31.6/1: "A zero-valued mbstate_t object is (at
// least) one way to describe an initial conversion state." mbsinit (hosted) reports that.
// [char.traits.specializations.char]/1: "The type mbstate_t is defined in <cwchar>".
#include <cwchar>
#include <type_traits>
#include "check.hpp"

static_assert(sizeof(std::mbstate_t) > 0);  // complete
static_assert(std::is_object_v<std::mbstate_t> && !std::is_array_v<std::mbstate_t>);
// [char.traits.typedefs]/2 requirements on state_type, which is mbstate_t for all five
// specializations
static_assert(std::is_default_constructible_v<std::mbstate_t>);
static_assert(std::is_copy_constructible_v<std::mbstate_t> && std::is_copy_assignable_v<std::mbstate_t>);
static_assert(std::is_destructible_v<std::mbstate_t>);

std::mbstate_t global_state;  // static storage: zero-initialised

int main() {
  std::mbstate_t a{};
  std::mbstate_t b = {};
  std::mbstate_t c = std::mbstate_t();
  CHECK(std::mbsinit(&a) != 0);
  CHECK(std::mbsinit(&b) != 0);
  CHECK(std::mbsinit(&c) != 0);
  CHECK(std::mbsinit(&global_state) != 0);
  CHECK(std::mbsinit(nullptr) != 0);  // C 7.31.6.2.1: "If ps is a null pointer ... nonzero"
  std::mbstate_t d = a;               // copies are initial states too
  d = b;
  CHECK(std::mbsinit(&d) != 0);
  return 0;
}
