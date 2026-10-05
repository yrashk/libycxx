// [mdspan.mdspan.members]/7-12: at(indices...), at(span) and at(array) return (*this)[I...]
// and throw out_of_range if I is not a multidimensional index in extents(); negative indices
// are not in the index space either.
// REQUIRES: exceptions
#include <mdspan>
#include <array>
#include <span>
#include <stdexcept>
#include "check.hpp"

int main() {
  int data[6] = {0, 1, 2, 3, 4, 5};
  std::mdspan<int, std::extents<int, 2, 3>> m(data);
  CHECK(m.at(1, 2) == 5 && &m.at(0, 1) == &data[1]);
  CHECK(m.at(std::array<int, 2>{1, 0}) == 3);
  long idx[2] = {0, 2};
  CHECK(m.at(std::span<long, 2>(idx)) == 2);
  m.at(1, 1) = 40;
  CHECK(data[4] == 40);
  auto throws = [&](auto... i) {
    try {
      (void)m.at(i...);
    } catch (const std::out_of_range&) {
      return true;
    }
    return false;
  };
  CHECK(throws(2, 0));
  CHECK(throws(0, 3));
  CHECK(throws(-1, 0));
  CHECK(throws(0, -1));
  CHECK(throws(5u, 0u));
  CHECK(!throws(1, 2));
  bool caught = false;
  try {
    (void)m.at(std::array<int, 2>{0, 7});
  } catch (const std::out_of_range&) {
    caught = true;
  }
  CHECK(caught);
  return 0;
}
