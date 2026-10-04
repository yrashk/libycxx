// [inplace.vector.overview]/1: "Its capacity is fixed and its elements are stored within the
// inplace_vector object itself" -- so the elements are T objects created on insertion only:
// the default constructor (noexcept, [inplace.vector.overview] synopsis; [container.reqmts]
// "X u;" Postconditions: u.empty()) constructs no T, so T need not be default constructible
// (Cpp17DefaultInsertable is required only by X(n) and resize(n),
// [inplace.vector.cons]/1, [sequence.reqmts]), and no T exists beyond size() elements.
// [inplace.vector.modifiers]/21: erasing calls the destructor once per erased element;
// [container.reqmts]: the destructor destroys every element. Elements are suitably aligned
// for an over-aligned T ([basic.align]; data() points to the first element,
// [inplace.vector.data]). [inplace.vector.modifiers]/22-23: swap needs only
// Cpp17MoveConstructible elements and exchanges contents of different sizes. N == 0 works
// for any T ([inplace.vector.overview]/5).
#include <inplace_vector>
#include <cstdint>
#include <type_traits>
#include <utility>
#include "move_only_elem.hpp"
#include "check.hpp"

struct Counted {
  static inline int live = 0;
  static inline int constructed = 0;
  int v;
  explicit Counted(int x) : v(x) {
    ++live;
    ++constructed;
  }
  Counted(const Counted& o) : v(o.v) {
    ++live;
    ++constructed;
  }
  Counted& operator=(const Counted&) = default;
  ~Counted() { --live; }
};
static_assert(!std::is_default_constructible_v<Counted>);

struct alignas(64) Over {
  int v = 0;
};

struct NoDefaultNoCopy {
  explicit NoDefaultNoCopy(int) {}
  NoDefaultNoCopy(const NoDefaultNoCopy&) = delete;
  NoDefaultNoCopy& operator=(const NoDefaultNoCopy&) = delete;
};

int main() {
  {
    std::inplace_vector<Counted, 16> v;
    static_assert(noexcept(std::inplace_vector<Counted, 16>()));
    CHECK(Counted::constructed == 0 && Counted::live == 0 && v.empty());
    v.emplace_back(1);
    v.emplace_back(2);
    v.push_back(Counted(3));
    CHECK(Counted::live == 3 && v.size() == 3);
    v.pop_back();
    CHECK(Counted::live == 2);
    v.erase(v.begin());
    CHECK(Counted::live == 1 && v.front().v == 2);
    v.resize(5, Counted(9));
    CHECK(Counted::live == 5);
    v.erase(v.begin() + 2, v.end());  // (resize(2) would need Cpp17DefaultInsertable)
    CHECK(Counted::live == 2);
    {
      std::inplace_vector<Counted, 16> w(v);
      CHECK(Counted::live == 4);
      w.clear();
      CHECK(Counted::live == 2 && w.empty());
    }
    std::inplace_vector<Counted, 16> x;
    x = v;
    CHECK(Counted::live == 4);
  }
  CHECK(Counted::live == 0);

  {
    std::inplace_vector<Over, 4> o;
    CHECK(alignof(decltype(o)) >= 64);
    o.push_back(Over{1});
    o.push_back(Over{2});
    CHECK(reinterpret_cast<std::uintptr_t>(o.data()) % 64 == 0);
    CHECK(reinterpret_cast<std::uintptr_t>(&o[1]) % 64 == 0);
  }

  {
    std::inplace_vector<MOElem, 8> a, b;
    for (int i = 0; i < 5; ++i) a.emplace_back(i);
    b.emplace_back(10);
    a.swap(b);
    CHECK(values_are(a, {10}) && values_are(b, {0, 1, 2, 3, 4}));
    std::swap(a, b);
    CHECK(values_are(a, {0, 1, 2, 3, 4}) && values_are(b, {10}));
    std::inplace_vector<MOElem, 8> c(std::move(a));
    CHECK(values_are(c, {0, 1, 2, 3, 4}));
    b = std::move(c);
    CHECK(values_are(b, {0, 1, 2, 3, 4}));
  }

  {
    std::inplace_vector<NoDefaultNoCopy, 0> z;
    CHECK(z.empty() && z.size() == 0 && z.begin() == z.end());
    std::inplace_vector<NoDefaultNoCopy, 3> n;
    n.emplace_back(1);
    n.emplace_back(2);
    CHECK(n.size() == 2);
    n.pop_back();
    CHECK(n.size() == 1);
  }
  return 0;
}
