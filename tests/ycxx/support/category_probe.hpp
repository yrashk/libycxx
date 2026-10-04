// A callable recording, as a two-digit code, the value category / constness under which it was
// invoked (tens digit) and of its single argument (units digit):
//   0 = non-const lvalue, 1 = const lvalue, 2 = non-const rvalue, 3 = const rvalue.
// The *_result variants wrap the code in a type chosen by the caller (for and_then / or_else).
#pragma once

#include <type_traits>

namespace catprobe {

template <class A>
constexpr int arg_code() {
  constexpr bool c = std::is_const_v<std::remove_reference_t<A>>;
  return std::is_lvalue_reference_v<A> ? (c ? 1 : 0) : (c ? 3 : 2);
}

template <class Wrap>
struct Probe {
  template <class A> constexpr auto operator()(A&& a) & { (void)a; return Wrap{}(0 + arg_code<A&&>()); }
  template <class A> constexpr auto operator()(A&& a) const& { (void)a; return Wrap{}(10 + arg_code<A&&>()); }
  template <class A> constexpr auto operator()(A&& a) && { (void)a; return Wrap{}(20 + arg_code<A&&>()); }
  template <class A> constexpr auto operator()(A&& a) const&& { (void)a; return Wrap{}(30 + arg_code<A&&>()); }
};

// Nullary form (or_else of optional, monadic operations of expected<void, E> on the value side).
template <class Wrap>
struct Probe0 {
  constexpr auto operator()() & { return Wrap{}(0); }
  constexpr auto operator()() const& { return Wrap{}(10); }
  constexpr auto operator()() && { return Wrap{}(20); }
  constexpr auto operator()() const&& { return Wrap{}(30); }
};

struct AsInt {
  constexpr int operator()(int c) const { return c; }
};

}  // namespace catprobe
