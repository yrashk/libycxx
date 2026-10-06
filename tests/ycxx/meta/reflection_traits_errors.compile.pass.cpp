// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// XFAIL: gcc  GCC 16.2 instantiates the class template behind the tuple_size, tuple_element and variant_alternative metafunctions, so an incomplete one or a failed Mandates is a compile error ("couldn't instantiate std::tuple_size<int>", the static_assert) instead of a meta::exception
// [meta.reflection.traits]/3.1: a call to a metafunction F whose associated specialization S
// "violate[s] a condition specified in a Mandates element" of its class template (/3.1.1, Note 1:
// S is not instantiated), or whose result S::value "would not be a valid converted constant
// expression" (/3.1.3), throws meta::exception: variant_alternative(5, variant<int, long>)
// (Mandates: I < sizeof...(Types), [variant.helper]/4) and tuple_size(^^int) (std::tuple_size<int>
// is incomplete, so it has no value member).
// REQUIRES: exceptions
#include <meta>
#include <tuple>
#include <variant>

namespace m = std::meta;

template <class F>
consteval bool throws(F f) {
  try {
    f();
  } catch (const m::exception&) {
    return true;
  }
  return false;
}
static_assert(throws([] { (void)m::tuple_size(^^int); }));
static_assert(throws([] { (void)m::variant_alternative(5, ^^std::variant<int, long>); }));
static_assert(throws([] { (void)m::tuple_element(3, ^^std::tuple<int>); }));

int main() {}
