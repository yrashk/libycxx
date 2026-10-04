// ADL robustness with values of type Holder<Incomplete>*, where Holder<Incomplete> cannot be
// instantiated. [contents]/3: an unqualified name used in the specification of a library
// declaration (other than swap, make_error_code, make_error_condition, from_stream,
// submdspan_mapping) means what unqualified lookup in the context of that declaration finds, so
// the algorithms below perform no argument-dependent lookup on the value type; the only
// operations they need on a pointer value are copy, assignment and comparison
// ([alg.nonmodifying], [alg.modifying.operations], [alg.sorting], [accumulate]), none of which
// needs Holder<Incomplete> complete. Algorithms that swap are not used: swap is looked up by
// ADL on purpose ([swappable.requirements]).
#include <algorithm>
#include <functional>
#include <numeric>
#include <ranges>
#include "adl_poison.hpp"
#include "check.hpp"

using HP = evil::Holder<evil::Incomplete>*;
int main() {
  alignas(16) static char buf[16 * 8];
  HP v[8];
  for (int i = 0; i < 8; ++i) v[i] = reinterpret_cast<HP>(buf + 16 * (7 - i % 4));
  HP out[8] = {};
  CHECK(std::find(v, v + 8, v[3]) == v + 3);
  CHECK(std::count(v, v + 8, v[0]) == 2);
  CHECK(std::copy(v, v + 8, out) == out + 8);
  CHECK(std::equal(v, v + 8, out));
  CHECK(std::mismatch(v, v + 8, out).first == v + 8);
  CHECK(std::search(v, v + 8, v + 4, v + 6) == v);
  CHECK(std::adjacent_find(v, v + 8) == v + 8);
  CHECK(*std::min_element(v, v + 8, std::less<>()) == v[3]);
  CHECK(std::is_sorted(v, v + 4, std::greater<>()));
  CHECK(std::lower_bound(v, v + 4, v[2], std::greater<>()) == v + 2);
  CHECK(std::merge(v, v + 4, v + 4, v + 8, out, std::greater<>()) == out + 8);
  CHECK(std::unique(out, out + 8) == out + 4);
  std::fill(out, out + 8, nullptr);
  CHECK(std::remove(out, out + 8, nullptr) == out);
  const HP old_value = v[0], new_value = v[1];  // not references into v
  std::replace(v, v + 8, old_value, new_value);
  CHECK(std::count(v, v + 8, new_value) == 4);
  CHECK(std::ranges::find(v, v[2]) == v + 2);
  CHECK(std::ranges::count(v, v[1]) == 4);
  CHECK(std::ranges::copy(v, out).out == out + 8);
  CHECK(std::ranges::equal(v, out));
  CHECK(std::accumulate(v, v + 8, 0, [](int n, HP) { return n + 1; }) == 8);
  return 0;
}
