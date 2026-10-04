// Inserting members of deque whose value argument refers to an element of the same deque.
// [sequence.reqmts]/24-27, /32-35: a.insert(p, t) "Inserts a copy of t before p";
// a.insert(p, n, t) "Inserts n copies of t before p" - unlike a.assign(n, t) (/67: "t is not a
// reference into a") there is no precondition excluding t referring into a.
// [sequence.reqmts]/22 Note 1 (emplace): "args can directly or indirectly refer to a value in a";
// [sequence.reqmts] push_front/push_back/emplace_front/emplace_back: prepend/append a copy of t
// (an object constructed from args). [deque.capacity]/4: resize(sz, c) "appends sz - size()
// copies of c". The value inserted is the value the element had before the call, at either end
// and in the middle (where elements are shifted towards the nearer end, [deque.modifiers]/2),
// including when new blocks are allocated.
#include <deque>
#include <string>
#include "check.hpp"

template <class T>
bool test(const T (&v)[6]) {
  using D = std::deque<T>;
  auto fresh = [&](int extra_front) {
    D d;
    for (int i = 0; i < 6; ++i) d.push_back(v[i]);
    // shift the start position inside the first block in different ways
    for (int i = 0; i < extra_front; ++i) d.push_front(v[0]);
    for (int i = 0; i < extra_front; ++i) d.pop_front();
    return d;
  };
  for (int extra : {0, 1, 7, 64, 513}) {
    D d = fresh(extra);
    d.insert(d.cbegin() + 1, d[4]);  // near the front, value from the back half
    if (d != D{v[0], v[4], v[1], v[2], v[3], v[4], v[5]}) return false;
    d = fresh(extra);
    d.insert(d.cbegin() + 5, d[0]);  // near the back, value from the front half
    if (d != D{v[0], v[1], v[2], v[3], v[4], v[0], v[5]}) return false;
    d = fresh(extra);
    d.insert(d.cbegin() + 2, d[1]);  // the value is in the part that moves to the front
    if (d != D{v[0], v[1], v[1], v[2], v[3], v[4], v[5]}) return false;
    d = fresh(extra);
    d.insert(d.cbegin() + 4, d[5]);  // the value is in the part that moves to the back
    if (d != D{v[0], v[1], v[2], v[3], v[5], v[4], v[5]}) return false;
    d = fresh(extra);
    d.insert(d.cbegin(), d.back());
    d.insert(d.cend(), d.front());
    if (d != D{v[5], v[0], v[1], v[2], v[3], v[4], v[5], v[5]}) return false;

    d = fresh(extra);
    d.insert(d.cbegin() + 1, 3, d[2]);
    if (d != D{v[0], v[2], v[2], v[2], v[1], v[2], v[3], v[4], v[5]}) return false;
    d = fresh(extra);
    d.insert(d.cbegin() + 4, 3, d[3]);
    if (d != D{v[0], v[1], v[2], v[3], v[3], v[3], v[3], v[4], v[5]}) return false;
    d = fresh(extra);
    d.insert(d.cbegin() + 3, 2, d[0]);
    if (d != D{v[0], v[1], v[2], v[0], v[0], v[3], v[4], v[5]}) return false;
    d = fresh(extra);
    d.insert(d.cbegin() + 3, 2, d[5]);
    if (d != D{v[0], v[1], v[2], v[5], v[5], v[3], v[4], v[5]}) return false;
    d = fresh(extra);
    d.insert(d.cbegin(), 600, d[3]);  // several new blocks at the front
    if (d.size() != 606 || d[0] != v[3] || d[599] != v[3] || d[600] != v[0] || d[603] != v[3]) return false;
    d = fresh(extra);
    d.insert(d.cend(), 600, d[2]);  // several new blocks at the back
    if (d.size() != 606 || d[5] != v[5] || d[6] != v[2] || d[605] != v[2]) return false;

    d = fresh(extra);
    d.emplace(d.cbegin() + 2, d[4]);
    if (d != D{v[0], v[1], v[4], v[2], v[3], v[4], v[5]}) return false;
    d = fresh(extra);
    d.emplace(d.cbegin() + 4, d[1]);
    if (d != D{v[0], v[1], v[2], v[3], v[1], v[4], v[5]}) return false;

    d = fresh(extra);
    for (int i = 0; i < 700; ++i) d.push_back(d.front());
    for (int i = 0; i < 700; ++i) d.push_front(d.back());
    if (d.size() != 1406 || d.front() != v[0] || d.back() != v[0] || d[700] != v[0] || d[705] != v[5])
      return false;
    d = fresh(extra);
    for (int i = 0; i < 700; ++i) d.emplace_back(d[1]);
    for (int i = 0; i < 700; ++i) d.emplace_front(d[d.size() - 1]);
    if (d.size() != 1406 || d.front() != v[1] || d.back() != v[1] || d[701] != v[1]) return false;

    d = fresh(extra);
    d.resize(800, d.back());
    if (d.size() != 800 || d[5] != v[5] || d[6] != v[5] || d[799] != v[5]) return false;
    d = fresh(extra);
    d.resize(800, d.front());
    if (d.size() != 800 || d[0] != v[0] || d[799] != v[0] || d[5] != v[5]) return false;
  }
  return true;
}

int main() {
  const int ints[6] = {10, 11, 12, 13, 14, 15};
  CHECK(test(ints));
  const std::string strs[6] = {"zero, a string longer than any small buffer", "one, a string longer than any small buffer",
                               "two", "three, a string longer than any small buffer",
                               "four, a string longer than any small buffer", "five"};
  CHECK(test(strs));
  return 0;
}
