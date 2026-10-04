// [unique.ptr]: every member and the non-member operations of unique_ptr and
// default_delete are constexpr (since C++23), so ownership can be created, transferred,
// reset, compared, swapped and destroyed during constant evaluation.
#include <memory>
#include <utility>
#include "check.hpp"

struct Node {
  int value;
  std::unique_ptr<Node> next;
};

constexpr int sum_list(int n) {
  std::unique_ptr<Node> head;
  for (int i = 1; i <= n; ++i) head = std::make_unique<Node>(i, std::move(head));
  int s = 0;
  for (const Node* p = head.get(); p; p = p->next.get()) s += p->value;
  return s;
}
static_assert(sum_list(10) == 55);

constexpr bool test() {
  std::unique_ptr<int> a = std::make_unique<int>(1);
  std::unique_ptr<int> b(new int(2));
  if (a == b || !(a != nullptr) || (a <=> a) != 0) return false;
  swap(a, b);
  if (*a != 2 || *b != 1) return false;
  int* r = a.release();
  delete r;
  a.reset(new int(3));
  b = std::move(a);
  if (a || *b != 3) return false;
  std::unique_ptr<int[]> arr = std::make_unique<int[]>(3);
  arr[1] = 5;
  std::unique_ptr<const int[]> carr(std::move(arr));
  if (carr[1] != 5 || arr) return false;
  b = nullptr;
  return !b;
}
static_assert(test());

int main() {
  CHECK(test());
  CHECK(sum_list(4) == 10);
  return 0;
}
