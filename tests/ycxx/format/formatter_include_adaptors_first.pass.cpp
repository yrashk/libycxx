// [stack.syn], [queue.syn]: <stack> and <queue> declare the adaptors' formatter specializations
// (constrained on formattable<Container, charT>); with <format> included afterwards, before any
// use, they format the adaptor's container ([container.adaptors.format]/2-3), here with an
// underlying range-format-spec.
#include <stack>
#include <queue>
#include <format>
#include <vector>
#include "check.hpp"

int main() {
  std::stack<int, std::vector<int>> s;
  for (int i : {3, 1, 2})
    s.push(i);
  CHECK(std::format("{::02}", s) == "[03, 01, 02]");
  std::queue<int> q;
  q.push(7);
  q.push(8);
  CHECK(std::format("{:n}", q) == "7, 8");
  std::priority_queue<int> pq;
  pq.push(4);
  CHECK(std::format("{}", pq) == "[4]");
  static_assert(!std::enable_nonlocking_formatter_optimization<std::stack<int>>);
}
