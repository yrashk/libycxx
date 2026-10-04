// [sequence.reqmts]/45-56: erase(q) returns an iterator to the element following q (or
// end()); erase(q1, q2) returns an iterator to the element q2 pointed to; clear() destroys
// all elements, empty() afterwards. [vector.modifiers]/4-6: erase invalidates only at or
// after the point of erase; the destructor of T is called once per erased element and the
// assignment operator once per element after the erased ones.
#include <vector>
#include "check.hpp"

struct Counted {
  static inline int dtors = 0;
  static inline int assigns = 0;
  int v;
  Counted(int x) : v(x) {}
  Counted(const Counted& o) : v(o.v) {}
  Counted(Counted&& o) noexcept : v(o.v) {}
  Counted& operator=(const Counted& o) {
    ++assigns;
    v = o.v;
    return *this;
  }
  Counted& operator=(Counted&& o) noexcept {
    ++assigns;
    v = o.v;
    return *this;
  }
  ~Counted() { ++dtors; }
};

static_assert(noexcept(std::vector<int>().clear()));

constexpr bool test() {
  std::vector<int> v{0, 1, 2, 3, 4, 5, 6};
  int* front = v.data();
  auto it = v.erase(v.cbegin() + 2);
  if (v != std::vector<int>{0, 1, 3, 4, 5, 6} || it != v.begin() + 2 || *it != 3) return false;
  if (v.data() != front) return false;  // elements before the erase point stay put
  it = v.erase(v.cbegin() + 1, v.cbegin() + 3);
  if (v != std::vector<int>{0, 4, 5, 6} || *it != 4) return false;
  it = v.erase(v.cend() - 1);
  if (it != v.end() || v.back() != 5) return false;
  it = v.erase(v.cbegin(), v.cbegin());
  if (it != v.begin() || v.size() != 3) return false;
  it = v.erase(v.cbegin(), v.cend());
  if (it != v.end() || !v.empty()) return false;
  v = {1, 2, 3};
  v.clear();
  if (!v.empty() || v.size() != 0) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  {
    std::vector<Counted> v;
    v.reserve(10);
    for (int i = 0; i < 8; ++i) v.emplace_back(i);
    Counted::dtors = Counted::assigns = 0;
    v.erase(v.begin() + 2, v.begin() + 4);  // erase 2, 6 - 2 - 2 = 4 elements after them
    CHECK(Counted::dtors == 2);
    CHECK(Counted::assigns == 4);
    CHECK(v.size() == 6 && v[2].v == 4 && v[5].v == 7);
    Counted::dtors = Counted::assigns = 0;
    v.erase(v.begin());
    CHECK(Counted::dtors == 1 && Counted::assigns == 5);
    Counted::dtors = Counted::assigns = 0;
    v.pop_back();
    CHECK(Counted::dtors == 1 && Counted::assigns == 0);
    Counted::dtors = 0;
    v.clear();
    CHECK(Counted::dtors == 4 && v.empty());
  }
  return 0;
}
