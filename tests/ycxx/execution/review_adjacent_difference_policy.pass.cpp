// [adjacent.difference]/5: for the overloads with an ExecutionPolicy and a non-empty range,
// "performs *result = *first. Then, for every d in [1, last - first - 1], performs
// *(result + d) = binary_op(*(first + d), *(first + (d - 1)))". Unlike the overloads without a
// policy (/4: an accumulator acc and a copy val of type T, binary_op(val, std::move(acc))),
// binary_op receives the input elements themselves, as lvalues. /2.2: only binary_op(*first,
// *first) and *first need to be writable to result (T need not be copy-constructible).
#include <numeric>
#include <execution>
#include <functional>
#include "check.hpp"

// Copy-assignable but not copy-constructible: the policy overloads never make a T.
struct Assignable {
  int v = 0;
  Assignable() = default;
  explicit Assignable(int x) : v(x) {}
  Assignable(const Assignable&) = delete;
  Assignable& operator=(const Assignable&) = default;
};
struct Diff {
  int operator()(const Assignable& a, const Assignable& b) const { return a.v - b.v; }
};
struct IntOut {
  int v = 0;
  IntOut& operator=(int x) { v = x; return *this; }
  IntOut& operator=(const Assignable& a) { v = a.v; return *this; }
};

template <class P>
void run(const P& pol) {
  int in[5] = {1, 4, 9, 16, 25};
  int out[5] = {};
  const int* seen_first[4] = {};
  const int* seen_second[4] = {};
  int calls = 0;
  // Non-const lvalue references: the elements are passed, never an rvalue accumulator.
  auto op = [&](int& cur, int& prev) {
    seen_first[calls] = &cur;
    seen_second[calls] = &prev;
    ++calls;
    return cur - prev;
  };
  CHECK(std::adjacent_difference(pol, in, in + 5, out, op) == out + 5);
  CHECK(calls == 4);
  for (int d = 1; d < 5; ++d) {
    CHECK(out[d] == in[d] - in[d - 1]);
    CHECK(seen_first[d - 1] == in + d);
    CHECK(seen_second[d - 1] == in + d - 1);
  }
  CHECK(out[0] == 1);

  Assignable a[3];
  a[0].v = 2;
  a[1].v = 7;
  a[2].v = 3;
  IntOut o[3];
  CHECK(std::adjacent_difference(pol, a, a + 3, o, Diff{}) == o + 3);
  CHECK(o[0].v == 2 && o[1].v == 5 && o[2].v == -4);

  int e[1] = {};
  CHECK(std::adjacent_difference(pol, in, in, e) == e);
  CHECK(std::adjacent_difference(pol, in, in + 3, out) == out + 3);
  CHECK(out[0] == 1 && out[1] == 3 && out[2] == 5);
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
}
