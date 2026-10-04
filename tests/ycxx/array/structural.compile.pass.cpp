// [array.overview]/4: array<T, N> is a structural type if T is; two values are
// template-argument-equivalent iff corresponding elements are.
#include <array>
#include <type_traits>

template <std::array<int, 3> A> struct Tag { static constexpr int first = A[0]; };

static_assert(Tag<std::array<int, 3>{1, 2, 3}>::first == 1);
static_assert(std::is_same_v<Tag<std::array<int, 3>{1, 2, 3}>, Tag<std::array<int, 3>{1, 2, 3}>>);
static_assert(!std::is_same_v<Tag<std::array<int, 3>{1, 2, 3}>, Tag<std::array<int, 3>{1, 2, 4}>>);

template <std::array<int, 0> Z> struct Empty {};
Empty<std::array<int, 0>{}> e;
