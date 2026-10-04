// [optional.observe]: operator->, operator* (all four value categories; noexcept),
// operator bool (explicit), has_value, value() (throws bad_optional_access),
// value_or (default template argument U = remove_cv_t<T>, so value_or({...}) works;
// && overload moves). [optional.bad.access].
#include <optional>
#include <exception>
#include <type_traits>
#include <utility>
#include "check.hpp"

using O = std::optional<int>;
static_assert(std::is_same_v<decltype(*std::declval<O&>()), int&>);
static_assert(std::is_same_v<decltype(*std::declval<O&&>()), int&&>);
static_assert(std::is_same_v<decltype(*std::declval<const O&>()), const int&>);
static_assert(std::is_same_v<decltype(*std::declval<const O&&>()), const int&&>);
static_assert(std::is_same_v<decltype(std::declval<O&>().value()), int&>);
static_assert(std::is_same_v<decltype(std::declval<O&&>().value()), int&&>);
static_assert(std::is_same_v<decltype(std::declval<const O&>().value()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<const O&&>().value()), const int&&>);
static_assert(std::is_same_v<decltype(std::declval<O&>().operator->()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const O&>().operator->()), const int*>);
static_assert(noexcept(*std::declval<O&>()));
static_assert(noexcept(std::declval<O&>().operator->()));
static_assert(noexcept(std::declval<O&>().has_value()));
static_assert(noexcept(static_cast<bool>(std::declval<O&>())));
static_assert(!std::is_convertible_v<O, bool>);
static_assert(std::is_constructible_v<bool, O>);
static_assert(std::is_same_v<decltype(std::declval<O&>().value_or(1L)), int>);

static_assert(std::is_base_of_v<std::exception, std::bad_optional_access>);
static_assert(std::is_nothrow_default_constructible_v<std::bad_optional_access>);
static_assert(std::is_nothrow_copy_constructible_v<std::bad_optional_access>);

struct Agg { int a, b; };
struct MoveTrack {
  int v;
  bool moved_from = false;
  constexpr MoveTrack(int x) : v(x) {}
  constexpr MoveTrack(const MoveTrack& o) : v(o.v) {}
  constexpr MoveTrack(MoveTrack&& o) : v(o.v) { o.moved_from = true; }
};

constexpr bool test() {
  O o(4);
  if (!o.has_value() || !o || *o != 4 || o.value() != 4) return false;
  *o = 5;
  if (o.value() != 5) return false;
  if (o.value_or(9) != 5) return false;
  O e;
  if (e.value_or(9) != 9) return false;
  std::optional<Agg> ag;
  if (ag.value_or({1, 2}).b != 2) return false;  // U defaults to Agg
  if (e.value_or({}) != 0) return false;
  std::optional<MoveTrack> m(std::in_place, 3);
  MoveTrack t = std::move(m).value_or(0);
  if (t.v != 3 || !m->moved_from) return false;
  std::optional<MoveTrack> m2(std::in_place, 3);
  MoveTrack t2 = m2.value_or(0);
  if (t2.v != 3 || m2->moved_from) return false;
  const std::optional<Agg> ca(Agg{1, 2});
  if (ca->a != 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  O e;
  bool threw = false;
  try { (void)e.value(); } catch (const std::bad_optional_access& x) { threw = x.what() != nullptr; }
  CHECK(threw);
  threw = false;
  try { (void)std::move(e).value(); } catch (const std::exception&) { threw = true; }
  CHECK(threw);
  threw = false;
  const O ce;
  try { (void)ce.value(); } catch (const std::bad_optional_access&) { threw = true; }
  CHECK(threw);
  return 0;
}
