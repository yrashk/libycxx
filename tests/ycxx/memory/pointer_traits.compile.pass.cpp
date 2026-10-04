// [pointer.traits.general], [pointer.traits.types], [pointer.traits.functions]:
// pointer_traits<T*> members; for a class Ptr, element_type is Ptr::element_type or the first
// template argument of SomePointer<T, Args...>; difference_type defaults to ptrdiff_t; rebind
// is Ptr::rebind<U> or SomePointer<U, Args...>. "If Ptr satisfies has-elem-type, a
// specialization ... has the following members ...; otherwise, such a specialization has no
// members by any of those names." pointer_to for T* is constexpr noexcept and returns
// addressof(r). "A specialization generated from the pointer_traits primary template has no
// member [to_address]."
#include <memory>
#include <cstddef>
#include <type_traits>

template <class T, class Tag = void>
struct Fancy {
  T* p;
  static Fancy pointer_to(T& r) { return Fancy{&r}; }
};
struct WithMembers {
  using element_type = char;
  using difference_type = short;
  template <class U>
  using rebind = U*;
};
struct NoElem {};
template <int N>
struct NonTypeArg {};  // not of the form SomePointer<T, Args...> with type parameters

template <class T>
concept has_element_type = requires { typename std::pointer_traits<T>::element_type; };
template <class T>
concept has_pointer = requires { typename std::pointer_traits<T>::pointer; };
template <class T>
concept has_difference_type = requires { typename std::pointer_traits<T>::difference_type; };
template <class T>
concept has_to_address = requires(T p) { std::pointer_traits<T>::to_address(p); };

using PI = std::pointer_traits<int*>;
static_assert(std::is_same_v<PI::pointer, int*>);
static_assert(std::is_same_v<PI::element_type, int>);
static_assert(std::is_same_v<PI::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<PI::rebind<long>, long*>);
static_assert(std::is_same_v<std::pointer_traits<const int*>::element_type, const int>);
static_assert(std::is_same_v<std::pointer_traits<void*>::element_type, void>);
static_assert(noexcept(PI::pointer_to(std::declval<int&>())));

constexpr int value = 7;
static_assert(std::pointer_traits<const int*>::pointer_to(value) == &value);

using PF = std::pointer_traits<Fancy<int, double>>;
static_assert(std::is_same_v<PF::pointer, Fancy<int, double>>);
static_assert(std::is_same_v<PF::element_type, int>);
static_assert(std::is_same_v<PF::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<PF::rebind<long>, Fancy<long, double>>);
static_assert(std::is_same_v<decltype(PF::pointer_to(std::declval<int&>())), Fancy<int, double>>);
static_assert(!has_to_address<Fancy<int>>);

using PW = std::pointer_traits<WithMembers>;
static_assert(std::is_same_v<PW::element_type, char>);
static_assert(std::is_same_v<PW::difference_type, short>);
static_assert(std::is_same_v<PW::rebind<int>, int*>);

// SFINAE-friendly: no element type -> no members at all
static_assert(!has_element_type<NoElem>);
static_assert(!has_pointer<NoElem>);
static_assert(!has_difference_type<NoElem>);
static_assert(!has_pointer<NonTypeArg<3>>);
static_assert(!has_element_type<int>);
