// The associative containers report errors and keep their exception guarantees during constant
// evaluation as at run time (P3068; map, set, unordered_map, unordered_set and flat_map are
// constexpr in C++26, P3372):
// [map.access]/6, /11: at(x) and the heterogeneous at(x) throw out_of_range if there is no such
//   element; [unord.map.elem]/6, /10 and [flat.map.access]/6, /11: the same;
// [associative.reqmts.except]/2: if any operation from within an insert or emplace of a single
//   element throws (the element's constructor or the Compare object), the insertion has no
//   effect;
// [unord.req.except]/2: likewise for the unordered containers, for any operation other than the
//   hash function (here: the element's constructor and the equality predicate).
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <cstddef>
#include <flat_map>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "check.hpp"

template <class E, class F>
constexpr bool throws(F f) {
  try {
    f();
  } catch (const E&) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

// A transparent comparator / hash, for the heterogeneous at(K).
struct less_sv {
  using is_transparent = void;
  constexpr bool operator()(std::string_view a, std::string_view b) const { return a < b; }
};
struct hash_sv {
  using is_transparent = void;
  constexpr std::size_t operator()(std::string_view s) const {
    std::size_t h = 0;
    for (char c : s) h = h * 31 + static_cast<unsigned char>(c);
    return h;
  }
};

constexpr bool at_members() {
  int ok = 0;
  std::map<int, int> m{{1, 10}, {2, 20}};
  ok += throws<std::out_of_range>([&] { (void)m.at(3); });
  ok += throws<std::out_of_range>([&] { (void)std::as_const(m).at(0); });
  ok += m.at(2) == 20;
  std::map<std::string_view, int, less_sv> hm{{"a", 1}};
  ok += throws<std::out_of_range>([&] { (void)hm.at(std::string_view("b")); });
  std::unordered_map<int, int> u{{1, 10}};
  ok += throws<std::out_of_range>([&] { (void)u.at(2); });
  ok += throws<std::out_of_range>([&] { (void)std::as_const(u).at(2); });
  std::unordered_map<std::string_view, int, hash_sv, std::equal_to<>> hu{{"a", 1}};
  ok += throws<std::out_of_range>([&] { (void)hu.at(std::string_view("zz")); });
  std::flat_map<int, int> f{{1, 10}};
  ok += throws<std::out_of_range>([&] { (void)f.at(2); });
  ok += throws<std::out_of_range>([&] { (void)std::as_const(f).at(2); });
  return ok == 9 && m.size() == 2 && u.size() == 1 && f.size() == 1;
}
static_assert(at_members());

// An element whose construction from a negative int throws.
struct elem {
  int v;
  constexpr elem(int x) : v(x) {
    if (x < 0) throw std::invalid_argument("negative");
  }
  constexpr bool operator==(const elem&) const = default;
  constexpr auto operator<=>(const elem&) const = default;
};
struct elem_hash {
  constexpr std::size_t operator()(const elem& e) const { return static_cast<std::size_t>(e.v) % 7; }
};

// A comparator / predicate that throws when it sees a given value.
struct picky_less {
  int bad;
  constexpr bool operator()(int a, int b) const {
    if (a == bad || b == bad) throw std::runtime_error("compare");
    return a < b;
  }
};
struct picky_eq {
  int bad;
  constexpr bool operator()(int a, int b) const {
    if (a == bad || b == bad) throw std::runtime_error("equal");
    return a == b;
  }
};
struct mod_hash {
  constexpr std::size_t operator()(int x) const { return static_cast<std::size_t>(x) % 3; }
};

constexpr bool single_insertions() {
  int ok = 0;
  std::set<elem> s{1, 2, 3};
  ok += throws<std::invalid_argument>([&] { s.emplace(-1); });
  ok += throws<std::invalid_argument>([&] { s.emplace_hint(s.begin(), -2); });
  ok += s.size() == 3 && !s.contains(elem(0));
  std::map<int, elem> m{{1, 1}};
  ok += throws<std::invalid_argument>([&] { m.emplace(2, -1); });
  ok += throws<std::invalid_argument>([&] { m.try_emplace(3, -1); });
  ok += m.size() == 1 && !m.contains(2) && !m.contains(3);
  std::map<int, int, picky_less> pm(picky_less{99});
  pm.emplace(1, 1);
  pm.emplace(5, 5);
  ok += throws<std::runtime_error>([&] { pm.emplace(99, 0); });
  ok += pm.size() == 2;
  std::unordered_set<elem, elem_hash> us{1, 2};
  ok += throws<std::invalid_argument>([&] { us.emplace(-3); });
  ok += us.size() == 2;
  std::unordered_map<int, int, mod_hash, picky_eq> um(4, mod_hash{}, picky_eq{3});
  um.emplace(0, 0);
  um.emplace(1, 1);
  // 3 hashes like 0: inserting it compares with the predicate, which throws
  ok += throws<std::runtime_error>([&] { um.emplace(3, 3); });
  ok += um.size() == 2 && um.at(0) == 0 && um.at(1) == 1;
  return ok == 12;
}
static_assert(single_insertions());

int main() {
  CHECK(at_members());
  CHECK(single_insertions());
}
