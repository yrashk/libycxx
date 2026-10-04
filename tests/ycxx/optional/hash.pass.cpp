// [optional.hash]: hash<optional<T>> enabled iff hash<remove_const_t<T>> is enabled; when
// engaged, the hash equals hash<remove_const_t<T>>()(*o). [unord.hash]/4: disabled
// specializations are not default/copy/move constructible or assignable.
#include <optional>
#include <functional>
#include <type_traits>
#include "check.hpp"

struct NoHash {};
struct Hashed { int v; bool operator==(const Hashed&) const = default; };
template <> struct std::hash<Hashed> {
  std::size_t operator()(const Hashed& h) const noexcept { return static_cast<std::size_t>(h.v) * 31u; }
};

template <class T>
constexpr bool enabled = std::is_default_constructible_v<std::hash<T>> && std::is_copy_constructible_v<std::hash<T>> &&
                         std::is_move_constructible_v<std::hash<T>> && std::is_copy_assignable_v<std::hash<T>> &&
                         std::is_move_assignable_v<std::hash<T>>;
template <class T>
constexpr bool disabled = !std::is_default_constructible_v<std::hash<T>> && !std::is_copy_constructible_v<std::hash<T>> &&
                          !std::is_move_constructible_v<std::hash<T>> && !std::is_copy_assignable_v<std::hash<T>> &&
                          !std::is_move_assignable_v<std::hash<T>>;

static_assert(enabled<std::optional<int>>);
static_assert(enabled<std::optional<const int>>);
static_assert(enabled<std::optional<Hashed>>);
static_assert(enabled<std::optional<const Hashed>>);
static_assert(disabled<std::optional<NoHash>>);
static_assert(disabled<std::optional<const NoHash>>);

int main() {
  std::optional<Hashed> h(Hashed{4});
  CHECK(std::hash<std::optional<Hashed>>{}(h) == std::hash<Hashed>{}(Hashed{4}));
  std::optional<const Hashed> ch(Hashed{5});
  CHECK(std::hash<std::optional<const Hashed>>{}(ch) == std::hash<Hashed>{}(Hashed{5}));
  std::optional<int> i(42);
  CHECK(std::hash<std::optional<int>>{}(i) == std::hash<int>{}(42));
  std::optional<int> e1, e2;
  CHECK(std::hash<std::optional<int>>{}(e1) == std::hash<std::optional<int>>{}(e2));
  return 0;
}
