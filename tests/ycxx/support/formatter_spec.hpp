// [format.formatter.spec]/2: "Each header that declares the template formatter provides the
// following enabled specializations": formatter<char, char>, <char, wchar_t>, <wchar_t, wchar_t>
// (debug-enabled); for each charT the string types charT*, const charT*, charT[N],
// basic_string<charT, traits, Allocator>, basic_string_view<charT, traits> (debug-enabled); every
// signed or unsigned integer type and bool; every cv-unqualified floating-point type; nullptr_t,
// void*, const void*. /3: enable_nonlocking_formatter_optimization<T> is true for each of them.
// /6: an enabled specialization meets the BasicFormatter requirements, so it is semiregular
// ([formatter.requirements]: default constructible, copyable, destructible, swappable). /5, /7:
// any other formatter<T, charT> (formatter<wchar_t, char>, and per /4 and Note 1 the wide string
// types with char) is disabled: neither default nor copy nor move constructible, nor copy or
// move assignable.
//
// Include after the header under test only; it names basic_string<charT, char_traits<charT>,
// allocator<charT>> and basic_string_view<charT, char_traits<charT>> (the names the
// specializations of /2.2 use), and needs no other standard header than <concepts>, <cstddef> and
// <type_traits>.
#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace formatter_spec {

template <class C>
using Traits = std::char_traits<C>;
template <class T>
using Alloc = std::allocator<T>;

template <class F>
constexpr bool disabled = !std::is_default_constructible_v<F> && !std::is_copy_constructible_v<F> &&
                          !std::is_move_constructible_v<F> && !std::is_copy_assignable_v<F> &&
                          !std::is_move_assignable_v<F>;

template <class F>
constexpr bool debug_enabled = std::semiregular<F> && requires(F& f) { f.set_debug_format(); };

template <class T>
constexpr bool nl = std::enable_nonlocking_formatter_optimization<T>;

template <class charT, class... Ts>
constexpr bool all_enabled = (std::semiregular<std::formatter<Ts, charT>> && ...) && (nl<Ts> && ...);

template <class charT>
constexpr bool spec2() {
  using S = std::basic_string<charT, Traits<charT>, Alloc<charT>>;
  using SV = std::basic_string_view<charT, Traits<charT>>;
  static_assert(debug_enabled<std::formatter<charT, charT>> && nl<charT>);
  static_assert(debug_enabled<std::formatter<charT*, charT>> && nl<charT*>);
  static_assert(debug_enabled<std::formatter<const charT*, charT>> && nl<const charT*>);
  static_assert(debug_enabled<std::formatter<charT[1], charT>> && nl<charT[1]>);
  static_assert(debug_enabled<std::formatter<charT[42], charT>> && nl<charT[42]>);
  static_assert(debug_enabled<std::formatter<S, charT>> && nl<S>);
  static_assert(debug_enabled<std::formatter<SV, charT>> && nl<SV>);
  static_assert(all_enabled<charT, bool, signed char, short, int, long, long long, unsigned char, unsigned short,
                            unsigned int, unsigned long, unsigned long long>);
  static_assert(all_enabled<charT, float, double, long double>);
#ifdef __STDCPP_FLOAT32_T__
  static_assert(all_enabled<charT, decltype(0.0f32)>);
#endif
#ifdef __STDCPP_FLOAT64_T__
  static_assert(all_enabled<charT, decltype(0.0f64)>);
#endif
  static_assert(all_enabled<charT, std::nullptr_t, void*, const void*>);
  // Not provided: disabled.
  static_assert(disabled<std::formatter<int&, charT>> && disabled<std::formatter<char8_t, charT>>);
  static_assert(disabled<std::formatter<int*, charT>> && disabled<std::formatter<const volatile void*, charT>>);
  return true;
}

constexpr bool check() {
  static_assert(spec2<char>() && spec2<wchar_t>());
  static_assert(debug_enabled<std::formatter<char, wchar_t>>);
  // Note 1, /4: no implicit multibyte / wide conversions.
  static_assert(disabled<std::formatter<wchar_t, char>>);
  static_assert(disabled<std::formatter<wchar_t*, char>> && disabled<std::formatter<const wchar_t*, char>>);
  static_assert(disabled<std::formatter<char*, wchar_t>> && disabled<std::formatter<const char*, wchar_t>>);
  static_assert(disabled<std::formatter<char[3], wchar_t>>);
  static_assert(disabled<std::formatter<std::basic_string<char, Traits<char>, Alloc<char>>, wchar_t>>);
  static_assert(disabled<std::formatter<std::basic_string_view<char, Traits<char>>, wchar_t>>);
  // The primary template's default argument.
  static_assert(std::same_as<std::formatter<int>, std::formatter<int, char>>);
  return true;
}

} // namespace formatter_spec
