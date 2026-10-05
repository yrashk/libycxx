// [vector.bool.fmt], [container.adaptors.format]: <format> first, then <vector>, <stack> and
// <queue>; the specializations those headers declare are found by the formatting functions.
// queue, priority_queue and stack format their underlying container as a range.
#include <format>
#include <vector>
#include <stack>
#include <queue>
#include <string>
#include "check.hpp"

static_assert(std::formattable<std::vector<bool>::reference, char>);
static_assert(std::formattable<std::stack<int>, wchar_t>);
static_assert(std::formattable<std::queue<int>, char>);
static_assert(std::formattable<std::priority_queue<int>, char>);

int main() {
  std::vector<bool> v{false, true};
  CHECK(std::format("{}", v[1]) == "true");
  CHECK(std::format(L"{:^7}", v[0]) == L" false ");
  std::stack<int> s;
  s.push(1);
  s.push(2);
  CHECK(std::format("{}", s) == "[1, 2]");
  CHECK(std::format(L"{:n}", s) == L"1, 2");
  std::queue<char> q;
  q.push('x');
  CHECK(std::format("{}", q) == "['x']");
  std::priority_queue<int> pq;
  pq.push(5);
  CHECK(std::format("{}", pq) == "[5]");
}
