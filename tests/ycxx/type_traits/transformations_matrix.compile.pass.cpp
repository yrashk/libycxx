// [meta.trans.cv] Table 57, [meta.trans.ref] Table 58, [meta.trans.sign] Table 59,
// [meta.trans.arr] Table 60, [meta.trans.ptr] Table 61, [meta.trans.other] Table 62 (type_identity,
// remove_cvref, decay, enable_if, conditional, underlying_type, unwrap_reference,
// unwrap_ref_decay), [meta.unary.prop.query] Table 55 (alignment_of, rank, extent), checked over
// cv-qualified types, references, arrays of unknown bound, member pointers and function types,
// including the abominable ones (cv- or ref-qualified function types, [dcl.fct]/6), which are not
// referenceable ([defns.referenceable]: "an object type, a function type that does not have
// cv-qualifiers or a ref-qualifier, or a reference type").
// [meta.trans.sign]: make_signed<T> for T neither a signed nor an unsigned integer type (char,
// wchar_t, char8_t/16_t/32_t, enumerations) is "the signed integer type with smallest rank for
// which sizeof(T) == sizeof(type), with the same cv-qualifiers as T" (likewise make_unsigned).
// [meta.rqmts]: TransformationTraits have a member type; X_t<T> = X<T>::type.
#include <type_traits>
#include <functional>
#include <cstddef>

struct S { int m; };
struct Incomplete;
enum E8 : unsigned char { e8 };
enum class EL : long { el };
enum class ELL : long long { ell };
enum EULL : unsigned long long { eull };
enum EB : bool { eb };
enum class ES : short { es };
enum class SDefault { sd };
enum Unfixed { u0 = -1, u1 = 1 };

template <class A, class B> constexpr bool same = std::is_same_v<A, B>;

// --- cv modifications
static_assert(same<std::remove_const_t<const volatile int>, volatile int>);
static_assert(same<std::remove_const_t<const int*>, const int*>);
static_assert(same<std::remove_const_t<int* const>, int*>);
static_assert(same<std::remove_const_t<const int[3]>, int[3]>);
static_assert(same<std::remove_const_t<const volatile int[][2]>, volatile int[][2]>);
static_assert(same<std::remove_const_t<const int&>, const int&>);
static_assert(same<std::remove_const_t<void() const>, void() const>);
static_assert(same<std::remove_const_t<const void>, void>);
static_assert(same<std::remove_volatile_t<const volatile int>, const int>);
static_assert(same<std::remove_volatile_t<volatile int*>, volatile int*>);
static_assert(same<std::remove_volatile_t<volatile int[4]>, int[4]>);
static_assert(same<std::remove_volatile_t<void() volatile>, void() volatile>);
static_assert(same<std::remove_cv_t<const volatile int>, int>);
static_assert(same<std::remove_cv_t<const volatile int*>, const volatile int*>);
static_assert(same<std::remove_cv_t<int* const volatile>, int*>);
static_assert(same<std::remove_cv_t<const volatile S[2][3]>, S[2][3]>);
static_assert(same<std::remove_cv_t<const int S::* const>, const int S::*>);
static_assert(same<std::remove_cv_t<const volatile void>, void>);
static_assert(same<std::remove_cv_t<const std::nullptr_t>, std::nullptr_t>);
static_assert(same<std::remove_cv_t<void() const volatile>, void() const volatile>);
static_assert(same<std::remove_cv_t<const int&&>, const int&&>);
static_assert(same<std::remove_cv<const Incomplete>::type, Incomplete>);
static_assert(same<std::add_const_t<int>, const int>);
static_assert(same<std::add_const_t<const int>, const int>);
static_assert(same<std::add_const_t<int&>, int&>);
static_assert(same<std::add_const_t<int&&>, int&&>);
static_assert(same<std::add_const_t<void()>, void()>);
static_assert(same<std::add_const_t<void() &>, void() &>);
static_assert(same<std::add_const_t<int[3]>, const int[3]>);
static_assert(same<std::add_const_t<int[]>, const int[]>);
static_assert(same<std::add_const_t<int*>, int* const>);
static_assert(same<std::add_const_t<void>, const void>);
static_assert(same<std::add_volatile_t<int>, volatile int>);
static_assert(same<std::add_volatile_t<int&>, int&>);
static_assert(same<std::add_volatile_t<void() const>, void() const>);
static_assert(same<std::add_volatile_t<int[2]>, volatile int[2]>);
static_assert(same<std::add_cv_t<int>, const volatile int>);
static_assert(same<std::add_cv_t<const int>, const volatile int>);
static_assert(same<std::add_cv_t<int&>, int&>);
static_assert(same<std::add_cv_t<void(int)>, void(int)>);
static_assert(same<std::add_cv_t<int S::*>, int S::* const volatile>);
static_assert(same<std::add_cv_t<Incomplete>, const volatile Incomplete>);

// --- reference modifications
static_assert(same<std::remove_reference_t<int>, int>);
static_assert(same<std::remove_reference_t<int&>, int>);
static_assert(same<std::remove_reference_t<const int&&>, const int>);
static_assert(same<std::remove_reference_t<int (&)[3]>, int[3]>);
static_assert(same<std::remove_reference_t<int (&&)[]>, int[]>);
static_assert(same<std::remove_reference_t<void (&)()>, void()>);
static_assert(same<std::remove_reference_t<void() &>, void() &>);   // not a reference type
static_assert(same<std::remove_reference_t<void() &&>, void() &&>);
static_assert(same<std::remove_reference_t<int* &>, int*>);
static_assert(same<std::add_lvalue_reference_t<int>, int&>);
static_assert(same<std::add_lvalue_reference_t<int&>, int&>);
static_assert(same<std::add_lvalue_reference_t<int&&>, int&>);
static_assert(same<std::add_lvalue_reference_t<const int>, const int&>);
static_assert(same<std::add_lvalue_reference_t<void>, void>);
static_assert(same<std::add_lvalue_reference_t<const volatile void>, const volatile void>);
static_assert(same<std::add_lvalue_reference_t<void()>, void (&)()>);
static_assert(same<std::add_lvalue_reference_t<void() noexcept>, void (&)() noexcept>);
static_assert(same<std::add_lvalue_reference_t<void() const>, void() const>);
static_assert(same<std::add_lvalue_reference_t<void() &>, void() &>);
static_assert(same<std::add_lvalue_reference_t<void() const && noexcept>, void() const && noexcept>);
static_assert(same<std::add_lvalue_reference_t<int[]>, int (&)[]>);
static_assert(same<std::add_lvalue_reference_t<Incomplete>, Incomplete&>);
static_assert(same<std::add_rvalue_reference_t<int>, int&&>);
static_assert(same<std::add_rvalue_reference_t<int&>, int&>);
static_assert(same<std::add_rvalue_reference_t<int&&>, int&&>);
static_assert(same<std::add_rvalue_reference_t<void>, void>);
static_assert(same<std::add_rvalue_reference_t<const void>, const void>);
static_assert(same<std::add_rvalue_reference_t<void()>, void (&&)()>);
static_assert(same<std::add_rvalue_reference_t<void() volatile>, void() volatile>);
static_assert(same<std::add_rvalue_reference_t<void() &&>, void() &&>);
static_assert(same<std::add_rvalue_reference_t<int[2][3]>, int (&&)[2][3]>);

// --- sign modifications
template <class T, std::size_t N = sizeof(T)> using SignedBySize =
    std::conditional_t<sizeof(signed char) == N, signed char,
    std::conditional_t<sizeof(short) == N, short,
    std::conditional_t<sizeof(int) == N, int,
    std::conditional_t<sizeof(long) == N, long, long long>>>>;
template <class T, std::size_t N = sizeof(T)> using UnsignedBySize =
    std::conditional_t<sizeof(unsigned char) == N, unsigned char,
    std::conditional_t<sizeof(unsigned short) == N, unsigned short,
    std::conditional_t<sizeof(unsigned) == N, unsigned,
    std::conditional_t<sizeof(unsigned long) == N, unsigned long, unsigned long long>>>>;

template <class T, class Sg, class Un> constexpr bool sign_ok() {
  static_assert(same<std::make_signed_t<T>, Sg>);
  static_assert(same<std::make_unsigned_t<T>, Un>);
  static_assert(same<std::make_signed_t<const T>, const Sg>);
  static_assert(same<std::make_unsigned_t<const T>, const Un>);
  static_assert(same<std::make_signed_t<volatile T>, volatile Sg>);
  static_assert(same<std::make_unsigned_t<volatile T>, volatile Un>);
  static_assert(same<std::make_signed_t<const volatile T>, const volatile Sg>);
  static_assert(same<typename std::make_unsigned<const volatile T>::type, const volatile Un>);
  return true;
}
static_assert(sign_ok<signed char, signed char, unsigned char>());
static_assert(sign_ok<unsigned char, signed char, unsigned char>());
static_assert(sign_ok<char, signed char, unsigned char>());   // char is neither signed nor unsigned integer type
static_assert(sign_ok<short, short, unsigned short>());
static_assert(sign_ok<unsigned short, short, unsigned short>());
static_assert(sign_ok<int, int, unsigned>());
static_assert(sign_ok<unsigned, int, unsigned>());
static_assert(sign_ok<long, long, unsigned long>());
static_assert(sign_ok<unsigned long, long, unsigned long>());
static_assert(sign_ok<long long, long long, unsigned long long>());
static_assert(sign_ok<unsigned long long, long long, unsigned long long>());
static_assert(sign_ok<wchar_t, SignedBySize<wchar_t>, UnsignedBySize<wchar_t>>());
static_assert(sign_ok<char8_t, signed char, unsigned char>());
static_assert(sign_ok<char16_t, SignedBySize<char16_t>, UnsignedBySize<char16_t>>());
static_assert(sign_ok<char32_t, SignedBySize<char32_t>, UnsignedBySize<char32_t>>());
static_assert(sign_ok<E8, signed char, unsigned char>());
static_assert(sign_ok<ES, short, unsigned short>());
static_assert(sign_ok<SDefault, int, unsigned>());
static_assert(sign_ok<Unfixed, SignedBySize<Unfixed>, UnsignedBySize<Unfixed>>());
static_assert(sign_ok<EB, signed char, unsigned char>());   // an enumeration, not cv bool
// Smallest rank with that size: long (rank below long long) when both are 8 bytes, even when the
// underlying type is long long / unsigned long long.
static_assert(sign_ok<EL, SignedBySize<EL>, UnsignedBySize<EL>>());
static_assert(sign_ok<ELL, SignedBySize<ELL>, UnsignedBySize<ELL>>());
static_assert(sign_ok<EULL, SignedBySize<EULL>, UnsignedBySize<EULL>>());
static_assert(sizeof(long) != sizeof(long long) || same<std::make_signed_t<ELL>, long>);
static_assert(sizeof(long) != sizeof(long long) || same<std::make_unsigned_t<EULL>, unsigned long>);

// --- array modifications
static_assert(same<std::remove_extent_t<int>, int>);
static_assert(same<std::remove_extent_t<int[2]>, int>);
static_assert(same<std::remove_extent_t<int[2][3]>, int[3]>);
static_assert(same<std::remove_extent_t<int[][3]>, int[3]>);
static_assert(same<std::remove_extent_t<const int[][3]>, const int[3]>);
static_assert(same<std::remove_extent_t<int (&)[3]>, int (&)[3]>);
static_assert(same<std::remove_extent_t<int (*)[3]>, int (*)[3]>);
static_assert(same<std::remove_extent_t<Incomplete[]>, Incomplete>);
static_assert(same<std::remove_all_extents_t<int>, int>);
static_assert(same<std::remove_all_extents_t<int[2]>, int>);
static_assert(same<std::remove_all_extents_t<int[2][3]>, int>);
static_assert(same<std::remove_all_extents_t<volatile int[][3][4]>, volatile int>);
static_assert(same<std::remove_all_extents_t<int (&)[3]>, int (&)[3]>);
static_assert(same<std::remove_all_extents_t<int* [3][4]>, int*>);

// --- pointer modifications
static_assert(same<std::remove_pointer_t<int>, int>);
static_assert(same<std::remove_pointer_t<int*>, int>);
static_assert(same<std::remove_pointer_t<int* const>, int>);
static_assert(same<std::remove_pointer_t<int* volatile>, int>);
static_assert(same<std::remove_pointer_t<int* const volatile>, int>);
static_assert(same<std::remove_pointer_t<const int*>, const int>);
static_assert(same<std::remove_pointer_t<int**>, int*>);
static_assert(same<std::remove_pointer_t<int*&>, int*&>);   // a reference, not a pointer
static_assert(same<std::remove_pointer_t<void (*)(int)>, void(int)>);
static_assert(same<std::remove_pointer_t<void (*)() noexcept>, void() noexcept>);
static_assert(same<std::remove_pointer_t<int S::*>, int S::*>);
static_assert(same<std::remove_pointer_t<void (S::*)()>, void (S::*)()>);
static_assert(same<std::remove_pointer_t<int (*)[]>, int[]>);
static_assert(same<std::remove_pointer_t<std::nullptr_t>, std::nullptr_t>);
static_assert(same<std::add_pointer_t<int>, int*>);
static_assert(same<std::add_pointer_t<int&>, int*>);
static_assert(same<std::add_pointer_t<const int&&>, const int*>);
static_assert(same<std::add_pointer_t<int*>, int**>);
static_assert(same<std::add_pointer_t<void>, void*>);
static_assert(same<std::add_pointer_t<const volatile void>, const volatile void*>);
static_assert(same<std::add_pointer_t<void()>, void (*)()>);
static_assert(same<std::add_pointer_t<void (&)()>, void (*)()>);
static_assert(same<std::add_pointer_t<void(...) noexcept>, void (*)(...) noexcept>);
static_assert(same<std::add_pointer_t<void() const>, void() const>);
static_assert(same<std::add_pointer_t<void() &>, void() &>);
static_assert(same<std::add_pointer_t<void() volatile && noexcept>, void() volatile && noexcept>);
static_assert(same<std::add_pointer_t<int[]>, int (*)[]>);
static_assert(same<std::add_pointer_t<Incomplete>, Incomplete*>);
static_assert(same<std::add_pointer_t<int S::*>, int S::**>);

// --- other transformations
static_assert(same<std::type_identity_t<int>, int>);
static_assert(same<std::type_identity_t<void() const &>, void() const &>);
static_assert(same<std::type_identity_t<const int (&)[]>, const int (&)[]>);
static_assert(same<std::type_identity_t<Incomplete>, Incomplete>);
static_assert(same<std::remove_cvref_t<const volatile int&>, int>);
static_assert(same<std::remove_cvref_t<const int&&>, int>);
static_assert(same<std::remove_cvref_t<const int (&)[3]>, int[3]>);
static_assert(same<std::remove_cvref_t<const int*&>, const int*>);
static_assert(same<std::remove_cvref_t<int* const&>, int*>);
static_assert(same<std::remove_cvref_t<void (&)()>, void()>);
static_assert(same<std::remove_cvref_t<void() const &>, void() const &>);
static_assert(same<std::remove_cvref_t<const void>, void>);
static_assert(same<std::decay_t<int>, int>);
static_assert(same<std::decay_t<const volatile int>, int>);
static_assert(same<std::decay_t<const int&>, int>);
static_assert(same<std::decay_t<volatile S&&>, S>);
static_assert(same<std::decay_t<int[3]>, int*>);
static_assert(same<std::decay_t<int[]>, int*>);
static_assert(same<std::decay_t<const int[2][3]>, const int (*)[3]>);
static_assert(same<std::decay_t<const int (&)[3]>, const int*>);
static_assert(same<std::decay_t<int (&&)[]>, int*>);
static_assert(same<std::decay_t<Incomplete[]>, Incomplete*>);
static_assert(same<std::decay_t<void(int)>, void (*)(int)>);
static_assert(same<std::decay_t<void (&)(int)>, void (*)(int)>);
static_assert(same<std::decay_t<void (&&)() noexcept>, void (*)() noexcept>);
static_assert(same<std::decay_t<void() const>, void() const>);   // add_pointer_t of an abominable type
static_assert(same<std::decay_t<void() &&>, void() &&>);
static_assert(same<std::decay_t<int* const>, int*>);
static_assert(same<std::decay_t<const int* const&>, const int*>);
static_assert(same<std::decay_t<int S::* const>, int S::*>);
static_assert(same<std::decay_t<void>, void>);
static_assert(same<std::decay_t<const volatile void>, void>);
static_assert(same<std::decay_t<const std::nullptr_t&>, std::nullptr_t>);
static_assert(same<std::decay_t<Incomplete&>, Incomplete>);
static_assert(same<std::enable_if_t<true>, void>);
static_assert(same<std::enable_if_t<true, int&>, int&>);
template <class T> concept HasType = requires { typename T::type; };
static_assert(!HasType<std::enable_if<false>>);
static_assert(!HasType<std::enable_if<false, int>>);
static_assert(same<std::conditional_t<true, int, void>, int>);
static_assert(same<std::conditional_t<false, int, void() const>, void() const>);
static_assert(same<std::conditional_t<true, Incomplete, void>, Incomplete>);
static_assert(same<std::underlying_type_t<E8>, unsigned char>);
static_assert(same<std::underlying_type_t<EL>, long>);
static_assert(same<std::underlying_type_t<ELL>, long long>);
static_assert(same<std::underlying_type_t<EB>, bool>);
static_assert(same<std::underlying_type_t<SDefault>, int>);   // [dcl.enum]/5: scoped enum without enum-base: int
static_assert(std::is_integral_v<std::underlying_type_t<Unfixed>>);
static_assert(std::is_signed_v<std::underlying_type_t<Unfixed>>);   // must represent -1
static_assert(!HasType<std::underlying_type<int>>);
static_assert(!HasType<std::underlying_type<S>>);
static_assert(!HasType<std::underlying_type<void>>);
static_assert(!HasType<std::underlying_type<int&>>);
static_assert(!HasType<std::underlying_type<E8&>>);
static_assert(!HasType<std::underlying_type<Incomplete>>);
static_assert(!HasType<std::underlying_type<void() const>>);
static_assert(same<std::unwrap_reference_t<int>, int>);
static_assert(same<std::unwrap_reference_t<std::reference_wrapper<int>>, int&>);
static_assert(same<std::unwrap_reference_t<std::reference_wrapper<const S>>, const S&>);
static_assert(same<std::unwrap_reference_t<std::reference_wrapper<int>&>, std::reference_wrapper<int>&>);
static_assert(same<std::unwrap_reference_t<const std::reference_wrapper<int>>, const std::reference_wrapper<int>>);
static_assert(same<std::unwrap_reference_t<std::reference_wrapper<int (&)()>>, int (&)()>);
static_assert(same<std::unwrap_ref_decay_t<const std::reference_wrapper<int>&>, int&>);
static_assert(same<std::unwrap_ref_decay_t<std::reference_wrapper<int>&&>, int&>);
static_assert(same<std::unwrap_ref_decay_t<const int (&)[3]>, const int*>);
static_assert(same<std::unwrap_ref_decay_t<volatile int>, int>);

// --- property queries: integral_constant<size_t, v> base, _v equals ::value
template <class Q, std::size_t V> constexpr bool query_ok =
    std::is_base_of_v<std::integral_constant<std::size_t, V>, Q> && Q::value == V &&
    same<typename Q::value_type, std::size_t>;
static_assert(query_ok<std::rank<int>, 0>);
static_assert(query_ok<std::rank<int[]>, 1>);
static_assert(query_ok<std::rank<int[][3][4]>, 3>);
static_assert(query_ok<std::rank<const int[2][3]>, 2>);
static_assert(query_ok<std::rank<int (&)[3]>, 0>);
static_assert(query_ok<std::rank<int (*)[3]>, 0>);
static_assert(query_ok<std::rank<void>, 0>);
static_assert(query_ok<std::rank<void() const>, 0>);
static_assert(std::rank_v<int[1][2][3][4]> == 4);
static_assert(query_ok<std::extent<int>, 0>);
static_assert(query_ok<std::extent<int[]>, 0>);
static_assert(query_ok<std::extent<int[][4]>, 0>);
static_assert(query_ok<std::extent<int[][4], 1>, 4>);
static_assert(query_ok<std::extent<int[2]>, 2>);
static_assert(query_ok<std::extent<int[2], 0>, 2>);
static_assert(query_ok<std::extent<int[2], 1>, 0>);
static_assert(query_ok<std::extent<int[2][3][4], 2>, 4>);
static_assert(query_ok<std::extent<int[2][3][4], 3>, 0>);
static_assert(query_ok<std::extent<const volatile int[5]>, 5>);
static_assert(query_ok<std::extent<int (&)[3]>, 0>);
static_assert(std::extent_v<int[5][6], 1> == 6 && std::extent_v<int[5][6]> == 5);
static_assert(query_ok<std::alignment_of<int>, alignof(int)>);
static_assert(query_ok<std::alignment_of<const double>, alignof(double)>);
static_assert(query_ok<std::alignment_of<int&>, alignof(int&)>);
static_assert(query_ok<std::alignment_of<int[]>, alignof(int)>);
static_assert(query_ok<std::alignment_of<long double[3][4]>, alignof(long double)>);
struct alignas(64) Over { char c; };
static_assert(query_ok<std::alignment_of<Over>, 64>);
static_assert(query_ok<std::alignment_of<Over&&>, 64>);
static_assert(std::alignment_of_v<char> == 1);

int main() {}
