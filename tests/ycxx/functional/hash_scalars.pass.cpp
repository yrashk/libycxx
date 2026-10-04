// [unord.hash]/2: "Each specialization of hash is either enabled or disabled". /3-4: the
// library provides enabled specializations for nullptr_t, all cv-unqualified arithmetic,
// enumeration, and pointer types; a specialization that is not explicitly or partially
// specialized is disabled, and disabled specializations are not default constructible,
// copy/move constructible or assignable. /5: enabled specializations meet Cpp17Hash and are
// function objects whose operator() gives equal results for equal arguments.
#include <functional>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

enum E { e0, e1 };
enum class SE : char { a, b };
struct NoHash {};

template <class T>
constexpr bool enabled = std::is_default_constructible_v<std::hash<T>> && std::is_copy_constructible_v<std::hash<T>> &&
                         std::is_move_constructible_v<std::hash<T>> && std::is_copy_assignable_v<std::hash<T>> &&
                         std::is_move_assignable_v<std::hash<T>> && std::is_destructible_v<std::hash<T>> &&
                         std::is_same_v<std::invoke_result_t<const std::hash<T>&, const T&>, std::size_t>;
template <class T>
constexpr bool disabled = !std::is_default_constructible_v<std::hash<T>> && !std::is_copy_constructible_v<std::hash<T>> &&
                          !std::is_move_constructible_v<std::hash<T>> && !std::is_copy_assignable_v<std::hash<T>> &&
                          !std::is_move_assignable_v<std::hash<T>> && !std::is_invocable_v<std::hash<T>, const T&>;

static_assert(enabled<bool>);
static_assert(enabled<char>);
static_assert(enabled<signed char>);
static_assert(enabled<unsigned char>);
static_assert(enabled<char8_t>);
static_assert(enabled<char16_t>);
static_assert(enabled<char32_t>);
static_assert(enabled<wchar_t>);
static_assert(enabled<short>);
static_assert(enabled<unsigned long long>);
static_assert(enabled<float>);
static_assert(enabled<double>);
static_assert(enabled<long double>);
static_assert(enabled<std::nullptr_t>);
static_assert(enabled<int*>);
static_assert(enabled<const volatile void*>);
static_assert(enabled<void (*)()>);
static_assert(enabled<E>);
static_assert(enabled<SE>);
static_assert(disabled<NoHash>);

int main() {
  CHECK(std::hash<int>{}(42) == std::hash<int>{}(42));
  int x = 0;
  CHECK(std::hash<int*>{}(&x) == std::hash<int*>{}(&x));
  CHECK(std::hash<std::nullptr_t>{}(nullptr) == std::hash<std::nullptr_t>{}(nullptr));
  CHECK(std::hash<E>{}(e1) == std::hash<E>{}(e1));
  CHECK(std::hash<SE>{}(SE::b) == std::hash<SE>{}(SE::b));
  // +0.0 == -0.0, so their hashes must be equal (Cpp17Hash: k1 == k2 implies h(k1) == h(k2))
  CHECK(std::hash<double>{}(0.0) == std::hash<double>{}(-0.0));
  CHECK(std::hash<float>{}(0.0f) == std::hash<float>{}(-0.0f));
  return 0;
}
