// <algorithm>: sorting and the simple linear algorithms (are they vectorized?).
#include "bench.hpp"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  bench::init(argc, argv);
  constexpr int M = 1000000;

  std::vector<int> ints(M);
  {
    bench::rng r;
    for (auto& x : ints) x = static_cast<int>(r() >> 33);
  }
  std::vector<std::string> strs(M / 5);
  {
    bench::rng r;
    for (auto& s : strs) s = "item-" + std::to_string(r() % 1000000000);
  }

  std::vector<int> w;
  auto reset = [&] { w = ints; };
  bench::run_prepared("sort 1e6 int", M, reset, [&] { std::sort(w.begin(), w.end()); bench::sink(w[0]); });
  bench::run_prepared("sort 1e6 int (sorted)", M, [&] { w = ints; std::sort(w.begin(), w.end()); },
                      [&] { std::sort(w.begin(), w.end()); bench::sink(w[0]); });
  bench::run_prepared("sort 1e6 int (reversed)", M,
                      [&] { w = ints; std::sort(w.begin(), w.end(), std::greater<>()); },
                      [&] { std::sort(w.begin(), w.end()); bench::sink(w[0]); });
  bench::run_prepared("stable_sort 1e6 int", M, reset, [&] { std::stable_sort(w.begin(), w.end()); bench::sink(w[0]); });
  bench::run_prepared("nth_element 1e6 int", M, reset, [&] {
    std::nth_element(w.begin(), w.begin() + M / 2, w.end());
    bench::sink(w[M / 2]);
  });
  bench::run_prepared("partial_sort 1e6 int (k=100)", M, reset, [&] {
    std::partial_sort(w.begin(), w.begin() + 100, w.end());
    bench::sink(w[0]);
  });
  bench::run_prepared("make_heap+sort_heap 1e5 int", M / 10, [&] { w.assign(ints.begin(), ints.begin() + M / 10); },
                      [&] { std::make_heap(w.begin(), w.end()); std::sort_heap(w.begin(), w.end()); bench::sink(w[0]); });

  std::vector<std::string> ws;
  auto sreset = [&] { ws = strs; };
  bench::run_prepared("sort 2e5 string", M / 5, sreset, [&] { std::sort(ws.begin(), ws.end()); bench::sink(ws[0]); });
  bench::run_prepared("stable_sort 2e5 string", M / 5, sreset, [&] { std::stable_sort(ws.begin(), ws.end()); bench::sink(ws[0]); });
  bench::run_prepared("nth_element 2e5 string", M / 5, sreset, [&] {
    std::nth_element(ws.begin(), ws.begin() + M / 10, ws.end());
    bench::sink(ws[M / 10]);
  });

  std::vector<int> small(M);
  std::iota(small.begin(), small.end(), 0);
  std::vector<int> dst(M);
  std::vector<unsigned char> bytes(M, 1);
  bench::run("find int (miss)", M, [&] { bench::sink(std::find(small.begin(), small.end(), bench::opaque(-1))); });
  bench::run("find byte (miss)", M, [&] { bench::sink(std::find(bytes.begin(), bytes.end(), bench::opaque<unsigned char>(0))); });
  bench::run("count int", M, [&] { bench::sink(std::count(small.begin(), small.end(), bench::opaque(5))); });
  bench::run("count_if int", M, [&] { bench::sink(std::count_if(small.begin(), small.end(), [](int x) { return x & 1; })); });
  bench::run("copy int", M, [&] { std::copy(small.begin(), small.end(), dst.begin()); bench::sink(dst[5]); });
  bench::run("copy_backward int", M, [&] { std::copy_backward(small.begin(), small.end(), dst.end()); bench::sink(dst[5]); });
  bench::run("move strings 1e5", M / 10, [&] {
    std::vector<std::string> a(strs.begin(), strs.begin() + M / 10), b(M / 10);
    std::move(a.begin(), a.end(), b.begin());
    bench::sink(b[0]);
  });
  bench::run("fill int", M, [&] { std::fill(dst.begin(), dst.end(), bench::opaque(3)); bench::sink(dst[5]); });
  bench::run("fill byte", M, [&] { std::fill(bytes.begin(), bytes.end(), bench::opaque<unsigned char>(1)); bench::sink(bytes[5]); });
  std::vector<int> small2 = small;
  bench::run("equal int", M, [&] { bench::sink(std::equal(small.begin(), small.end(), small2.begin())); });
  bench::run("accumulate int", M, [&] { bench::sink(std::accumulate(small.begin(), small.end(), 0L)); });
  bench::run("reverse int", M, [&] { std::reverse(dst.begin(), dst.end()); bench::sink(dst[5]); });
  bench::run("min_element int", M, [&] { bench::sink(std::min_element(ints.begin(), ints.end())); });
  bench::run("lower_bound int (1e6 lookups)", M, [&] {
    long s = 0;
    for (int i = 0; i < M; ++i) s += std::lower_bound(small.begin(), small.end(), static_cast<int>((i * 7919LL) % M)) - small.begin();
    bench::sink(s);
  });
  std::vector<int> u;
  bench::run_prepared("unique int", M, [&] {
    u.resize(M);
    for (int i = 0; i < M; ++i) u[i] = i / 3;
  }, [&] { bench::sink(std::unique(u.begin(), u.end())); });
}
