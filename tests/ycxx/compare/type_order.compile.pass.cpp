// [compare.type]: type_order / type_order_v.
// /1: "There is an implementation-defined total ordering of all types. For any (possibly
// incomplete) types X and Y, ... TYPE-ORDER(X, Y) is a constant expression of type
// strong_ordering ... strong_ordering::equal if they are the same type." Note 1: "int,
// const int and int& are different types." /3: may be instantiated with incomplete types.
#include <compare>
#include <cstddef>
#include <type_traits>

struct Incomplete;
struct Complete { int x; };
template <class T> struct Tmpl {};
enum class Enum : int {};
union Union;
using IntAlias = int;
typedef Complete CompleteAlias;

// Member layout of type_order.
static_assert(std::is_same_v<std::type_order<int, long>::value_type, std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::type_order<int, long>::value), const std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::type_order_v<int, long>), const std::strong_ordering>);
static_assert(std::type_order_v<int, long> == std::type_order<int, long>::value);
static_assert(std::type_order<int, long>{}() == std::type_order_v<int, long>);
static_assert(static_cast<std::strong_ordering>(std::type_order<int, long>{}) == std::type_order_v<int, long>);
static_assert(noexcept(std::type_order<int, long>{}()));
static_assert(noexcept(static_cast<std::strong_ordering>(std::type_order<int, long>{})));
// Usable as a constant expression in a template argument.
template <bool B> struct Holder {};
[[maybe_unused]] Holder<(std::type_order_v<char, int> < 0)> holder;

// Equal exactly for the same type, including through aliases.
static_assert(std::type_order_v<int, int> == std::strong_ordering::equal);
static_assert(std::type_order_v<IntAlias, int> == std::strong_ordering::equal);
static_assert(std::type_order_v<signed int, int> == std::strong_ordering::equal);
static_assert(std::type_order_v<CompleteAlias, Complete> == std::strong_ordering::equal);
static_assert(std::type_order_v<Incomplete, Incomplete> == std::strong_ordering::equal);
static_assert(std::type_order_v<std::remove_const_t<const int>, int> == std::strong_ordering::equal);
static_assert(std::type_order_v<decltype(nullptr), std::nullptr_t> == std::strong_ordering::equal);
static_assert(std::type_order_v<int, const int> != std::strong_ordering::equal);
static_assert(std::type_order_v<int, int&> != std::strong_ordering::equal);
static_assert(std::type_order_v<const int, int&> != std::strong_ordering::equal);
static_assert(std::type_order_v<int&, int&&> != std::strong_ordering::equal);
static_assert(std::type_order_v<long, long long> != std::strong_ordering::equal);
static_assert(std::type_order_v<char, signed char> != std::strong_ordering::equal);
static_assert(std::type_order_v<void(), void() noexcept> != std::strong_ordering::equal);

// Total order checks over a set of distinct types: the sign matrix must be antisymmetric,
// zero only on the diagonal, and transitive.
constexpr int sign(std::strong_ordering o) { return o < 0 ? -1 : o > 0 ? 1 : 0; }

template <class A, class... Ts>
constexpr void fill_row(int* row) {
  int j = 0;
  ((row[j++] = sign(std::type_order_v<A, Ts>)), ...);
}

template <class... Ts>
constexpr bool is_total_order() {
  constexpr int n = sizeof...(Ts);
  int m[n][n] = {};
  int i = 0;
  (fill_row<Ts, Ts...>(m[i++]), ...);
  for (int a = 0; a < n; ++a)
    for (int b = 0; b < n; ++b) {
      if ((a == b) != (m[a][b] == 0)) return false;     // equal iff same type
      if (m[a][b] != -m[b][a]) return false;            // antisymmetry
      for (int c = 0; c < n; ++c)                       // transitivity
        if (m[a][b] < 0 && m[b][c] < 0 && m[a][c] >= 0) return false;
    }
  return true;
}

static_assert(is_total_order<int, const int, volatile int, const volatile int, int&, int&&,
                             const int&, int*, int const*, int[], int[3], int[2][3], int(),
                             int (*)(), int (&)(), long, unsigned, char, signed char, unsigned char,
                             void, const void, std::nullptr_t, double, Incomplete, Incomplete*,
                             Complete, Tmpl<int>, Tmpl<long>, Tmpl<Incomplete>, Enum, Union,
                             int Complete::*, int (Complete::*)()>());

// The relation is consistent no matter in which order the questions are asked.
static_assert((std::type_order_v<Incomplete, int> < 0) == (std::type_order_v<int, Incomplete> > 0));

