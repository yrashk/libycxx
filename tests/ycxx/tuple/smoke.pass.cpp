// Harness smoke test.
#include <tuple>
#include "check.hpp"
int main() {
  std::tuple<int, char> t{1, 'a'};
  CHECK(std::get<0>(t) == 1);
  return 0;
}
