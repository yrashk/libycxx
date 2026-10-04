// [range.utility.conv.general]/4-5: container-appendable and container-append accept a
// container whose only way of adding an element is
//   c.emplace_hint(c.end(), std::forward<Ref>(ref))
// so ranges::to default-constructs it and emplaces each element with a hint
// ([range.utility.conv.to]/2.1.4).
#include <cstddef>
#include <ranges>
#include "check.hpp"

struct ByEmplaceHint {
  int data[8] = {};
  std::size_t n = 0;
  int hints = 0;
  constexpr int* begin() { return data; }
  constexpr int* end() { return data + n; }
  constexpr int* emplace_hint(int* pos, int v) {
    if (pos == end()) ++hints;
    data[n++] = v;
    return data + n - 1;
  }
};

constexpr bool test() {
  int a[3] = {4, 5, 6};
  auto c = std::ranges::to<ByEmplaceHint>(a);
  CHECK(c.n == 3 && c.data[2] == 6 && c.hints == 3);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
