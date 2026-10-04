// [util.smartptr.shared.const]/16-19: the aliasing constructors shared_ptr(const
// shared_ptr<Y>& r, element_type* p) and shared_ptr(shared_ptr<Y>&& r, element_type* p)
// store p and share ownership with r; after the rvalue form r is empty and r.get() ==
// nullptr. use_count counts owners, not stored pointers; an aliasing shared_ptr from an
// empty r stores p but is empty (use_count() == 0).
#include <memory>
#include <utility>
#include "check.hpp"

struct Pair {
  static inline int live = 0;
  int first = 1;
  int second = 2;
  Pair() { ++live; }
  ~Pair() { --live; }
};

int main() {
  {
    std::shared_ptr<int> second;
    {
      auto owner = std::make_shared<Pair>();
      std::shared_ptr<int> first(owner, &owner->first);
      CHECK(first.get() == &owner->first && *first == 1 && owner.use_count() == 2);
      second = std::shared_ptr<int>(std::move(owner), &owner->second);
      // owner->second was evaluated before the move; owner is now empty.
      CHECK(!owner && owner.use_count() == 0);
      CHECK(*second == 2 && second.use_count() == 2 && first.use_count() == 2);
    }
    CHECK(Pair::live == 1);  // second keeps the whole Pair alive
    CHECK(*second == 2);
  }
  CHECK(Pair::live == 0);
  {
    static int global = 9;
    std::shared_ptr<int> empty;
    std::shared_ptr<int> alias(empty, &global);
    CHECK(alias.get() == &global && *alias == 9 && alias.use_count() == 0 && alias);
    std::shared_ptr<int> owned = std::make_shared<int>(1);
    std::shared_ptr<int> nullalias(owned, nullptr);
    CHECK(!nullalias && nullalias.use_count() == 2);  // owns but stores null
  }
  return 0;
}
