// [sequence.reqmts]/20-23: a.emplace(p, args) has type iterator, inserts an object of type T
// constructed with std::forward<Args>(args)... before p, and returns an iterator to the new
// element; Note 1: "args can directly or indirectly refer to a value in a". /84-87:
// a.emplace_back(args) has type reference, appends T(std::forward<Args>(args)...) and returns
// a.back(). Multiple constructor arguments are forwarded unchanged (rvalues stay rvalues).
#include <vector>
#include <string>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

struct Two {
  int a;
  int b;
  bool from_rvalue = false;
  constexpr Two(int x, int y) : a(x), b(y) {}
  constexpr Two(const Elem& e, int y) : a(e.value()), b(y) {}
  constexpr Two(Elem&& e, int y) : a(e.value()), b(y), from_rvalue(true) {}
  constexpr bool operator==(const Two& o) const { return a == o.a && b == o.b; }
};

#include "reqs/sequence_emplace.hpp"

using namespace reqs::sequence_emplace;

constexpr bool multi_arg() {
  std::vector<Two> v;
  Two& r = v.emplace_back(1, 2);
  if (&r != &v.back() || r.a != 1 || r.b != 2) return false;
  auto it = v.emplace(v.cbegin(), 3, 4);
  if (it != v.begin() || v[0].a != 3 || v.size() != 2) return false;
  Elem e(9);
  v.emplace_back(e, 1);
  if (v.back().from_rvalue || e.value() != 9) return false;
  v.emplace_back(std::move(e), 1);
  if (!v.back().from_rvalue) return false;
  v.emplace(v.cbegin() + 1, Elem(5), 0);
  if (!v[1].from_rvalue || v[1].a != 5) return false;
  if (std::addressof(v.emplace_back(0, 0)) != std::addressof(v.back())) return false;
  return true;
}

static_assert(generic<std::vector<int>>());
static_assert(generic<std::vector<Elem>>());
static_assert(generic<std::vector<bool>>());
static_assert(multi_arg());

int main() {
  CHECK(generic<std::vector<int>>());
  CHECK(generic<std::vector<Elem>>());
  CHECK(generic<std::vector<bool>>());
  CHECK(generic<std::vector<std::string>>());
  CHECK(multi_arg());
  return 0;
}
