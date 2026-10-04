// [refwrap.helpers]: ref(T&) -> reference_wrapper<T>; ref(reference_wrapper<T>) returns t;
// cref(const T&) -> reference_wrapper<const T>; cref(reference_wrapper<T>) ->
// reference_wrapper<const T>; all constexpr and noexcept. "template<class T> void
// ref(const T&&) = delete;" and likewise for cref. T may be incomplete.
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

template <class T>
concept can_ref = requires(T&& t) { std::ref(std::forward<T>(t)); };
template <class T>
concept can_cref = requires(T&& t) { std::cref(std::forward<T>(t)); };

static_assert(can_ref<int&>);
static_assert(!can_ref<int>);
static_assert(!can_ref<const int>);
static_assert(can_cref<const int&>);
static_assert(!can_cref<int>);
static_assert(!can_cref<const int&&>);
static_assert(noexcept(std::ref(std::declval<int&>())));
static_assert(noexcept(std::cref(std::declval<int&>())));
static_assert(std::is_same_v<decltype(std::ref(std::declval<int&>())), std::reference_wrapper<int>>);
static_assert(std::is_same_v<decltype(std::ref(std::declval<const int&>())), std::reference_wrapper<const int>>);
static_assert(std::is_same_v<decltype(std::cref(std::declval<int&>())), std::reference_wrapper<const int>>);
static_assert(std::is_same_v<decltype(std::ref(std::declval<std::reference_wrapper<int>>())), std::reference_wrapper<int>>);
static_assert(std::is_same_v<decltype(std::cref(std::declval<std::reference_wrapper<int>>())),
                             std::reference_wrapper<const int>>);

struct Incomplete;
Incomplete& inc();
inline auto use_incomplete() { return std::ref(inc()); }
inline auto use_incomplete_c() { return std::cref(inc()); }

constexpr bool test() {
  int x = 1;
  auto r = std::ref(x);
  auto rr = std::ref(r);  // not reference_wrapper<reference_wrapper<int>>
  if (&rr.get() != &x) return false;
  auto cr = std::cref(r);
  if (&cr.get() != &x) return false;
  auto c = std::cref(x);
  if (&c.get() != &x) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
