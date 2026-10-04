// Parallel algorithm overloads in namespace std, with each of the four standard policies.
// [algorithms.parallel.overloads]/2: "Unless otherwise specified, the semantics of calling a
// parallel algorithm overload are identical to calling the corresponding algorithm overload
// without the parameter P, using all but the first argument." So every result below is the
// one the non-parallel overload gives ([alg.foreach], [alg.all.of], [alg.find], [alg.count],
// [alg.copy], [alg.transform], [alg.fill], [alg.replace], [alg.remove], [alg.unique],
// [alg.reverse], [alg.rotate], [sort], [stable.sort], [partial.sort], [alg.nth.element],
// [alg.merge], [alg.min.max], [alg.equal], [mismatch], [alg.search], [set.union],
// [includes], [alg.partitions], [alg.swap], [alg.lex.comparison], [is.sorted],
// [uninitialized.copy]). The parallel overloads take forward iterators ([algorithms.
// requirements]/4: ForwardIterator), so a forward-only iterator is used for some calls;
// stability of stable_sort, stable_partition and merge is part of the semantics.
#include <algorithm>
#include <execution>
#include <functional>
#include <memory>
#include <numeric>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

constexpr int N = 1000;

template <class P>
void run(P&& pol) {
  static int a[N], b[N], c[2 * N];
  for (int i = 0; i < N; ++i) a[i] = (i * 37) % N;  // a permutation of 0..N-1

  // for_each / for_each_n: return void / first + n
  static_assert(std::is_void_v<decltype(std::for_each(pol, a, a + N, [](int&) {}))>);
  std::for_each(pol, a, a + N, [](int& x) { x += 1; });
  CHECK(std::for_each_n(pol, a, N, [](int& x) { x -= 1; }) == a + N);
  CHECK(std::accumulate(a, a + N, 0LL) == 1LL * N * (N - 1) / 2);

  CHECK(std::all_of(pol, a, a + N, [](int x) { return x >= 0; }));
  CHECK(std::any_of(pol, a, a + N, [](int x) { return x == N - 1; }));
  CHECK(std::none_of(pol, a, a + N, [](int x) { return x >= N; }));
  CHECK(std::find(pol, a, a + N, 37) == a + 1);
  CHECK(std::find_if(pol, a, a + N, [](int x) { return x == 74; }) == a + 2);
  CHECK(std::find_if_not(pol, a, a + N, [](int x) { return x < 100; }) == a + 3);  // 111
  CHECK(std::count(pol, a, a + N, 5) == 1);
  CHECK(std::count_if(pol, a, a + N, [](int x) { return x % 2 == 0; }) == N / 2);
  CHECK(std::adjacent_find(pol, a, a + N) == a + N);

  CHECK(std::copy(pol, a, a + N, b) == b + N);
  CHECK(std::equal(pol, a, a + N, b));
  CHECK(std::equal(pol, a, a + N, b, b + N));
  CHECK(std::copy_n(pol, a, 10, b) == b + 10);
  CHECK(std::copy_if(pol, a, a + N, b, [](int x) { return x < 10; }) == b + 10);
  CHECK(std::mismatch(pol, a, a + N, b).first == a + 1);  // b[0] == a[0] == 0, b[1] == 1 != 37
  CHECK(std::transform(pol, a, a + N, b, [](int x) { return 2 * x; }) == b + N);
  CHECK(b[1] == 74);
  CHECK(std::transform(pol, a, a + N, b, c, std::plus<>()) == c + N);
  CHECK(c[1] == 111);

  // forward iterators suffice
  {
    int d[N];
    CHECK(std::copy(pol, ForwardIter<int>(a), ForwardIter<int>(a + N), ForwardIter<int>(d)).p == d + N);
    CHECK(std::equal(pol, a, a + N, d));
    CHECK(std::find(pol, ForwardIter<int>(a), ForwardIter<int>(a + N), 74).p == a + 2);
  }

  std::fill(pol, b, b + N, 3);
  CHECK(std::fill_n(pol, b, 5, 4) == b + 5);
  std::replace(pol, b, b + N, 4, 6);
  CHECK(std::count(pol, b, b + N, 6) == 5);
  std::replace_if(pol, b, b + N, [](int x) { return x == 3; }, 7);
  CHECK(std::count(pol, b, b + N, 7) == N - 5);
  std::generate(pol, b, b + N, [] { return 1; });
  CHECK(std::generate_n(pol, b, 3, [] { return 2; }) == b + 3);
  CHECK(std::remove(pol, b, b + N, 2) == b + N - 3);
  CHECK(std::remove_if(pol, b, b + N - 3, [](int x) { return x == 1; }) == b);

  // unique keeps the first of each run
  for (int i = 0; i < N; ++i) b[i] = i / 3;
  int* u = std::unique(pol, b, b + N);
  CHECK(u - b == (N + 2) / 3);
  for (int i = 0; i < u - b; ++i) CHECK(b[i] == i);

  std::copy(a, a + N, b);
  std::reverse(pol, b, b + N);
  CHECK(b[0] == a[N - 1] && b[N - 1] == a[0]);
  CHECK(std::rotate(pol, b, b + 10, b + N) == b + N - 10);
  CHECK(b[N - 10] == a[N - 1]);
  CHECK(std::reverse_copy(pol, a, a + N, b) == b + N && b[0] == a[N - 1]);
  CHECK(std::rotate_copy(pol, a, a + 1, a + N, b) == b + N && b[N - 1] == a[0]);
  CHECK(std::swap_ranges(pol, a, a + N, b) == b + N);
  CHECK(std::swap_ranges(pol, a, a + N, b) == b + N);
  CHECK(b[N - 1] == a[0]);

  // sorting
  std::copy(a, a + N, b);
  std::sort(pol, b, b + N);
  for (int i = 0; i < N; ++i) CHECK(b[i] == i);
  CHECK(std::is_sorted(pol, b, b + N));
  CHECK(std::is_sorted_until(pol, a, a + N) == a + 28);  // 0 37 74 ... 962 999 | 36
  std::sort(pol, b, b + N, std::greater<>());
  CHECK(b[0] == N - 1);
  std::copy(a, a + N, b);
  std::partial_sort(pol, b, b + 5, b + N);
  for (int i = 0; i < 5; ++i) CHECK(b[i] == i);
  std::copy(a, a + N, b);
  std::nth_element(pol, b, b + 500, b + N);
  CHECK(b[500] == 500);
  CHECK(std::partial_sort_copy(pol, a, a + N, c, c + 3) == c + 3 && c[0] == 0 && c[2] == 2);
  CHECK(*std::min_element(pol, a, a + N) == 0 && *std::max_element(pol, a, a + N) == N - 1);
  auto mm = std::minmax_element(pol, a, a + N);
  CHECK(*mm.first == 0 && *mm.second == N - 1);

  // stability
  static KV kv[N], kv2[N];
  for (int i = 0; i < N; ++i) kv[i] = KV{a[i] % 7, i};
  std::stable_sort(pol, kv, kv + N);
  for (int i = 1; i < N; ++i) CHECK(kv[i - 1].key < kv[i].key || (kv[i - 1].key == kv[i].key && kv[i - 1].id < kv[i].id));
  for (int i = 0; i < N; ++i) kv[i] = KV{a[i] % 7, i};
  KV* pp = std::stable_partition(pol, kv, kv + N, [](const KV& x) { return x.key < 3; });
  for (KV* q = kv; q != kv + N; ++q) CHECK((q < pp) == (q->key < 3));
  for (KV* q = kv + 1; q < pp; ++q) CHECK(q[-1].id < q->id);
  for (KV* q = pp + 1; q < kv + N; ++q) CHECK(q[-1].id < q->id);
  CHECK(std::is_partitioned(pol, kv, kv + N, [](const KV& x) { return x.key < 3; }));
  // merge: equivalent elements of the first range come first
  for (int i = 0; i < N; ++i) {
    kv[i] = KV{i / 4, i};
    kv2[i] = KV{i / 4, N + i};
  }
  static KV kvo[2 * N];
  CHECK(std::merge(pol, kv, kv + N, kv2, kv2 + N, kvo) == kvo + 2 * N);
  for (int i = 0; i < 2 * N; i += 8) {
    for (int j = 0; j < 4; ++j) CHECK(kvo[i + j].id < N && kvo[i + 4 + j].id >= N);
  }
  for (int i = 0; i < N; ++i) kvo[i] = kv[i], kvo[N + i] = kv2[i];
  std::inplace_merge(pol, kvo, kvo + N, kvo + 2 * N);
  for (int i = 0; i < 2 * N; i += 8) {
    for (int j = 0; j < 4; ++j) CHECK(kvo[i + j].id < N && kvo[i + 4 + j].id >= N);
  }

  // set operations on sorted inputs
  for (int i = 0; i < N; ++i) b[i] = 2 * i, c[i] = 3 * i;
  int out[2 * N];
  int* e = std::set_union(pol, b, b + N, c, c + N, out);
  CHECK(e - out == N + N - ((2 * (N - 1)) / 6 + 1));  // |b| + |c| - |multiples of 6 in b|
  CHECK(std::is_sorted(out, e) && std::adjacent_find(out, e) == e);
  e = std::set_intersection(pol, b, b + N, c, c + N, out);
  for (int* q = out; q != e; ++q) CHECK(*q % 6 == 0);
  CHECK(e - out == (2 * (N - 1)) / 6 + 1);
  CHECK(std::includes(pol, b, b + N, out, e));
  CHECK(std::search(pol, b, b + N, b + 10, b + 13) == b + 10);
  CHECK(std::search_n(pol, b, b + N, 1, 20) == b + 10);
  CHECK(std::find_end(pol, b, b + N, b + 10, b + 13) == b + 10);
  CHECK(std::find_first_of(pol, b, b + N, c + 1, c + 3) == b + 3);  // 6
  CHECK(std::lexicographical_compare(pol, b, b + N, c, c + N));     // 0 2 < 0 3

  // uninitialized_copy with a policy (<memory>, [uninitialized.copy])
  alignas(int) static unsigned char raw[N * sizeof(int)];
  int* r = reinterpret_cast<int*>(raw);
  CHECK(std::uninitialized_copy(pol, a, a + N, r) == r + N);
  CHECK(std::equal(a, a + N, r));
  std::destroy(pol, r, r + N);
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
  return 0;
}
