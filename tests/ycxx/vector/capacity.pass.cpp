// [vector.capacity]: capacity() is the number of elements that can be held without
// reallocation; reserve(n): capacity() >= n afterwards if it reallocates, unchanged
// otherwise; reallocation happens iff capacity() < n; no reallocation during insertions
// until size would exceed capacity(); throws length_error if n > max_size().
// shrink_to_fit does not increase capacity(). resize(sz) erases or appends
// default-inserted elements; resize(sz, c) appends copies of c. [container.reqmts]/52-62:
// size/max_size/empty. [vector.capacity]/7: without reallocation pointers stay valid.
// REQUIRES: exceptions
#include <vector>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

static_assert(noexcept(std::vector<int>().size()) && noexcept(std::vector<int>().capacity()));
static_assert(noexcept(std::vector<int>().empty()) && noexcept(std::vector<int>().max_size()));

struct DefaultSeven {
  int v = 7;
};

constexpr bool test() {
  std::vector<int> v;
  if (v.capacity() < v.size() || v.max_size() < 1) return false;
  v.reserve(50);
  if (v.capacity() < 50 || v.size() != 0) return false;
  auto cap = v.capacity();
  v.push_back(1);
  int* p = v.data();
  int& first = v[0];
  for (int i = 1; i < static_cast<int>(cap); ++i) v.push_back(i);
  if (v.capacity() != cap || v.data() != p || &first != p) return false;  // no reallocation
  v.reserve(10);
  if (v.capacity() != cap) return false;  // never shrinks
  v.resize(3);
  if (v.size() != 3 || v[2] != 2 || v.capacity() != cap) return false;
  v.shrink_to_fit();
  if (v.capacity() > cap || v.capacity() < 3) return false;
  v.resize(6);
  if (v.size() != 6 || v[3] != 0 || v[5] != 0) return false;
  v.resize(8, 9);
  if (v.size() != 8 || v[6] != 9 || v[7] != 9 || v[5] != 0) return false;
  v.resize(2, 9);
  if (v.size() != 2 || v[1] != 1) return false;
  v.resize(0);
  if (!v.empty()) return false;

  std::vector<DefaultSeven> d(2);
  d.resize(4);
  if (d[3].v != 7) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::vector<int> v{1, 2, 3};
  if (v.max_size() < static_cast<std::vector<int>::size_type>(-1)) {
    bool threw = false;
    try {
      v.reserve(v.max_size() + 1);
    } catch (const std::length_error&) {
      threw = true;
    }
    CHECK(threw);
  }
  CHECK(v.size() == 3 && v[2] == 3);
  return 0;
}
