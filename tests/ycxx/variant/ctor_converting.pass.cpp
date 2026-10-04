// [variant.ctor]/14-19: converting constructor template<class T> variant(T&&).
// Tj is chosen by overload resolution over FUN(Ti) for those Ti where
// "Ti x[] = {std::forward<T>(t)};" is well-formed -- i.e. narrowing conversions
// (including integer->floating, int->bool, pointer->bool) exclude an alternative.
#include <variant>
#include <type_traits>
#include "check.hpp"

struct FromCStr { const char* p; constexpr FromCStr(const char* s) : p(s) {} };
struct Explicit { int v; constexpr explicit Explicit(int x) : v(x) {} };

constexpr bool test() {
  // exact match wins
  { std::variant<int, long> v(1);  if (v.index() != 0) return false; }
  { std::variant<int, long> v(1L); if (v.index() != 1) return false; }
  // int -> float is narrowing, int -> long is not
  { int i = 3; std::variant<float, long> v(i); if (v.index() != 1 || std::get<1>(v) != 3) return false; }
  // int -> double also narrowing (non-constant source)
  { int i = 3; std::variant<double, long long> v(i); if (v.index() != 1) return false; }
  // double -> int narrowing; double chosen
  { std::variant<int, double> v(1.5); if (v.index() != 1) return false; }
  // float -> double promotion beats float -> long double conversion
  { std::variant<long double, double> v(1.0f); if (v.index() != 1) return false; }
  // int -> short narrowing; long chosen
  { int i = 7; std::variant<short, long> v(i); if (v.index() != 1) return false; }
  // int -> unsigned narrowing; long long chosen
  { int i = 7; std::variant<unsigned, long long> v(i); if (v.index() != 1) return false; }
  // int -> bool is narrowing: bool alternative excluded
  { int i = 1; std::variant<bool, long> v(i); if (v.index() != 1) return false; }
  // bool -> bool exact
  { std::variant<bool, long> v(true); if (v.index() != 0 || !std::get<0>(v)) return false; }
  // const char* -> bool is narrowing (pointer to bool), so the class type is chosen
  { std::variant<bool, FromCStr> v("abc"); if (v.index() != 1) return false; }
  // char -> int promotion beats char -> long conversion
  { std::variant<long, int> v('a'); if (v.index() != 1 || std::get<1>(v) != 'a') return false; }
  // explicit constructors do not participate (copy-list-init in Ti x[] = {t})
  { std::variant<Explicit, long> v(5); if (v.index() != 1) return false; }
  return true;
}
static_assert(test());

// Constraints
// ambiguity -> not constructible
static_assert(!std::is_constructible_v<std::variant<int, int>, int>);
static_assert(!std::is_constructible_v<std::variant<long, long long>, int>);
// all candidates narrowing -> not constructible
static_assert(!std::is_constructible_v<std::variant<float, short>, int>);
// pointer to bool is narrowing, so variant<bool> cannot be built from a pointer
static_assert(!std::is_constructible_v<std::variant<bool>, int*>);
static_assert(!std::is_constructible_v<std::variant<bool, int>, const char*>);
// explicit-only alternative cannot be the target
static_assert(!std::is_constructible_v<std::variant<Explicit>, int>);
// in_place tags never go through the converting constructor
static_assert(!std::is_convertible_v<std::in_place_type_t<int>, std::variant<int>>);
// implicit conversion
static_assert(std::is_convertible_v<int, std::variant<int, double>>);
// noexcept follows is_nothrow_constructible_v<Tj, T>
struct ThrowFromInt { ThrowFromInt(int) noexcept(false) {} };
static_assert(std::is_nothrow_constructible_v<std::variant<int, double>, double>);
static_assert(!std::is_nothrow_constructible_v<std::variant<ThrowFromInt, double>, int>);

int main() {
  CHECK(test());
  std::variant<bool, FromCStr> v("hello");
  CHECK(std::get<1>(v).p[0] == 'h');
  return 0;
}
