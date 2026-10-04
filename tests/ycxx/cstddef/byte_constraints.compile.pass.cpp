// [support.types.byteops]/1,3,5,7,16: the shift operators and to_integer have
// "Constraints: is_integral_v<IntType> is true." Non-integral shift counts and non-integral
// to_integer targets do not match those templates. The bitwise operators take byte only.
#include <cstddef>

enum E { e1 = 1 };
enum class SE { one = 1 };

template <class T>
concept shl = requires(std::byte b, T t) { b << t; };
template <class T>
concept shr = requires(std::byte b, T t) { b >> t; };
template <class T>
concept shl_assign = requires(std::byte b, T t) { b <<= t; };
template <class T>
concept shr_assign = requires(std::byte b, T t) { b >>= t; };
template <class T>
concept to_int = requires(std::byte b) { std::to_integer<T>(b); };

static_assert(shl<int> && shl<unsigned long long> && shl<bool> && shl<char> && shl<wchar_t>);
static_assert(shr<int> && shl_assign<long> && shr_assign<short>);
static_assert(!shl<double> && !shr<float> && !shl_assign<double> && !shr_assign<long double>);
static_assert(!shl<SE> && !shr<SE> && !shl_assign<SE> && !shr_assign<SE>);
static_assert(!shl<std::byte> && !shr<std::byte>);
static_assert(!shl<int*> && !shl<std::nullptr_t>);
static_assert(to_int<int> && to_int<bool> && to_int<char8_t> && to_int<unsigned char>);
static_assert(!to_int<double> && !to_int<SE> && !to_int<E> && !to_int<std::byte> && !to_int<void*>);

template <class T>
concept bit_or = requires(std::byte b, T t) { b | t; };
static_assert(bit_or<std::byte>);
static_assert(!bit_or<int> && !bit_or<unsigned char>);
template <class T>
concept compl_ok = requires(T t) { ~t; };
static_assert(compl_ok<std::byte>);
