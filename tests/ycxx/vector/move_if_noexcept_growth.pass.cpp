// [vector.modifiers]/2 and [vector.capacity]/4: on reallocation the existing elements must
// be relocated so that the strong guarantee holds when T is Cpp17CopyInsertable; a type with
// a non-throwing move constructor is moved (no copies), a copyable type whose move
// constructor may throw is copied. A move-only type with a potentially-throwing move is
// still supported (effects unspecified if it throws).
#include <vector>
#include "check.hpp"

struct NothrowMove {
  static inline int copies = 0, moves = 0;
  int v;
  NothrowMove(int x) : v(x) {}
  NothrowMove(const NothrowMove& o) : v(o.v) { ++copies; }
  NothrowMove(NothrowMove&& o) noexcept : v(o.v) { ++moves; }
  NothrowMove& operator=(const NothrowMove&) = default;
  NothrowMove& operator=(NothrowMove&&) noexcept = default;
};
struct ThrowingMove {
  static inline int copies = 0, moves = 0;
  int v;
  ThrowingMove(int x) : v(x) {}
  ThrowingMove(const ThrowingMove& o) : v(o.v) { ++copies; }
  ThrowingMove(ThrowingMove&& o) noexcept(false) : v(o.v) { ++moves; }
  ThrowingMove& operator=(const ThrowingMove&) = default;
  ThrowingMove& operator=(ThrowingMove&&) = default;
};
struct MoveOnlyThrowing {
  int v;
  MoveOnlyThrowing(int x) : v(x) {}
  MoveOnlyThrowing(MoveOnlyThrowing&& o) noexcept(false) : v(o.v) {}
  MoveOnlyThrowing& operator=(MoveOnlyThrowing&&) = default;
};

int main() {
  {
    std::vector<NothrowMove> v;
    v.reserve(4);
    for (int i = 0; i < 4; ++i) v.emplace_back(i);
    NothrowMove::copies = NothrowMove::moves = 0;
    v.reserve(v.capacity() + 1);
    CHECK(NothrowMove::copies == 0 && NothrowMove::moves == 4);
    NothrowMove::copies = NothrowMove::moves = 0;
    v.shrink_to_fit();
    CHECK(NothrowMove::copies == 0);
  }
  {
    std::vector<ThrowingMove> v;
    v.reserve(4);
    for (int i = 0; i < 4; ++i) v.emplace_back(i);
    ThrowingMove::copies = ThrowingMove::moves = 0;
    v.reserve(v.capacity() + 1);
    CHECK(ThrowingMove::copies == 4 && ThrowingMove::moves == 0);
    while (v.size() < v.capacity()) v.emplace_back(0);
    ThrowingMove::copies = ThrowingMove::moves = 0;
    v.emplace_back(1);  // reallocating single-element insertion at the end
    CHECK(ThrowingMove::copies == static_cast<int>(v.size()) - 1);
    CHECK(ThrowingMove::moves == 0);
  }
  {
    std::vector<MoveOnlyThrowing> v;
    for (int i = 0; i < 50; ++i) v.emplace_back(i);
    v.insert(v.begin(), MoveOnlyThrowing(-1));
    v.erase(v.begin() + 3);
    v.reserve(200);
    CHECK(v.size() == 50 && v[0].v == -1 && v[3].v == 3 && v[49].v == 49);
  }
  return 0;
}
