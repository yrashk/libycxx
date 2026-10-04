// The element requirements of vector ([container.alloc.reqmts]/2: Cpp17DefaultInsertable,
// Cpp17CopyInsertable, Cpp17MoveInsertable, Cpp17EmplaceConstructible, Cpp17Erasable;
// [utility.arg.requirements]) say nothing about unary operator&, so a vector of a type whose
// operator& is overloaded (here: deleted for non-const lvalues and returning garbage for
// const ones) must still work; the library takes addresses with addressof
// ([specialized.addressof]) / to_address. [vector.data]/1: data() == addressof(front()).
#include <vector>
#include <memory>
#include <utility>
#include "check.hpp"

struct Evil {
  int v = 0;
  Evil() = default;
  Evil(int x) : v(x) {}
  Evil(const Evil&) = default;
  Evil(Evil&&) noexcept = default;
  Evil& operator=(const Evil&) = default;
  Evil& operator=(Evil&&) noexcept = default;
  void operator&() = delete;
  const Evil* operator&() const { return nullptr; }
  friend bool operator==(const Evil& a, const Evil& b) { return a.v == b.v; }
};

int main() {
  std::vector<Evil> v;
  v.push_back(Evil(1));
  Evil e(2);
  v.push_back(e);
  v.emplace_back(3);
  v.insert(v.begin(), Evil(0));
  v.insert(v.begin() + 1, 2, e);
  v.emplace(v.begin(), -1);
  v.reserve(100);
  v.resize(10);
  v.resize(12, e);
  v.shrink_to_fit();
  CHECK(v.size() == 12 && v[0].v == -1 && v[1].v == 0 && v[2].v == 2 && v[4].v == 1);
  CHECK(v.data() == std::addressof(v.front()));
  CHECK(std::to_address(v.begin() + 3) == std::addressof(v[3]));
  v.erase(v.begin() + 1);
  v.erase(v.begin(), v.begin() + 2);
  v.pop_back();
  std::vector<Evil> w(v);
  CHECK(w == v);
  std::vector<Evil> x(std::move(w));
  w = x;
  w.assign(3, e);
  w.assign({Evil(7), Evil(8)});
  CHECK(w.size() == 2 && w[1].v == 8);
  w.swap(x);
  x.clear();
  std::vector<Evil> y(5);
  std::vector<Evil> z(5, e);
  Evil arr[3] = {Evil(1), Evil(2), Evil(3)};
  std::vector<Evil> r(arr, arr + 3);
  r.insert(r.end(), arr, arr + 3);
  CHECK(r.size() == 6 && r[5].v == 3 && y.size() == 5 && z[4].v == 2);
  return 0;
}
