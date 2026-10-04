// [container.opt.reqmts]/2-4: a <=> b has type synth-three-way-result<X::value_type> and
// returns lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end(),
// synth-three-way); /1: when the iterators are constexpr iterators the operation is a
// constexpr function. [expos.only.entity]: synth-three-way uses <=> when T models
// three_way_comparable and otherwise derives a weak_ordering from <. [alg.three.way]: the
// first mismatching element decides; if one sequence is a prefix of the other, the shorter
// one is less. Checked for vector (basic_string's <=> is specified separately by
// [string.cmp]).
#include <vector>
#include <compare>
#include <type_traits>
#include "container_values.hpp"
#include "check.hpp"

struct LessOnly {
  int v;
  constexpr LessOnly(int x) : v(x) {}
  friend constexpr bool operator<(const LessOnly& a, const LessOnly& b) { return a.v < b.v; }
  friend constexpr bool operator==(const LessOnly& a, const LessOnly& b) { return a.v == b.v; }
};

struct Weak {
  int v;
  constexpr Weak(int x) : v(x) {}
  friend constexpr std::weak_ordering operator<=>(const Weak& a, const Weak& b) { return a.v <=> b.v; }
  friend constexpr bool operator==(const Weak& a, const Weak& b) { return a.v == b.v; }
};

#include "reqs/container_three_way.hpp"

using namespace reqs::container_three_way;

constexpr bool bool_vec() {
  std::vector<bool> a{false, true}, b{true}, c{false, true, false};
  static_assert(std::is_same_v<decltype(a <=> b), std::strong_ordering>);
  return (a <=> b) < 0 && (a <=> c) < 0 && (c <=> b) < 0 && (a <=> a) == 0;
}

static_assert(test<std::vector<int>, std::strong_ordering>());
static_assert(test<std::vector<Elem>, std::strong_ordering>());
static_assert(test<std::vector<double>, std::partial_ordering>());
static_assert(test<std::vector<LessOnly>, std::weak_ordering>());
static_assert(test<std::vector<Weak>, std::weak_ordering>());
static_assert(bool_vec());

int main() {
  CHECK((test<std::vector<int>, std::strong_ordering>()));
  CHECK((test<std::vector<Elem>, std::strong_ordering>()));
  CHECK((test<std::vector<double>, std::partial_ordering>()));
  CHECK((test<std::vector<LessOnly>, std::weak_ordering>()));
  CHECK((test<std::vector<Weak>, std::weak_ordering>()));
  CHECK(bool_vec());
  return 0;
}
