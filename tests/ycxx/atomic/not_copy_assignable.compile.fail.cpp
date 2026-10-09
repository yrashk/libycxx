// EXPECT-ERROR: error: static assertion failed[^\n]*atomic<T> needs a trivially copyable, copy and move constructible and assignable T
// [atomics.types.generic.general]/1: "The program is ill-formed if any of ...
// is_copy_assignable_v<T>, is_move_assignable_v<T> ... is false." The type below is trivially
// copyable (its copy constructor is trivial and its assignment is deleted).
#include <atomic>

struct NoAssign {
  int v;
  NoAssign& operator=(const NoAssign&) = delete;
};
static_assert(__is_trivially_copyable(NoAssign));
std::atomic<NoAssign> a;
