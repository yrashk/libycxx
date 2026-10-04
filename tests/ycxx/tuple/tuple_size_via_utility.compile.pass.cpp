// [tuple.helper]/6 (tuple_size<const T>), /9 (tuple_element<I, const T>) and [tuple.syn]: "In
// addition to being available via inclusion of the <tuple> header, the template is available when
// any of the headers <array>, <complex>, <ranges>, or <utility> are included." Here only <utility>.
#include <utility>
#include <cstddef>

struct Mine {};
template <> struct std::tuple_size<Mine> { static constexpr std::size_t value = 4; };
template <std::size_t I> struct std::tuple_element<I, Mine> { using type = int; };

static_assert(std::tuple_size<const Mine>::value == 4);
static_assert(std::tuple_size_v<const Mine> == 4);
static_assert(sizeof(std::tuple_element_t<1, const Mine>) == sizeof(int));
static_assert(std::tuple_element<3, const Mine>::type() == 0);

int main() {}
