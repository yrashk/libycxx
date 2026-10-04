// [variant.assign]/1-10: copy and move assignment -- same-index assigns, different-index
// re-constructs; deleted/constraint rules; triviality; move noexcept specification.
// Copy (2.4/2.5): if Tj is nothrow copy-constructible or not nothrow move-constructible,
// emplace<j>(GET<j>(rhs)); otherwise operator=(variant(rhs)) (copy then move).
#include <variant>
#include <type_traits>
#include "check.hpp"

struct Tracker {
  static inline int copies = 0, moves = 0, copy_assigns = 0, move_assigns = 0;
  int v = 0;
  Tracker(int x) : v(x) {}
  Tracker(const Tracker& o) noexcept(false) : v(o.v) { ++copies; }  // potentially-throwing copy
  Tracker(Tracker&& o) noexcept : v(o.v) { ++moves; }
  Tracker& operator=(const Tracker& o) { v = o.v; ++copy_assigns; return *this; }
  Tracker& operator=(Tracker&& o) noexcept { v = o.v; ++move_assigns; return *this; }
  static void reset() { copies = moves = copy_assigns = move_assigns = 0; }
};
struct NothrowCopy {
  static inline int copies = 0, moves = 0;
  NothrowCopy() = default;
  NothrowCopy(const NothrowCopy&) noexcept { ++copies; }
  NothrowCopy(NothrowCopy&&) noexcept { ++moves; }
  NothrowCopy& operator=(const NothrowCopy&) = default;
  NothrowCopy& operator=(NothrowCopy&&) = default;
};
struct NoCopyAssign {
  NoCopyAssign() = default;
  NoCopyAssign(const NoCopyAssign&) = default;
  NoCopyAssign& operator=(const NoCopyAssign&) = delete;
};
struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;
};
struct ThrowingMoveAssign {
  ThrowingMoveAssign() = default;
  ThrowingMoveAssign(ThrowingMoveAssign&&) noexcept = default;
  ThrowingMoveAssign& operator=(ThrowingMoveAssign&&) noexcept(false) { return *this; }
};
struct NonTrivialAssign {
  NonTrivialAssign& operator=(const NonTrivialAssign&) { return *this; }
};

static_assert(std::is_copy_assignable_v<std::variant<int, double>>);
static_assert(!std::is_copy_assignable_v<std::variant<int, NoCopyAssign>>);
static_assert(!std::is_copy_assignable_v<std::variant<int, MoveOnly>>);
static_assert(std::is_move_assignable_v<std::variant<int, MoveOnly>>);
static_assert(std::is_trivially_copy_assignable_v<std::variant<int, double>>);
static_assert(!std::is_trivially_copy_assignable_v<std::variant<int, NonTrivialAssign>>);
static_assert(std::is_trivially_move_assignable_v<std::variant<int, double>>);
static_assert(std::is_nothrow_move_assignable_v<std::variant<int, MoveOnly>>);
static_assert(!std::is_nothrow_move_assignable_v<std::variant<int, ThrowingMoveAssign>>);

constexpr bool test_constexpr() {
  std::variant<int, double> a(1), b(2.5);
  a = b;
  if (a.index() != 1 || std::get<1>(a) != 2.5) return false;
  b = std::variant<int, double>(4);
  if (b.index() != 0 || std::get<0>(b) != 4) return false;
  a = std::move(b);
  if (a.index() != 0 || std::get<0>(a) != 4) return false;
  return true;
}
static_assert(test_constexpr());

int main() {
  CHECK(test_constexpr());
  // same index: copy-assigns
  {
    std::variant<int, Tracker> a(std::in_place_index<1>, 1), b(std::in_place_index<1>, 2);
    Tracker::reset();
    a = b;
    CHECK(std::get<1>(a).v == 2);
    CHECK(Tracker::copy_assigns == 1 && Tracker::copies == 0);
    Tracker::reset();
    a = std::move(b);
    CHECK(Tracker::move_assigns == 1 && Tracker::moves == 0);
  }
  // different index, Tj has potentially-throwing copy but nothrow move: (2.5) copy + move
  {
    std::variant<int, Tracker> a(0), b(std::in_place_index<1>, 7);
    Tracker::reset();
    a = b;
    CHECK(a.index() == 1 && std::get<1>(a).v == 7);
    CHECK(Tracker::copies == 1 && Tracker::moves == 1);
    CHECK(Tracker::copy_assigns == 0 && Tracker::move_assigns == 0);
  }
  // different index, Tj nothrow copy: (2.4) emplace from copy, no move
  {
    std::variant<int, NothrowCopy> a(0), b(std::in_place_index<1>);
    NothrowCopy::copies = NothrowCopy::moves = 0;
    a = b;
    CHECK(a.index() == 1 && NothrowCopy::copies == 1 && NothrowCopy::moves == 0);
  }
  // different index, move: (8.4) emplace from move
  {
    std::variant<int, Tracker> a(0), b(std::in_place_index<1>, 3);
    Tracker::reset();
    a = std::move(b);
    CHECK(a.index() == 1 && std::get<1>(a).v == 3 && Tracker::moves == 1 && Tracker::move_assigns == 0);
  }
  return 0;
}
