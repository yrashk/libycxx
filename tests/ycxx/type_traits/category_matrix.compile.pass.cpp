// [meta.unary.cat]/2: "For any given type T, the result of applying one of these templates to
// T and to cv T shall yield the same result." /3 (Note): "exactly one of the primary type
// categories has a value member that evaluates to true."
// [meta.unary.comp] Table 53: is_reference = lvalue or rvalue reference; is_arithmetic =
// integral or floating-point; is_fundamental = arithmetic, void or nullptr_t; is_object = object
// type ([basic.types]); is_scalar = scalar type; is_compound = compound type ([basic.compound]);
// is_member_pointer = pointer to non-static data member or function.
// [meta.rqmts]/2: each is a Cpp17UnaryTypeTrait with base characteristic bool_constant<cond>;
// [meta.type.synop]: X_v<T> = X<T>::value.
// [basic.fundamental]/14: the extended floating-point types (float16_t, ...) are floating-point
// types; [basic.fundamental]/11: char8_t, char16_t, char32_t, wchar_t, bool are integral.
// [dcl.fct]/6: "void() const &" (an abominable function type) is a function type.
// COUNTERPART: libcxx:utilities/meta/derived_from_integral_constant.compile.pass.cpp
#include <type_traits>
#include <cstddef>
#include <stdfloat>

struct S { int m; void f(); void g() const & noexcept; };
struct Incomplete;
union U { int i; float f; };
enum E { e0 };
enum class SE : unsigned char { a };
enum struct SE2 { b };
auto lambda = [] {};
using Lambda = decltype(lambda);

enum Cat { Void, Null, Integral, Floating, Array, Pointer, LRef, RRef, MemObj, MemFn, Enum, Union, Class, Function };

template <class T> constexpr int count_primary() {
  return std::is_void_v<T> + std::is_null_pointer_v<T> + std::is_integral_v<T> + std::is_floating_point_v<T> +
         std::is_array_v<T> + std::is_pointer_v<T> + std::is_lvalue_reference_v<T> + std::is_rvalue_reference_v<T> +
         std::is_member_object_pointer_v<T> + std::is_member_function_pointer_v<T> + std::is_enum_v<T> +
         std::is_union_v<T> + std::is_class_v<T> + std::is_function_v<T>;
}

template <template <class> class Trait, class T> constexpr bool trait_ok() {
  // _v agrees with ::value, base characteristic is bool_constant<value>.
  constexpr bool v = Trait<T>::value;
  static_assert(std::is_base_of_v<std::bool_constant<v>, Trait<T>>);
  static_assert(std::is_same_v<typename Trait<T>::value_type, bool>);
  static_assert(std::is_same_v<typename Trait<T>::type, std::bool_constant<v>>);
  static_assert(Trait<T>() == v && Trait<T>()() == v);
  return v;
}

template <class T, Cat c> constexpr bool check_one() {
  static_assert(count_primary<T>() == 1);
  static_assert(trait_ok<std::is_void, T>() == (c == Void));
  static_assert(std::is_void_v<T> == (c == Void));
  static_assert(trait_ok<std::is_null_pointer, T>() == (c == Null));
  static_assert(std::is_null_pointer_v<T> == (c == Null));
  static_assert(trait_ok<std::is_integral, T>() == (c == Integral));
  static_assert(std::is_integral_v<T> == (c == Integral));
  static_assert(trait_ok<std::is_floating_point, T>() == (c == Floating));
  static_assert(std::is_floating_point_v<T> == (c == Floating));
  static_assert(trait_ok<std::is_array, T>() == (c == Array));
  static_assert(std::is_array_v<T> == (c == Array));
  static_assert(trait_ok<std::is_pointer, T>() == (c == Pointer));
  static_assert(std::is_pointer_v<T> == (c == Pointer));
  static_assert(trait_ok<std::is_lvalue_reference, T>() == (c == LRef));
  static_assert(std::is_lvalue_reference_v<T> == (c == LRef));
  static_assert(trait_ok<std::is_rvalue_reference, T>() == (c == RRef));
  static_assert(std::is_rvalue_reference_v<T> == (c == RRef));
  static_assert(trait_ok<std::is_member_object_pointer, T>() == (c == MemObj));
  static_assert(std::is_member_object_pointer_v<T> == (c == MemObj));
  static_assert(trait_ok<std::is_member_function_pointer, T>() == (c == MemFn));
  static_assert(std::is_member_function_pointer_v<T> == (c == MemFn));
  static_assert(trait_ok<std::is_enum, T>() == (c == Enum));
  static_assert(std::is_enum_v<T> == (c == Enum));
  static_assert(trait_ok<std::is_union, T>() == (c == Union));
  static_assert(std::is_union_v<T> == (c == Union));
  static_assert(trait_ok<std::is_class, T>() == (c == Class));
  static_assert(std::is_class_v<T> == (c == Class));
  static_assert(trait_ok<std::is_function, T>() == (c == Function));
  static_assert(std::is_function_v<T> == (c == Function));
  // Composite categories.
  constexpr bool ref = c == LRef || c == RRef;
  constexpr bool arith = c == Integral || c == Floating;
  constexpr bool fund = arith || c == Void || c == Null;
  constexpr bool memptr = c == MemObj || c == MemFn;
  constexpr bool scalar = arith || c == Enum || c == Pointer || memptr || c == Null;
  constexpr bool object = !(ref || c == Function || c == Void);
  static_assert(trait_ok<std::is_reference, T>() == ref && std::is_reference_v<T> == ref);
  static_assert(trait_ok<std::is_arithmetic, T>() == arith && std::is_arithmetic_v<T> == arith);
  static_assert(trait_ok<std::is_fundamental, T>() == fund && std::is_fundamental_v<T> == fund);
  static_assert(trait_ok<std::is_object, T>() == object && std::is_object_v<T> == object);
  static_assert(trait_ok<std::is_scalar, T>() == scalar && std::is_scalar_v<T> == scalar);
  static_assert(trait_ok<std::is_compound, T>() == !fund && std::is_compound_v<T> == !fund);
  static_assert(trait_ok<std::is_member_pointer, T>() == memptr && std::is_member_pointer_v<T> == memptr);
  return true;
}

// T, const T, volatile T, const volatile T all give the same answers ([meta.unary.cat]/2).
template <class T, Cat c> constexpr bool check() {
  if constexpr (std::is_reference_v<T> || std::is_function_v<T>) {
    return check_one<T, c>();
  } else {
    return check_one<T, c>() && check_one<const T, c>() && check_one<volatile T, c>() &&
           check_one<const volatile T, c>();
  }
}

static_assert(check<void, Void>());
static_assert(check<std::nullptr_t, Null>());
static_assert(check<decltype(nullptr), Null>());
static_assert(check<bool, Integral>());
static_assert(check<char, Integral>());
static_assert(check<signed char, Integral>());
static_assert(check<unsigned char, Integral>());
static_assert(check<wchar_t, Integral>());
static_assert(check<char8_t, Integral>());
static_assert(check<char16_t, Integral>());
static_assert(check<char32_t, Integral>());
static_assert(check<short, Integral>());
static_assert(check<unsigned short, Integral>());
static_assert(check<int, Integral>());
static_assert(check<unsigned, Integral>());
static_assert(check<long, Integral>());
static_assert(check<unsigned long, Integral>());
static_assert(check<long long, Integral>());
static_assert(check<unsigned long long, Integral>());
static_assert(check<float, Floating>());
static_assert(check<double, Floating>());
static_assert(check<long double, Floating>());
#if defined(__STDCPP_FLOAT16_T__)
static_assert(check<std::float16_t, Floating>());
#endif
#if defined(__STDCPP_FLOAT32_T__)
static_assert(check<std::float32_t, Floating>());
#endif
#if defined(__STDCPP_FLOAT64_T__)
static_assert(check<std::float64_t, Floating>());
#endif
#if defined(__STDCPP_FLOAT128_T__)
static_assert(check<std::float128_t, Floating>());
#endif
#if defined(__STDCPP_BFLOAT16_T__)
static_assert(check<std::bfloat16_t, Floating>());
#endif
static_assert(check<int[3], Array>());
static_assert(check<int[], Array>());
static_assert(check<int[2][3], Array>());
static_assert(check<int[][3], Array>());
static_assert(check<S[2], Array>());
static_assert(check<Incomplete[], Array>());
static_assert(check<int*, Pointer>());
static_assert(check<void*, Pointer>());
static_assert(check<const void*, Pointer>());
static_assert(check<int**, Pointer>());
static_assert(check<void (*)(), Pointer>());
static_assert(check<void (*)(...) noexcept, Pointer>());
static_assert(check<int (*)[3], Pointer>());
static_assert(check<Incomplete*, Pointer>());
static_assert(check<int (S::**)(), Pointer>());
static_assert(check<int&, LRef>());
static_assert(check<const int&, LRef>());
static_assert(check<int (&)[3], LRef>());
static_assert(check<int (&)[], LRef>());
static_assert(check<void (&)(), LRef>());
static_assert(check<Incomplete&, LRef>());
static_assert(check<int (*&)(), LRef>());
static_assert(check<int&&, RRef>());
static_assert(check<const S&&, RRef>());
static_assert(check<void (&&)(), RRef>());
static_assert(check<int (&&)[], RRef>());
static_assert(check<int S::*, MemObj>());
static_assert(check<const int S::*, MemObj>());
static_assert(check<int* S::*, MemObj>());
static_assert(check<void (*S::*)(), MemObj>());   // pointer to a data member of pointer-to-function type
static_assert(check<int Incomplete::*, MemObj>());
static_assert(check<void (S::*)(), MemFn>());
static_assert(check<void (S::*)() const, MemFn>());
static_assert(check<void (S::*)() volatile &&, MemFn>());
static_assert(check<void (S::*)() const & noexcept, MemFn>());
static_assert(check<int (S::*)(int, ...), MemFn>());
static_assert(check<void (Incomplete::*)(), MemFn>());
static_assert(check<E, Enum>());
static_assert(check<SE, Enum>());
static_assert(check<SE2, Enum>());
static_assert(check<U, Union>());
static_assert(check<S, Class>());
static_assert(check<Incomplete, Class>());
static_assert(check<Lambda, Class>());
static_assert(check<std::true_type, Class>());
static_assert(check<void(), Function>());
static_assert(check<int(int, ...), Function>());
static_assert(check<void() noexcept, Function>());
static_assert(check<void() const, Function>());
static_assert(check<void() volatile, Function>());
static_assert(check<void() &, Function>());
static_assert(check<void() &&, Function>());
static_assert(check<void() const &, Function>());
static_assert(check<void() const volatile && noexcept, Function>());
static_assert(check<int (*(S))[3], Function>());   // function returning a pointer to array

int main() {}
