// Empty ranges given as two null pointers (or a null pointer and a count of zero) for the
// algorithms, the uninitialized-memory algorithms, the containers' range members, span, format
// and charconv. A null pointer value is a valid iterator of an empty range
// ([iterator.requirements.general]/8: [i, i) is an empty range; [expr.add]/4.1: null + 0 is
// null), so every call below is defined and must make no access. The library must not pass the
// null pointer on to memmove/memcpy/memcmp/memset (ISO C 7.26.1/2): run under SANITIZER=ubsan,
// whose nonnull-attribute check reports such a call. Trivially copyable element types are used
// because they are the ones for which an implementation may call those functions.
//   [alg.copy]/? copy, copy_n (n = 0), copy_backward, move, move_backward return result
//     (+ 0); [alg.fill] fill_n with n = 0 returns first; [alg.equal], [alg.lex.comparison]
//     (two empty ranges: equal is true, lexicographical_compare false), [mismatch], [alg.find],
//     [alg.count], [alg.search] (empty pattern: returns first1), [alg.reverse], [alg.rotate]
//     (returns first + (last - middle) = first), [alg.sort], [alg.unique], [alg.transform],
//     [alg.replace], [alg.remove], [alg.min.max] (min_element of an empty range returns last).
//   [uninitialized.copy], [uninitialized.fill], [specialized.destroy]: return result / no effect.
//   [sequence.reqmts] X(i, j), a.assign(i, j), a.insert(p, i, j) with i == j.
//   [span.cons]/4: span(first, count) "Preconditions: [first, first + count) is a valid range".
//   [format.functions] format_to_n(out, 0, ...): "M = clamp(n, 0, N)", writes [out, out + M),
//     returns {out + M, N}.
//   [charconv.to.chars]/1: on failure "ec has the value errc::value_too_large, ptr has the value
//     last"; [charconv.from.chars]/1: no match: "ptr is first and ec is errc::invalid_argument".
#include <algorithm>
#include <charconv>
#include <cstring>
#include <deque>
#include <format>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <system_error>
#include <vector>
#include "check.hpp"

template <class T>
void algorithms() {
  T* n = nullptr;
  const T* c = nullptr;
  T one[1] = {T(1)};
  CHECK(std::copy(c, c, n) == n);
  CHECK(std::copy(c, c, one) == one && one[0] == T(1));
  CHECK(std::copy_n(c, 0, n) == n);
  CHECK(std::copy_backward(c, c, n) == n);
  CHECK(std::move(n, n, n) == n);
  CHECK(std::move_backward(n, n, n) == n);
  CHECK(std::copy_if(c, c, n, [](T) { return true; }) == n);
  std::fill(n, n, T(3));
  CHECK(std::fill_n(n, 0, T(3)) == n);
  CHECK(std::equal(c, c, c));
  CHECK(std::equal(c, c, c, c));
  CHECK(std::equal(c, c, one, one));
  CHECK(!std::lexicographical_compare(c, c, c, c));
  CHECK(std::lexicographical_compare(c, c, one, one + 1));
  CHECK(std::lexicographical_compare_three_way(c, c, c, c) == 0);
  CHECK(std::mismatch(c, c, c, c) == std::pair(c, c));
  CHECK(std::find(c, c, T(1)) == c);
  CHECK(std::count(c, c, T(1)) == 0);
  CHECK(std::search(c, c, c, c) == c);
  CHECK(std::search(one, one + 1, c, c) == one);
  CHECK(std::find_end(c, c, c, c) == c);
  std::reverse(n, n);
  CHECK(std::rotate(n, n, n) == n);
  std::sort(n, n);
  std::stable_sort(n, n);
  CHECK(std::is_sorted(c, c));
  CHECK(std::unique(n, n) == n);
  CHECK(std::transform(c, c, n, [](T x) { return x; }) == n);
  std::replace(n, n, T(1), T(2));
  CHECK(std::remove(n, n, T(1)) == n);
  CHECK(std::min_element(c, c) == c && std::max_element(c, c) == c);
  CHECK(std::accumulate(c, c, T(5)) == T(5));
  CHECK(std::inner_product(c, c, c, T(5)) == T(5));
  CHECK(std::partial_sum(c, c, n) == n);
  CHECK(std::adjacent_difference(c, c, n) == n);
  std::iota(n, n, T(0));
  std::swap_ranges(n, n, n);

  CHECK(std::ranges::copy(c, c, n).out == n);
  CHECK(std::ranges::copy(std::span<const T>(), n).out == n);
  CHECK(std::ranges::equal(std::span<const T>(), std::span<const T>()));
  CHECK(std::ranges::fill(n, n, T(1)) == n);
  CHECK(std::ranges::move(n, n, n).out == n);
  CHECK(std::ranges::copy_backward(c, c, n).out == n);
  CHECK(!std::ranges::lexicographical_compare(std::span<const T>(), std::span<const T>()));
  CHECK(std::ranges::search(std::span<const T>(), std::span<const T>()).empty());

  CHECK(std::uninitialized_copy(c, c, n) == n);
  CHECK(std::uninitialized_copy_n(c, 0, n) == n);
  CHECK(std::uninitialized_move(n, n, n) == n);
  std::uninitialized_fill(n, n, T(1));
  CHECK(std::uninitialized_fill_n(n, 0, T(1)) == n);
  std::uninitialized_default_construct(n, n);
  std::uninitialized_value_construct(n, n);
  CHECK(std::uninitialized_value_construct_n(n, 0) == n);
  std::destroy(n, n);
  CHECK(std::destroy_n(n, 0) == n);
  CHECK(std::ranges::uninitialized_copy(c, c, n, n).out == n);
  CHECK(std::ranges::uninitialized_fill(n, n, T(1)) == n);

  std::vector<T> v(c, c);
  CHECK(v.empty());
  v.assign(c, c);
  CHECK(v.insert(v.end(), c, c) == v.end());
  v.append_range(std::span<const T>());
  v.assign_range(std::span<const T>(c, std::size_t(0)));
  CHECK(v.empty());
  v.push_back(T(1));
  v.insert(v.begin(), c, c);
  v.assign(one, one + 1);
  CHECK(v.size() == 1 && v[0] == T(1));
  std::vector<T> w(v.begin(), v.begin());
  CHECK(w.empty() && w == std::vector<T>());
  std::deque<T> d(c, c);
  d.insert(d.end(), c, c);
  CHECK(d.empty());
  std::span<const T> s(c, std::size_t(0));
  CHECK(s.empty() && s.data() == nullptr && s.first(0).empty() && s.subspan(0).empty());
  std::span<T> sn(n, n);
  CHECK(sn.empty());
}

int main() {
  algorithms<char>();
  algorithms<unsigned char>();
  algorithms<int>();
  algorithms<long long>();
  algorithms<double>();

  char* out = nullptr;
  auto r = std::format_to_n(out, 0, "{}-{}", 42, "abc");
  CHECK(r.out == nullptr && r.size == 6);
  auto rw = std::format_to_n(static_cast<wchar_t*>(nullptr), 0, L"{}", 7);
  CHECK(rw.out == nullptr && rw.size == 1);
  CHECK(std::formatted_size("{}", "") == 0);
  CHECK(std::format("{}", std::string_view()) == "");
  CHECK(std::format("{:>3}", std::string_view()) == "   ");

  auto tc = std::to_chars(out, out, 5);
  CHECK(tc.ec == std::errc::value_too_large && tc.ptr == nullptr);
  auto tf = std::to_chars(out, out, 1.5);
  CHECK(tf.ec == std::errc::value_too_large && tf.ptr == nullptr);
  int x = 9;
  auto fc = std::from_chars(static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), x);
  CHECK(fc.ec == std::errc::invalid_argument && fc.ptr == nullptr && x == 9);
  double y = 9;
  auto ff = std::from_chars(static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), y);
  CHECK(ff.ec == std::errc::invalid_argument && ff.ptr == nullptr && y == 9);
  return 0;
}
