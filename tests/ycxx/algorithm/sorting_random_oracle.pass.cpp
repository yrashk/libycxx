// The sorting algorithms on pseudo-random inputs with few to many distinct keys (down to a
// single key), every size up to 70 and some large ones, checked against a naive stable merge
// sort written here. Elements carry a key and a unique id, so each result is also checked to be
// a permutation of the input.
// [sort]: sorted with respect to comp and proj ([alg.sorting.general]/5). [stable.sort]: in
// addition stable ([algorithm.stable]): equivalent elements keep their relative order, so the
// result equals the oracle exactly. [partial.sort]: [first, middle) holds the first
// middle - first elements of the sorted order, sorted; the rest of the elements are in
// [middle, last). [partial.sort.copy]/5-6: with N = min(last - first, result_last -
// result_first), the first N elements of the sorted order are placed sorted in [result_first,
// result_first + N); returns result_first + N ({last, result_first + N} for ranges).
// [alg.nth.element]/3: *nth is the element that would be there if the range were sorted, and no
// element of [nth, last) is less than one of [first, nth). The ranges:: forms are checked with
// projections and with greater; they return last. Containers: pointers, deque iterators.
#include <algorithm>
#include <deque>
#include <functional>
#include <ranges>
#include <vector>
#include "check.hpp"

struct Rec {
  int key;
  int id;
};

unsigned state = 2463534242u;
unsigned next() {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

// Stable merge sort by key, ascending or descending.
void oracle_sort(std::vector<Rec>& v, bool desc) {
  if (v.size() < 2) return;
  std::vector<Rec> lo(v.begin(), v.begin() + static_cast<long>(v.size() / 2));
  std::vector<Rec> hi(v.begin() + static_cast<long>(v.size() / 2), v.end());
  oracle_sort(lo, desc);
  oracle_sort(hi, desc);
  std::size_t i = 0, j = 0, k = 0;
  while (i < lo.size() && j < hi.size()) {
    bool take_hi = desc ? hi[j].key > lo[i].key : hi[j].key < lo[i].key;
    v[k++] = take_hi ? hi[j++] : lo[i++];
  }
  while (i < lo.size()) v[k++] = lo[i++];
  while (j < hi.size()) v[k++] = hi[j++];
}

std::vector<int> key_of_id;

template <class R>
bool is_permutation_of_input(const R& r) {
  std::vector<char> seen(key_of_id.size(), 0);
  std::size_t n = 0;
  for (const Rec& x : r) {
    if (x.id < 0 || static_cast<std::size_t>(x.id) >= seen.size() || seen[static_cast<std::size_t>(x.id)]) return false;
    seen[static_cast<std::size_t>(x.id)] = 1;
    if (key_of_id[static_cast<std::size_t>(x.id)] != x.key) return false;
    ++n;
  }
  return n == key_of_id.size();
}

template <class It>
bool keys_equal(It first, const std::vector<Rec>& want, std::size_t from, std::size_t to) {
  for (std::size_t i = from; i < to; ++i, ++first)
    if ((*first).key != want[i].key) return false;
  return true;
}
template <class It>
bool exactly_equal(It first, const std::vector<Rec>& want) {
  for (std::size_t i = 0; i < want.size(); ++i, ++first)
    if ((*first).key != want[i].key || (*first).id != want[i].id) return false;
  return true;
}

const auto by_key = [](const Rec& a, const Rec& b) { return a.key < b.key; };
const auto by_key_desc = [](const Rec& a, const Rec& b) { return a.key > b.key; };

void run(std::size_t n, unsigned distinct) {
  std::vector<Rec> in(n);
  key_of_id.assign(n, 0);
  for (std::size_t i = 0; i < n; ++i) {
    in[i] = {static_cast<int>(next() % distinct) - static_cast<int>(distinct / 2), static_cast<int>(i)};
    key_of_id[i] = in[i].key;
  }
  std::vector<Rec> asc = in, desc = in;
  oracle_sort(asc, false);
  oracle_sort(desc, true);

  // sort / stable_sort, std and ranges, vector and deque
  {
    std::vector<Rec> v = in;
    std::sort(v.begin(), v.end(), by_key);
    CHECK(is_permutation_of_input(v) && keys_equal(v.begin(), asc, 0, n));
    v = in;
    std::stable_sort(v.begin(), v.end(), by_key);
    CHECK(exactly_equal(v.begin(), asc));
    v = in;
    CHECK(std::ranges::sort(v, std::ranges::greater{}, &Rec::key) == v.end());
    CHECK(is_permutation_of_input(v) && keys_equal(v.begin(), desc, 0, n));
    v = in;
    CHECK(std::ranges::stable_sort(v.begin(), v.end(), std::ranges::greater{}, &Rec::key) == v.end());
    CHECK(exactly_equal(v.begin(), desc));
    CHECK(std::ranges::is_sorted(v, std::ranges::greater{}, &Rec::key));
    CHECK(std::is_sorted_until(v.begin(), v.end(), by_key_desc) == v.end());

    std::deque<Rec> d(in.begin(), in.end());
    std::sort(d.begin(), d.end(), by_key);
    CHECK(is_permutation_of_input(d) && keys_equal(d.begin(), asc, 0, n));
    d.assign(in.begin(), in.end());
    std::stable_sort(d.begin(), d.end(), by_key);
    CHECK(exactly_equal(d.begin(), asc));
    d.assign(in.begin(), in.end());
    CHECK(std::ranges::stable_sort(d, {}, &Rec::key) == d.end());
    CHECK(exactly_equal(d.begin(), asc));
    // int elements with the default comparator (key order only)
    std::vector<int> iv(n);
    for (std::size_t i = 0; i < n; ++i) iv[i] = in[i].key;
    std::sort(iv.begin(), iv.end());
    for (std::size_t i = 0; i < n; ++i) CHECK(iv[i] == asc[i].key);
    for (std::size_t i = 0; i < n; ++i) iv[i] = in[i].key;
    std::ranges::sort(iv, std::ranges::greater{});
    for (std::size_t i = 0; i < n; ++i) CHECK(iv[i] == desc[i].key);
  }

  // middle / nth positions: ends, neighbours of the ends, the middle and a few random ones
  std::vector<std::size_t> pos = {0, n, n / 2};
  if (n > 0) pos.insert(pos.end(), {1, n - 1, next() % n, next() % n});
  for (std::size_t m : pos) {
    if (m > n) continue;
    const long lm = static_cast<long>(m);
    // partial_sort
    std::vector<Rec> v = in;
    std::partial_sort(v.begin(), v.begin() + lm, v.end(), by_key);
    CHECK(is_permutation_of_input(v) && keys_equal(v.begin(), asc, 0, m));
    v = in;
    CHECK(std::ranges::partial_sort(v, v.begin() + lm, std::ranges::greater{}, &Rec::key) == v.end());
    CHECK(is_permutation_of_input(v) && keys_equal(v.begin(), desc, 0, m));
    std::deque<Rec> d(in.begin(), in.end());
    CHECK(std::ranges::partial_sort(d.begin(), d.begin() + lm, d.end(), {}, &Rec::key) == d.end());
    CHECK(is_permutation_of_input(d) && keys_equal(d.begin(), asc, 0, m));

    // partial_sort_copy into m slots (fewer, equal or more than n)
    for (std::size_t cap : {m, m + 3}) {
      std::vector<Rec> out(cap, Rec{-99999, -1});
      auto r = std::partial_sort_copy(in.begin(), in.end(), out.begin(), out.end(), by_key);
      std::size_t got = std::min(n, cap);
      CHECK(r == out.begin() + static_cast<long>(got));
      CHECK(keys_equal(out.begin(), asc, 0, got));
      for (std::size_t i = got; i < cap; ++i) CHECK(out[i].id == -1);
      std::vector<Rec> out2(cap, Rec{-99999, -1});
      auto rr = std::ranges::partial_sort_copy(in, out2, std::ranges::greater{}, &Rec::key, &Rec::key);
      CHECK(rr.in == in.end() && rr.out == out2.begin() + static_cast<long>(got));
      CHECK(keys_equal(out2.begin(), desc, 0, got));
    }

    // nth_element
    if (m < n) {
      v = in;
      std::nth_element(v.begin(), v.begin() + lm, v.end(), by_key);
      CHECK(is_permutation_of_input(v));
      CHECK(v[m].key == asc[m].key);
      for (std::size_t i = 0; i < m; ++i) CHECK(v[i].key <= v[m].key);
      for (std::size_t i = m; i < n; ++i) CHECK(v[i].key >= v[m].key);
      v = in;
      CHECK(std::ranges::nth_element(v, v.begin() + lm, std::ranges::greater{}, &Rec::key) == v.end());
      CHECK(is_permutation_of_input(v) && v[m].key == desc[m].key);
      for (std::size_t i = 0; i < m; ++i) CHECK(v[i].key >= v[m].key);
      for (std::size_t i = m; i < n; ++i) CHECK(v[i].key <= v[m].key);
      std::deque<Rec> dq(in.begin(), in.end());
      std::ranges::nth_element(dq.begin(), dq.begin() + lm, dq.end(), {}, &Rec::key);
      CHECK(is_permutation_of_input(dq) && dq[m].key == asc[m].key);
    } else {
      v = in;
      std::nth_element(v.begin(), v.end(), v.end(), by_key);  // nth == last: no effect required
      CHECK(is_permutation_of_input(v));
    }
  }
}

int main() {
  const unsigned distinct[] = {1, 2, 3, 7, 50, 1u << 30};
  for (std::size_t n = 0; n <= 70; ++n)
    for (unsigned d : distinct) run(n, d);
  for (std::size_t n : {127u, 128u, 129u, 1000u, 4099u, 30000u})
    for (unsigned d : distinct) run(n, d);
}
