// [expected.syn]: struct unexpect_t { explicit unexpect_t() = default; };
// inline constexpr unexpect_t unexpect{};
#include <expected>
#include <type_traits>

static_assert(std::is_same_v<decltype(std::unexpect), const std::unexpect_t>);
static_assert(std::is_default_constructible_v<std::unexpect_t>);
static_assert(std::is_empty_v<std::unexpect_t>);
// explicit default constructor: copy-list-initialization from {} is ill-formed
template <class T> void take(T);
template <class T> concept copy_list_init_ok = requires { take<T>({}); };
static_assert(!copy_list_init_ok<std::unexpect_t>);
static_assert(std::is_trivially_copyable_v<std::unexpect_t>);
constexpr std::unexpect_t u{};
