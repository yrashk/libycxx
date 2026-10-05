// [expected.object.assign]: copy/move assignment in all four state combinations, operator=(U&&)
// and operator=(unexpected<G>) using reinit-expected:
//   nothrow-constructible from args -> destroy old, construct new directly;
//   else nothrow-move-constructible -> construct temporary, destroy old, move in;
//   else -> move old into a backup, construct new, restore backup on exception.
// REQUIRES: exceptions
#include <expected>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Log {
  static inline int ctor = 0, move = 0, copy_assign = 0, move_assign = 0, dtor = 0;
  static void reset() { ctor = move = copy_assign = move_assign = dtor = 0; }
};
// nothrow conversion from int
struct NT {
  int v;
  NT(int x) noexcept : v(x) { ++Log::ctor; }
  NT(const NT& o) noexcept : v(o.v) { ++Log::ctor; }
  NT(NT&& o) noexcept : v(o.v) { ++Log::move; }
  NT& operator=(const NT& o) { v = o.v; ++Log::copy_assign; return *this; }
  NT& operator=(NT&& o) noexcept { v = o.v; ++Log::move_assign; return *this; }
  ~NT() { ++Log::dtor; }
};
// potentially-throwing conversion/copy, nothrow move
struct TM {
  int v;
  TM(int x) noexcept(false) : v(x) { if (x < 0) throw x; ++Log::ctor; }
  TM(const TM& o) noexcept(false) : v(o.v) { ++Log::ctor; }
  TM(TM&& o) noexcept : v(o.v) { ++Log::move; }
  TM& operator=(const TM& o) { v = o.v; ++Log::copy_assign; return *this; }
  TM& operator=(TM&& o) noexcept { v = o.v; ++Log::move_assign; return *this; }
};
// everything may throw
struct TT {
  int v;
  TT(int x) noexcept(false) : v(x) { if (x < 0) throw x; }
  TT(const TT& o) noexcept(false) : v(o.v) {}
  TT(TT&& o) noexcept(false) : v(o.v) {}
  TT& operator=(const TT&) = default;
  TT& operator=(TT&&) = default;
};

constexpr bool test_constexpr() {
  std::expected<int, long> a(1), b(std::unexpect, 2L), c(3);
  a = c;  // value/value
  if (*a != 3) return false;
  a = b;  // value <- error
  if (a || a.error() != 2) return false;
  a = b;  // error <- error
  if (a.error() != 2) return false;
  a = c;  // error <- value
  if (!a || *a != 3) return false;
  a = std::expected<int, long>(std::unexpect, 4L);
  if (a.error() != 4) return false;
  a = 5;  // U&& on error
  if (!a || *a != 5) return false;
  a = 6;  // U&& on value
  if (*a != 6) return false;
  a = std::unexpected(7L);
  if (a || a.error() != 7) return false;
  a = std::unexpected(8);  // unexpected<int> -> long
  if (a.error() != 8) return false;
  std::expected<int, long> d;
  d = {};  // U defaults to remove_cv_t<T>: assigns int{} (not ambiguous)
  if (!d || *d != 0) return false;
  return true;
}
static_assert(test_constexpr());

int main() {
  CHECK(test_constexpr());
  // value <- value: plain assignment
  {
    std::expected<NT, int> a(1), b(2);
    Log::reset();
    a = b;
    CHECK(a->v == 2 && Log::copy_assign == 1 && Log::ctor == 0);
    Log::reset();
    a = std::move(b);
    CHECK(Log::move_assign == 1 && Log::move == 0);
  }
  // error <- value, nothrow-constructible: direct construction (no temporary/move)
  {
    std::expected<NT, int> a(std::unexpect, 1);
    Log::reset();
    a = 5;
    CHECK(a.has_value() && a->v == 5 && Log::ctor == 1 && Log::move == 0);
  }
  // error <- value, construction may throw but nothrow move: temporary + one move
  {
    std::expected<TM, int> a(std::unexpect, 1);
    Log::reset();
    a = 5;
    CHECK(a.has_value() && a->v == 5 && Log::ctor == 1 && Log::move == 1);
    std::expected<TM, int> b(std::unexpect, 2);
    Log::reset();
    b = a;  // copy assignment, rhs has value: reinit-expected(val, unex, *rhs)
    CHECK(b.has_value() && b->v == 5 && Log::ctor == 1 && Log::move == 1);
  }
  // nothing nothrow for T, E nothrow-move: backup of the error; on exception restored
  {
    std::expected<TT, int> a(std::unexpect, 9);
    bool threw = false;
    try { a = -1; } catch (int) { threw = true; }
    CHECK(threw);
    CHECK(!a.has_value() && a.error() == 9);  // strong guarantee via backup
    a = 3;
    CHECK(a.has_value() && a->v == 3);
  }
  // value <- unexpected, T may throw: E side rollback
  {
    std::expected<int, TT> a(4);
    bool threw = false;
    try { a = std::unexpected<int>(-1); } catch (int) { threw = true; }
    CHECK(threw);
    CHECK(a.has_value() && *a == 4);
    a = std::unexpected<int>(2);
    CHECK(!a.has_value() && a.error().v == 2);
  }
  return 0;
}
