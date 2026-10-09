// [vector.capacity]/4,7,9 and [vector.modifiers]/2: reallocation preserves values,
// with no effects on an exception for CopyInsertable T. Copy/move selection is unspecified.
// Move-only types with potentially throwing moves are supported (effects unspecified if thrown).
// REQUIRES: exceptions
#include <vector>
#include <stdexcept>
#include "check.hpp"

struct NothrowMove {
  static inline int copies = 0, moves = 0;
  int v;
  NothrowMove(int x) : v(x) {}
  NothrowMove(const NothrowMove& o) : v(o.v) { ++copies; }
  NothrowMove(NothrowMove&& o) noexcept : v(o.v) { o.v = -99; ++moves; }
  NothrowMove& operator=(const NothrowMove&) = default;
  NothrowMove& operator=(NothrowMove&&) noexcept = default;
};
struct ThrowingMove {
  static inline bool fail = false;
  static inline int copies = 0, moves = 0;
  int v;
  ThrowingMove(int x) : v(x) {}
  ThrowingMove(const ThrowingMove& o) : v(o.v) {
    if (fail) throw std::runtime_error("copy");
    ++copies;
  }
  ThrowingMove(ThrowingMove&& o) noexcept(false) : v(o.v) {
    o.v = -99;
    if (fail) throw std::runtime_error("move");
    ++moves;
  }
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
    CHECK(v.size() == 4);
    for (int i = 0; i < 4; ++i) CHECK(v[i].v == i);
    NothrowMove::copies = NothrowMove::moves = 0;
    v.shrink_to_fit();
    CHECK(v.size() == 4 && v.capacity() >= v.size());
    for (int i = 0; i < 4; ++i) CHECK(v[i].v == i);
  }
  {
    std::vector<ThrowingMove> v;
    v.reserve(4);
    for (int i = 0; i < 4; ++i) v.emplace_back(i);
    ThrowingMove::copies = ThrowingMove::moves = 0;
    v.reserve(v.capacity() + 1);
    CHECK(v.size() == 4);
    for (int i = 0; i < 4; ++i) CHECK(v[i].v == i);
    auto cap = v.capacity();
    auto* data = v.data();
    ThrowingMove::fail = true;
    bool threw = false;
    try { v.reserve(cap + 1); }
    catch (const std::runtime_error&) { threw = true; }
    ThrowingMove::fail = false;
    CHECK(threw && v.size() == 4 && v.capacity() == cap && v.data() == data);
    for (int i = 0; i < 4; ++i) CHECK(v[i].v == i);
    while (v.size() < v.capacity()) v.emplace_back(0);
    ThrowingMove::copies = ThrowingMove::moves = 0;
    v.emplace_back(1);  // reallocating single-element insertion at the end
    CHECK(v.back().v == 1);
    for (int i = 0; i < 4; ++i) CHECK(v[i].v == i);
    for (std::size_t i = 4; i + 1 < v.size(); ++i) CHECK(v[i].v == 0);
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
