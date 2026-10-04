// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// [basic.fundamental]/17-19: std::meta::info (a reflection type) is described in
// [basic.fundamental], so it is a fundamental type; [basic.types.general]/9: it is a scalar type
// (and so trivially copyable and standard-layout); [temp.param]/12.1: scalar types are structural.
// [meta.unary.cat] Table 52: is_reflection<T>: "T is std::meta::info"; /2: T and cv T give the
// same result; Note 1: exactly one primary category holds. [meta.unary.comp] Table 53:
// is_fundamental, is_scalar, is_object true; is_arithmetic, is_compound false.
// [meta.syn]: info is decltype(^^::).
#include <meta>
#include <type_traits>

using I = std::meta::info;
static_assert(std::is_same_v<I, decltype(^^::)> && std::is_same_v<I, decltype(^^int)>);

template <class T> constexpr int primaries =
    std::is_void_v<T> + std::is_null_pointer_v<T> + std::is_integral_v<T> + std::is_floating_point_v<T> +
    std::is_array_v<T> + std::is_pointer_v<T> + std::is_lvalue_reference_v<T> + std::is_rvalue_reference_v<T> +
    std::is_member_object_pointer_v<T> + std::is_member_function_pointer_v<T> + std::is_enum_v<T> +
    std::is_union_v<T> + std::is_class_v<T> + std::is_function_v<T> + std::is_reflection_v<T>;

template <class T> constexpr bool reflection_like() {
  static_assert(std::is_reflection_v<T> && std::is_reflection<T>::value);
  static_assert(std::is_base_of_v<std::true_type, std::is_reflection<T>>);
  static_assert(primaries<T> == 1);
  static_assert(std::is_fundamental_v<T> && std::is_scalar_v<T> && std::is_object_v<T>);
  static_assert(!std::is_arithmetic_v<T> && !std::is_compound_v<T> && !std::is_member_pointer_v<T>);
  static_assert(!std::is_class_v<T> && !std::is_pointer_v<T> && !std::is_null_pointer_v<T>);
  static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
  static_assert(!std::is_signed_v<T> && !std::is_unsigned_v<T>);
  return true;
}
static_assert(reflection_like<I>() && reflection_like<const I>() && reflection_like<volatile I>() &&
              reflection_like<const volatile I>());
static_assert(std::is_structural_v<I>);
static_assert(std::is_default_constructible_v<I> && std::is_nothrow_copy_constructible_v<I>);
static_assert(I() == I());   // the null reflection

static_assert(!std::is_reflection_v<int> && !std::is_reflection_v<I&> && !std::is_reflection_v<I*> &&
              !std::is_reflection_v<I[2]> && !std::is_reflection_v<void> && !std::is_reflection_v<I()>);
static_assert(std::is_base_of_v<std::false_type, std::is_reflection<I&&>>);
static_assert(std::is_reference_v<I&> && std::is_pointer_v<I*> && std::is_array_v<I[1]>);
static_assert(std::is_same_v<std::remove_cvref_t<const I&>, I> && std::is_same_v<std::decay_t<const I>, I>);
static_assert(std::is_same_v<std::common_type_t<I, const I&>, I>);

int main() {}
