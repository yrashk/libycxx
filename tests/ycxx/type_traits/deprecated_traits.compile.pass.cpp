// FLAGS: -Wno-deprecated-declarations -Wno-deprecated
// [depr.meta.types] (Annex D, normative; [depr.general]: deprecated features remain part of the
// standard): <type_traits> also declares is_trivial, is_trivial_v, is_pod, is_pod_v,
// aligned_storage, aligned_storage_t, aligned_union, aligned_union_t.
// /3 "A trivial class is a class that is trivially copyable and has one or more eligible default
// constructors, all of which are trivial." "A trivial type is a scalar type, a trivial class, an
// array of such a type, or a cv-qualified version of one of these types."
// /4 "A POD class is a class that is both a trivial class and a standard-layout class, and has
// no non-static data members of type non-POD class (or array thereof). A POD type is a scalar
// type, a POD class, an array of such a type, or a cv-qualified version of one of these types."
// /6, /9: Cpp17UnaryTypeTraits with base true_type / false_type.
// /11-13: aligned_storage<Len, Align = default-alignment>: default-alignment is "the most
// stringent alignment requirement for any object type whose size is no greater than Len"; type is
// "a trivial standard-layout type suitable for use as uninitialized storage for any object whose
// size is at most Len and whose alignment is a divisor of Align".
// /17: aligned_union<Len, Types...>::type: trivial standard-layout, storage for any of Types,
// size at least Len; alignment_value is a size_t integral constant: the strictest alignment.
#include <type_traits>
#include <cstddef>

struct Triv { int i; };
struct NonTrivDefault { int i = 0; };            // non-trivial default constructor
struct TrivNotSL { int a; private: int b; };     // trivial, not standard-layout
struct PodWithNonPodMember { TrivNotSL m; };     // standard-layout? no (member not SL) -> not POD
struct UserCopy { UserCopy() = default; UserCopy(const UserCopy&); };
struct DeletedDefault { DeletedDefault() = delete; };   // no eligible trivial default ctor... deleted
struct Poly { virtual void f(); };
enum E { e };

template <class T> constexpr bool triv = std::is_trivial_v<T> && std::is_base_of_v<std::true_type, std::is_trivial<T>>;
template <class T> constexpr bool ntriv = !std::is_trivial_v<T> && std::is_base_of_v<std::false_type, std::is_trivial<T>>;
template <class T> constexpr bool pod = std::is_pod_v<T> && std::is_base_of_v<std::true_type, std::is_pod<T>>;
template <class T> constexpr bool npod = !std::is_pod_v<T> && std::is_base_of_v<std::false_type, std::is_pod<T>>;

static_assert(triv<int> && triv<const volatile int> && triv<int*> && triv<E> && triv<std::nullptr_t> && triv<int Triv::*>);
static_assert(triv<Triv> && triv<const Triv> && triv<Triv[3]> && triv<Triv[]> && triv<TrivNotSL>);
static_assert(ntriv<NonTrivDefault> && ntriv<UserCopy> && ntriv<Poly> && ntriv<int&> && ntriv<void> && ntriv<void()>);
static_assert(ntriv<NonTrivDefault[2]>);
static_assert(pod<int> && pod<Triv> && pod<Triv[2][2]> && pod<const E> && pod<int*>);
static_assert(npod<TrivNotSL> && npod<PodWithNonPodMember> && npod<NonTrivDefault> && npod<Poly> && npod<int&> && npod<void>);

// aligned_storage
template <std::size_t Len, std::size_t Align> constexpr bool storage_ok() {
  using T = std::aligned_storage_t<Len, Align>;
  static_assert(std::is_same_v<T, typename std::aligned_storage<Len, Align>::type>);
  static_assert(std::is_trivial_v<T> && std::is_standard_layout_v<T>);
  static_assert(sizeof(T) >= Len && alignof(T) % Align == 0);
  return true;
}
static_assert(storage_ok<1, 1>() && storage_ok<3, 2>() && storage_ok<10, 8>() && storage_ok<64, 64>());
static_assert(storage_ok<1, alignof(std::max_align_t)>() && storage_ok<100, 16>());
// Default alignment: at least that of every object type of size <= Len.
static_assert(alignof(std::aligned_storage_t<sizeof(double)>) >= alignof(double));
static_assert(alignof(std::aligned_storage_t<sizeof(long double)>) >= alignof(long double));
static_assert(alignof(std::aligned_storage_t<1>) >= 1 && sizeof(std::aligned_storage_t<1>) >= 1);
static_assert(alignof(std::aligned_storage_t<4>) >= alignof(int));
static_assert(alignof(std::aligned_storage_t<64>) >= alignof(std::max_align_t));
static_assert(std::is_trivial_v<std::aligned_storage_t<7>> && std::is_standard_layout_v<std::aligned_storage_t<7>>);

// aligned_union
struct alignas(32) Over { char c; };
using AU = std::aligned_union<0, char, double, Over>;
static_assert(std::is_same_v<decltype(AU::alignment_value), const std::size_t>);
static_assert(AU::alignment_value == 32);
static_assert(sizeof(AU::type) >= sizeof(Over) && sizeof(AU::type) >= sizeof(double) && alignof(AU::type) % 32 == 0);
static_assert(std::is_trivial_v<AU::type> && std::is_standard_layout_v<AU::type>);
using AU2 = std::aligned_union<100, char>;
static_assert(AU2::alignment_value == 1 && sizeof(std::aligned_union_t<100, char>) >= 100);
static_assert(std::aligned_union<4, short, int>::alignment_value == alignof(int));
static_assert(sizeof(std::aligned_union_t<1, int[5]>) >= sizeof(int[5]));

int main() {}
