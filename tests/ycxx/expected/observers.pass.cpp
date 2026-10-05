// [expected.object.obs]: operator->, operator* (value categories, noexcept), operator bool
// (explicit), has_value, has_error (C++26), value() (throws bad_expected_access<E> carrying a
// copy of / the moved error), error() (value categories, noexcept), value_or, error_or
// (default template arguments allow braced initializers).
// REQUIRES: exceptions
#include <expected>
#include <type_traits>
#include <utility>
#include "check.hpp"

using E = std::expected<int, long>;
static_assert(std::is_same_v<decltype(*std::declval<E&>()), int&>);
static_assert(std::is_same_v<decltype(*std::declval<E&&>()), int&&>);
static_assert(std::is_same_v<decltype(*std::declval<const E&>()), const int&>);
static_assert(std::is_same_v<decltype(*std::declval<const E&&>()), const int&&>);
static_assert(std::is_same_v<decltype(std::declval<E&>().value()), int&>);
static_assert(std::is_same_v<decltype(std::declval<E&&>().value()), int&&>);
static_assert(std::is_same_v<decltype(std::declval<const E&>().value()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<const E&&>().value()), const int&&>);
static_assert(std::is_same_v<decltype(std::declval<E&>().error()), long&>);
static_assert(std::is_same_v<decltype(std::declval<E&&>().error()), long&&>);
static_assert(std::is_same_v<decltype(std::declval<const E&>().error()), const long&>);
static_assert(std::is_same_v<decltype(std::declval<const E&&>().error()), const long&&>);
static_assert(std::is_same_v<decltype(std::declval<E&>().operator->()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const E&>().operator->()), const int*>);
static_assert(noexcept(*std::declval<E&>()) && noexcept(std::declval<E&>().error()));
static_assert(noexcept(std::declval<E&>().has_value()) && noexcept(std::declval<const E&>().has_error()));
static_assert(noexcept(static_cast<bool>(std::declval<const E&>())));
static_assert(!std::is_convertible_v<E, bool>);
static_assert(std::is_same_v<decltype(std::declval<E&>().value_or(1)), int>);
static_assert(std::is_same_v<decltype(std::declval<E&>().error_or(1)), long>);

struct Pt { int x, y; };
struct MoveTrack {
  int v;
  bool moved_from = false;
  constexpr MoveTrack(int x) : v(x) {}
  constexpr MoveTrack(const MoveTrack& o) : v(o.v) {}
  constexpr MoveTrack(MoveTrack&& o) : v(o.v) { o.moved_from = true; }
};

constexpr bool test() {
  E v(3), e(std::unexpect, 4L);
  if (!v.has_value() || v.has_error() || !v || *v != 3 || v.value() != 3) return false;
  if (e.has_value() || !e.has_error() || e || e.error() != 4) return false;
  *v = 5;
  if (v.value() != 5) return false;
  e.error() = 6;
  if (e.error() != 6) return false;
  if (v.value_or(9) != 5 || e.value_or(9) != 9) return false;
  if (v.error_or(9) != 9 || e.error_or(9) != 6) return false;
  std::expected<Pt, Pt> p(std::unexpect, 1, 2);
  if (p.value_or({7, 8}).y != 8) return false;            // U defaults to T
  std::expected<Pt, Pt> q(std::in_place, 1, 2);
  if (q.error_or({7, 8}).x != 7 || q->x != 1) return false;  // G defaults to E
  std::expected<MoveTrack, MoveTrack> m(std::in_place, 1);
  MoveTrack t = std::move(m).value_or(0);
  if (t.v != 1 || !m->moved_from) return false;
  std::expected<MoveTrack, MoveTrack> me(std::unexpect, 2);
  MoveTrack t2 = std::move(me).error_or(0);
  if (t2.v != 2 || !me.error().moved_from) return false;
  return true;
}
static_assert(test());

struct CopyCount {
  static inline int copies = 0, moves = 0;
  int v;
  CopyCount(int x) : v(x) {}
  CopyCount(const CopyCount& o) : v(o.v) { ++copies; }
  CopyCount(CopyCount&& o) : v(o.v) { ++moves; }
};

int main() {
  CHECK(test());
  std::expected<int, CopyCount> e(std::unexpect, 7);
  bool caught = false;
  try {
    (void)e.value();
  } catch (const std::bad_expected_access<CopyCount>& ex) {
    caught = ex.error().v == 7;
  }
  CHECK(caught);
  CHECK(e.error().v == 7);
  caught = false;
  CopyCount::moves = 0;
  try {
    (void)std::move(e).value();
  } catch (std::bad_expected_access<CopyCount>& ex) {
    caught = ex.error().v == 7;
  }
  CHECK(caught);
  CHECK(CopyCount::moves >= 1);  // bad_expected_access(std::move(error())) moves the error
  caught = false;
  const std::expected<int, CopyCount>& ce = e;
  try { (void)ce.value(); } catch (const std::bad_expected_access<void>&) { caught = true; }
  CHECK(caught);
  return 0;
}
