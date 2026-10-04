// [container.reqmts]/17-18: t = v has postcondition t == v, and Cpp17CopyAssignable
// ([utility.arg.requirements], Table 35) adds "the value of v is unchanged" — so copy
// self-assignment leaves the vector's value as it was. [container.reqmts]/22: for t = rv the
// value postcondition applies only "if t and rv do not refer to the same object";
// [lib.types.movedfrom] / [res.on.arguments]/1.3 leave a self-moved object in a valid but
// unspecified state, so it must remain usable. a = il / assign from the vector's own
// elements is not exercised (assign(i, j) forbids it, [sequence.reqmts]/58).
// self-swap ([container.reqmts]/49) leaves the value unchanged.
#include <vector>
#include <string>
#include <utility>
#include "check.hpp"

template <class T>
constexpr bool test(std::vector<T> v) {
  const std::vector<T> orig = v;
  std::vector<T>& alias = v;
  v = alias;
  if (v != orig) return false;
  v.swap(alias);
  if (v != orig) return false;
  using std::swap;
  swap(v, alias);
  if (v != orig) return false;
  v = std::move(alias);
  v.clear();  // valid state: every operation without preconditions works
  v.push_back(orig.front());
  v.insert(v.end(), orig.begin(), orig.end());
  return v.size() == orig.size() + 1;
}

static_assert(test<int>({1, 2, 3}));
static_assert(test<bool>({true, false, true}));
static_assert(test<std::string>({"a", "a string long enough to be on the heap"}));

int main() {
  CHECK(test<int>({1, 2, 3}));
  CHECK(test<bool>({true, false, true}));
  CHECK(test<std::string>({"a", "a string long enough to be on the heap"}));
  CHECK(test<std::vector<int>>({{1}, {2, 3}}));
  return 0;
}
