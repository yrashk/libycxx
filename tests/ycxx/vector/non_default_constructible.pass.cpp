// Only the operations whose preconditions include Cpp17DefaultInsertable (vector(n),
// resize(sz) — [vector.cons]/3, [vector.capacity]/14) need T to be default constructible;
// everything else must work for a T without a default constructor: vector(n, value),
// vector(first, last), push_back / emplace_back, insert / emplace, reserve, shrink_to_fit,
// resize(sz, c) ([vector.capacity]/17: only Cpp17CopyInsertable), assign, erase, swap,
// copy / move ([container.alloc.reqmts]/2: the requirements are expressed via
// allocator_traits::construct with the given arguments only).
#include <vector>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct NoDefault {
  int v;
  NoDefault() = delete;
  constexpr explicit NoDefault(int x) : v(x) {}
  constexpr bool operator==(const NoDefault&) const = default;
};
static_assert(!std::is_default_constructible_v<NoDefault>);

constexpr bool test() {
  using V = std::vector<NoDefault>;
  V v(3, NoDefault(1));
  if (v.size() != 3 || v[2].v != 1) return false;
  v.push_back(NoDefault(2));
  v.emplace_back(3);
  v.insert(v.begin(), NoDefault(0));
  v.emplace(v.begin() + 1, 9);
  v.insert(v.end(), 2, NoDefault(4));
  v.reserve(50);
  v.shrink_to_fit();
  v.resize(12, NoDefault(5));
  if (v.size() != 12 || v[0].v != 0 || v[1].v != 9 || v[11].v != 5) return false;
  v.resize(4, NoDefault(6));
  if (v.size() != 4) return false;
  NoDefault arr[] = {NoDefault(7), NoDefault(8)};
  V w(arr, arr + 2);
  w.assign(3, NoDefault(1));
  w.assign(arr, arr + 1);
  w.assign({NoDefault(3), NoDefault(4)});
  w.erase(w.begin());
  v.swap(w);
  V copy(v);
  V moved(std::move(copy));
  return v.size() == 1 && v[0].v == 4 && moved == v && w.size() == 4;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
