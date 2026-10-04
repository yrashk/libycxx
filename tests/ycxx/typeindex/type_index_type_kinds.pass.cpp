// [type.index]/2: type_index(const type_info& rhs) is "the equivalent of target = &rhs"; /3 ==
// is *target == *rhs.target, /8 <=> is equal exactly when the type_infos compare equal.
// [expr.typeid]/5: "If the type of the type-id is a reference to a possibly cv-qualified
// type, the result of the typeid expression refers to a std::type_info object representing
// the cv-unqualified referenced type. If the type of the type-id is a possibly cv-qualified
// type, the result ... represent[s] the cv-unqualified type." Only *top-level* cv and
// references are dropped. So type_index distinguishes all of these kinds of types.
#include <typeindex>
#include <compare>
#include <cstring>
#include "check.hpp"

struct A {};
struct B {};
template <class T>
struct Tmpl {};
enum E1 { x1 };
enum class E2 { x2 };
union U {
  int i;
};

template <class T>
std::type_index ti() {
  return std::type_index(typeid(T));
}

#define DISTINCT(...)                                                                      \
  do {                                                                                     \
    std::type_index list[] = {__VA_ARGS__};                                                \
    constexpr int n = sizeof(list) / sizeof(list[0]);                                      \
    for (int i = 0; i < n; ++i)                                                            \
      for (int j = 0; j < n; ++j) {                                                        \
        CHECK((list[i] == list[j]) == (i == j));                                           \
        CHECK(((list[i] <=> list[j]) == 0) == (i == j));                                   \
        CHECK((list[i] < list[j]) != (list[j] < list[i]) || i == j);                       \
      }                                                                                    \
  } while (0)

int main() {
  // top-level cv and references are dropped
  CHECK(ti<int>() == ti<const int>());
  CHECK(ti<int>() == ti<volatile int>());
  CHECK(ti<int>() == ti<int&>());
  CHECK(ti<int>() == ti<const int&&>());
  CHECK(ti<A>() == ti<const volatile A&>());
  CHECK(ti<int*>() == ti<int* const>());

  // but not cv below the top level
  DISTINCT(ti<int*>(), ti<const int*>(), ti<volatile int*>(), ti<int**>(), ti<int* const*>());

  // arrays, functions, member pointers
  DISTINCT(ti<int[3]>(), ti<int[4]>(), ti<int[]>(), ti<int (*)[3]>(), ti<int*>());
  DISTINCT(ti<void()>(), ti<void() noexcept>(), ti<int()>(), ti<void(int)>(), ti<void (*)()>(),
           ti<void (**)()>());
  CHECK(ti<void (&)()>() == ti<void()>());
  DISTINCT(ti<int A::*>(), ti<int B::*>(), ti<long A::*>(), ti<void (A::*)()>(),
           ti<void (A::*)() const>());

  // class kinds and template specializations
  DISTINCT(ti<A>(), ti<B>(), ti<Tmpl<int>>(), ti<Tmpl<long>>(), ti<Tmpl<A>>(), ti<E1>(),
           ti<E2>(), ti<U>(), ti<void>(), ti<decltype(nullptr)>());

  // character and integer types are all distinct
  DISTINCT(ti<char>(), ti<signed char>(), ti<unsigned char>(), ti<wchar_t>(), ti<char8_t>(),
           ti<char16_t>(), ti<char32_t>(), ti<short>(), ti<int>(), ti<long>(), ti<long long>(),
           ti<unsigned>(), ti<bool>(), ti<float>(), ti<double>(), ti<long double>());

  // closure types are unique
  auto l1 = [] {};
  auto l2 = [] {};
  CHECK(ti<decltype(l1)>() != ti<decltype(l2)>());

  // a local class
  struct Local {};
  CHECK(ti<Local>() != ti<A>());
  CHECK(ti<Local>() == std::type_index(typeid(Local)));

  // name() and hash_code() follow the type_info (/9, /10) for any kind of type
  CHECK(std::strcmp(ti<int[3]>().name(), typeid(int[3]).name()) == 0);
  CHECK(ti<const int&>().hash_code() == typeid(int).hash_code());
  return 0;
}
