// ADL robustness of <algorithm> and <numeric>.
// [contents]/3: an unqualified name used in the specification of a library declaration
// (other than swap, make_error_code, make_error_condition, from_stream, submdspan_mapping)
// means what unqualified lookup in the context of that declaration finds; so the algorithms
// must not pick up a user's function template found by argument-dependent lookup on the value
// or iterator type (support/adl_poison.hpp declares unconstrained, ill-formed-if-instantiated
// templates named move, copy, distance, advance, next, min, max, fill, sort, merge, ... in the
// namespaces of the types used here).
// [iterator.requirements.general], [iterator.cpp17], [iterator.concepts]: neither a comma
// operator nor a unary & is part of the iterator requirements, and [utility.arg.requirements]
// requires neither of the value type; evil::Iter and evil::Val delete both.
// Every algorithm is also checked for its result ([alg.sort], [alg.merge], [alg.heap.operations],
// [alg.modifying.operations], [alg.nonmodifying], [numeric.ops]).
#include <algorithm>
#include <functional>
#include <numeric>
#include <ranges>
#include "adl_poison.hpp"
#include "check.hpp"

using evil::Iter;
using evil::Val;

template <class T, std::size_t N>
constexpr bool same(const T (&a)[N], std::initializer_list<int> il) {
  auto p = il.begin();
  for (std::size_t i = 0; i < N; ++i, ++p)
    if (a[i].v != *p) return false;
  return true;
}

constexpr bool test_std() {
  Val a[] = {5, 3, 9, 1, 7, 3, 8};
  Iter<Val> b(a), e(a + 7);
  std::sort(b, e);
  if (!same(a, {1, 3, 3, 5, 7, 8, 9})) return false;
  std::reverse(b, e);
  std::stable_sort(b, e);
  if (!same(a, {1, 3, 3, 5, 7, 8, 9})) return false;
  std::reverse(b, e);
  std::partial_sort(b, b + 3, e);
  if (a[0].v != 1 || a[1].v != 3 || a[2].v != 3) return false;
  std::nth_element(b, b + 3, e, std::greater<>());
  if (a[3].v != 5) return false;
  std::make_heap(b, e);
  std::pop_heap(b, e);
  std::push_heap(b, e);
  std::sort_heap(b, e);
  if (!same(a, {1, 3, 3, 5, 7, 8, 9})) return false;
  if (std::lower_bound(b, e, Val(5)) != b + 3 || std::upper_bound(b, e, Val(3)) != b + 3) return false;
  if (!std::binary_search(b, e, Val(8))) return false;
  auto er = std::equal_range(b, e, Val(3));
  if (er.first != b + 1 || er.second != b + 3) return false;
  std::rotate(b, b + 2, e);
  if (!same(a, {3, 5, 7, 8, 9, 1, 3})) return false;
  std::inplace_merge(b, b + 5, e);
  if (!same(a, {1, 3, 3, 5, 7, 8, 9})) return false;
  if (std::is_sorted_until(b, e) != e || !std::is_sorted(b, e)) return false;

  Val out[14];
  Iter<Val> o(out);
  if (std::merge(b, b + 3, b + 3, e, o) != o + 7) return false;
  if (!std::equal(b, e, o, o + 7)) return false;
  if (std::copy(b, e, o) != o + 7 || std::copy_backward(b, e, o + 14) != o + 7) return false;
  if (std::move(b, e, o) != o + 7 || std::move_backward(b, e, o + 14) != o + 7) return false;
  if (std::copy_n(b, 3, o) != o + 3 || std::copy_if(b, e, o, [](Val x) { return x.v > 4; }) != o + 4) return false;
  if (std::set_union(b, b + 4, b + 2, e, o) != o + 7) return false;
  if (std::set_intersection(b, b + 4, b + 2, e, o) != o + 2) return false;
  if (std::set_difference(b, e, b + 1, b + 3, o) != o + 5) return false;
  if (!std::includes(b, e, b + 2, b + 5)) return false;
  if (std::unique(b, e) != b + 6) return false;  // 1 3 5 7 8 9 | x
  a[6] = Val(9);
  if (std::find(b, e, Val(7)) != b + 3 || std::find_if(b, e, [](Val x) { return x.v > 7; }) != b + 4) return false;
  if (std::count(b, e, Val(9)) != 2 || std::adjacent_find(b, e) != b + 5) return false;
  if (std::search(b, e, b + 2, b + 4) != b + 2 || std::search_n(b, e, 2, Val(9)) != b + 5) return false;
  if (std::find_end(b, e, b + 5, b + 6) != b + 6) return false;
  if (std::mismatch(b, e, o).first != b + 1) return false;  // o: 1 5 7 8 9 (set_difference)
  if (*std::min_element(b, e) != Val(1) || *std::max_element(b, e) != Val(9)) return false;
  if (std::minmax_element(b, e).first != b) return false;
  std::fill(o, o + 14, Val(2));
  std::fill_n(o, 2, Val(4));
  std::replace(o, o + 14, Val(4), Val(6));
  if (std::count(o, o + 14, Val(6)) != 2) return false;
  if (std::remove(o, o + 14, Val(6)) != o + 12) return false;
  if (std::remove_if(o, o + 12, [](Val x) { return x.v == 2; }) != o) return false;
  std::transform(b, e, o, [](Val x) { return Val(x.v * 2); });
  if (o[1].v != 6) return false;
  std::swap_ranges(b, b + 2, o);
  std::iter_swap(b, b + 1);
  std::reverse_copy(b, e, o);
  std::rotate_copy(b, b + 1, e, o);
  std::shift_left(b, e, 2);
  std::shift_right(b, e, 2);
  std::generate(o, o + 3, [] { return Val(1); });
  std::for_each(b, e, [](Val&) {});
  std::for_each_n(b, 2, [](Val&) {});

  Val p[] = {3, 1, 2};
  Iter<Val> pb(p), pe(p + 3);
  std::sort(pb, pe);
  if (!std::next_permutation(pb, pe) || !same(p, {1, 3, 2})) return false;
  if (!std::prev_permutation(pb, pe) || !same(p, {1, 2, 3})) return false;
  Val q[] = {2, 3, 1};
  if (!std::is_permutation(pb, pe, Iter<Val>(q))) return false;
  if (std::partition(pb, pe, [](Val x) { return x.v == 2; }) != pb + 1 || p[0].v != 2) return false;
  if (std::partition_point(pb, pb + 1, [](Val x) { return x.v == 2; }) != pb + 1) return false;
  if (!std::is_partitioned(pb, pe, [](Val x) { return x.v == 2; })) return false;
  Val big[] = {3, 0, 0};
  if (!std::lexicographical_compare(pb, pe, Iter<Val>(big), Iter<Val>(big + 3))) return false;  // p[0] == 2
  if (std::lexicographical_compare_three_way(pb, pe, pb, pe) != 0) return false;
  if (std::min({Val(2), Val(1)}) != Val(1) || std::max(Val(2), Val(1)) != Val(2)) return false;
  if (std::clamp(Val(9), Val(1), Val(3)) != Val(3)) return false;

  // <numeric>
  Val n[] = {1, 2, 3, 4};
  Iter<Val> nb(n), ne(n + 4);
  if (std::accumulate(nb, ne, Val(0)) != Val(10)) return false;
  if (std::reduce(nb, ne, Val(0)) != Val(10)) return false;
  if (std::inner_product(nb, ne, nb, Val(0)) != Val(30)) return false;
  if (std::transform_reduce(nb, ne, nb, Val(0)) != Val(30)) return false;
  Val s[4];
  Iter<Val> sb(s);
  if (std::partial_sum(nb, ne, sb) != sb + 4 || s[3].v != 10) return false;
  if (std::inclusive_scan(nb, ne, sb) != sb + 4 || s[3].v != 10) return false;
  if (std::exclusive_scan(nb, ne, sb, Val(0)) != sb + 4 || s[3].v != 6) return false;
  if (std::adjacent_difference(nb, ne, sb) != sb + 4 || s[3].v != 1) return false;
  std::iota(sb, sb + 4, Val(5));
  if (s[3].v != 8) return false;
  return true;
}

constexpr bool test_ranges() {
  namespace r = std::ranges;
  Val a[] = {5, 3, 9, 1, 7, 3, 8};
  r::subrange<Iter<Val>> rg(Iter<Val>(a), Iter<Val>(a + 7));
  if (r::sort(rg) != rg.end() || !same(a, {1, 3, 3, 5, 7, 8, 9})) return false;
  r::reverse(rg);
  r::stable_sort(rg, {}, &Val::v);
  if (!same(a, {1, 3, 3, 5, 7, 8, 9})) return false;
  r::rotate(rg, rg.begin() + 2);
  if (r::inplace_merge(rg, rg.begin() + 5) != rg.end() || !same(a, {1, 3, 3, 5, 7, 8, 9})) return false;
  r::make_heap(rg);
  r::sort_heap(rg);
  r::nth_element(rg, rg.begin() + 3);
  r::partial_sort(rg, rg.begin() + 7);
  if (r::lower_bound(rg, 5, {}, &Val::v) != rg.begin() + 3) return false;
  if (r::find(rg, 7, &Val::v) != rg.begin() + 4 || r::count(rg, Val(3)) != 2) return false;
  if (r::min(rg) != Val(1) || r::max(rg) != Val(9) || r::minmax(rg).max != Val(9)) return false;
  Val out[7];
  if (r::copy(rg, Iter<Val>(out)).out != Iter<Val>(out + 7)) return false;
  if (r::merge(rg.begin(), rg.begin() + 3, rg.begin() + 3, rg.end(), Iter<Val>(out)).out != Iter<Val>(out + 7)) return false;
  if (!r::equal(rg, out)) return false;
  if (r::unique(rg).begin() != rg.begin() + 6) return false;
  if (r::fold_left(rg.begin(), rg.begin() + 3, Val(0), std::plus<>()) != Val(9)) return false;
  r::fill(out, Val(0));
  r::transform(rg, Iter<Val>(out), [](Val x) { return Val(-x.v); });
  if (out[0].v != -1) return false;
  if (!r::next_permutation(rg).found) return false;
  if (!r::is_permutation(rg, out, {}, {}, [](Val x) { return Val(-x.v); })) return false;
  return true;
}

// Global-namespace value type with the poisoned names at global scope.
void test_global() {
  GVal g[] = {3, 1, 2};
  std::sort(g, g + 3);
  CHECK(g[0].v == 1 && g[2].v == 3);
  std::ranges::sort(g, std::ranges::greater());
  CHECK(g[0].v == 3);
  GVal o[3];
  std::copy(g, g + 3, o);
  std::move_backward(g, g + 3, o + 3);
  CHECK(std::ranges::equal(g, o));
  CHECK(*std::max_element(g, g + 3) == GVal(3));
  std::fill(o, o + 3, GVal(0));
  CHECK(std::count(o, o + 3, GVal(0)) == 3);
}

int main() {
  static_assert(test_std());
  static_assert(test_ranges());
  CHECK(test_std());
  CHECK(test_ranges());
  test_global();
  return 0;
}
