// [container.adaptors.format]/1-3: queue, priority_queue and stack are formattable when their
// container is: parse and format are those of formatter<ranges::ref_view<Container>> applied
// to the underlying container (so the elements appear in the container's order, with the
// range format specification). [vector.bool.fmt]/1-2: vector<bool>::reference is formatted
// by formatter<bool> (so "{}" gives true / false and the bool specifications apply), and so
// vector<bool> formats as a range of those. [format.formattable].
// COUNTERPART: libcxx:containers/container.adaptors/container.adaptors.format/format.pass.cpp
// COUNTERPART: libcxx:containers/sequences/vector.bool/vector.bool.fmt/format.pass.cpp
#include <deque>
#include <format>
#include <functional>
#include <list>
#include <queue>
#include <stack>
#include <string>
#include <vector>
#include "check.hpp"

struct NoFormatter {};

static_assert(std::formattable<std::queue<int>, char>);
static_assert(std::formattable<std::stack<int, std::vector<int>>, char>);
static_assert(std::formattable<std::priority_queue<int>, wchar_t>);
static_assert(!std::formattable<std::stack<NoFormatter>, char>);
static_assert(!std::formattable<std::queue<NoFormatter, std::list<NoFormatter>>, char>);
static_assert(std::formattable<std::vector<bool>::reference, char>);
static_assert(std::formattable<std::vector<bool>, char>);

int main() {
  std::stack<int> s;
  for (int i : {1, 2, 3}) s.push(i);
  CHECK(std::format("{}", s) == "[1, 2, 3]");  // bottom to top: the deque's order
  CHECK(std::format("{:n}", s) == "1, 2, 3");
  CHECK(std::format("{::02}", s) == "[01, 02, 03]");
  const std::stack<int> cs = s;
  CHECK(std::format("{}", cs) == "[1, 2, 3]");
  std::queue<std::string, std::list<std::string>> q;
  q.push("a");
  q.push("b");
  CHECK(std::format("{}", q) == "[\"a\", \"b\"]");
  CHECK(std::format("{:>12}", q) == "  [\"a\", \"b\"]");
  std::priority_queue<int> pq;
  for (int i : {3, 2, 1}) pq.push(i);  // stays a max-heap without moving: [3, 2, 1]
  CHECK(std::format("{}", pq) == "[3, 2, 1]");
  std::priority_queue<char, std::vector<char>, std::greater<char>> pc;
  pc.push('a');
  CHECK(std::format("{}", pc) == "['a']");
  CHECK(std::format(L"{}", std::stack<int>(std::deque<int>{4, 5})) == L"[4, 5]");

  std::vector<bool> vb{true, false, true};
  CHECK(std::format("{}", vb[0]) == "true");
  CHECK(std::format("{:d}", vb[1]) == "0");
  CHECK(std::format("{:^7}", vb[1]) == " false ");
  CHECK(std::format("{:#x}", vb[2]) == "0x1");
  CHECK(std::format("{}", vb) == "[true, false, true]");
  CHECK(std::format("{::d}", vb) == "[1, 0, 1]");
  CHECK(std::format(L"{}", vb[0]) == L"true");
  return 0;
}
