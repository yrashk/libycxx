// Containers: vector, deque, list, map, unordered_map.
#include "bench.hpp"

#include <deque>
#include <flat_map>
#include <flat_set>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

std::vector<int> random_ints(std::size_t n, unsigned mod = 0) {
  bench::rng r;
  std::vector<int> v(n);
  for (auto& x : v) x = static_cast<int>(mod ? r() % mod : r() >> 33);
  return v;
}

std::vector<std::string> random_keys(std::size_t n) {
  bench::rng r;
  std::vector<std::string> v;
  v.reserve(n);
  for (std::size_t i = 0; i < n; ++i) v.push_back("key_" + std::to_string(r() % 100000000));
  return v;
}

}  // namespace

int main(int argc, char** argv) {
  bench::init(argc, argv);
  constexpr int N = 100000;

  bench::run("vector.push_back int", N, [] {
    std::vector<int> v;
    for (int i = 0; i < N; ++i) v.push_back(i);
    bench::sink(v.data());
  });
  bench::run("vector.push_back int (reserved)", N, [] {
    std::vector<int> v;
    v.reserve(N);
    for (int i = 0; i < N; ++i) v.push_back(i);
    bench::sink(v.data());
  });
  bench::run("vector.push_back string", N, [] {
    std::vector<std::string> v;
    for (int i = 0; i < N; ++i) v.push_back("short");
    bench::sink(v.data());
  });
  bench::run("vector.insert front int (n=2000)", 2000, [] {
    std::vector<int> v;
    for (int i = 0; i < 2000; ++i) v.insert(v.begin(), i);
    bench::sink(v.data());
  });
  bench::run("vector.insert range int", 1000, [] {
    static const std::vector<int> src(1000, 7);
    std::vector<int> v(1000, 1);
    v.insert(v.begin() + 500, src.begin(), src.end());
    bench::sink(v.data());
  });
  bench::run("vector.emplace_back 1e6 int (reserve 1000)", 1000000, [] {
    std::vector<int> v;
    v.reserve(1000);
    for (int i = 0; i < 1000000; ++i) v.emplace_back(i);
    bench::sink(v.data());
  });
  {
    std::vector<int> src = random_ints(1000000);
    bench::run("vector.copy 1e6 int", 1000000, [&] {
      std::vector<int> c(src);
      bench::sink(c.data());
    });
    std::vector<std::string> ssrc = random_keys(100000);
    bench::run("vector.copy 1e5 string", 100000, [&] {
      std::vector<std::string> c(ssrc);
      bench::sink(c.data());
    });
  }

  bench::run("deque.push_back int", N, [] {
    std::deque<int> d;
    for (int i = 0; i < N; ++i) d.push_back(i);
    bench::sink(d.size());
  });
  bench::run("deque.push_front int", N, [] {
    std::deque<int> d;
    for (int i = 0; i < N; ++i) d.push_front(i);
    bench::sink(d.size());
  });
  bench::run("deque.push both+pop", N, [] {
    std::deque<int> d;
    for (int i = 0; i < N; ++i) {
      if (i & 1) d.push_back(i);
      else d.push_front(i);
    }
    long s = 0;
    while (!d.empty()) {
      s += d.front();
      d.pop_front();
    }
    bench::sink(s);
  });
  bench::run("deque.iterate sum", N, [] {
    static const std::deque<int> d = [] {
      std::deque<int> x;
      for (int i = 0; i < N; ++i) x.push_back(i);
      return x;
    }();
    long s = 0;
    for (int x : d) s += x;
    bench::sink(s);
  });

  bench::run("queue<int> push+pop (BFS-like)", N, [] {
    std::queue<int> q;
    long s = 0;
    for (int i = 0; i < N; ++i) {
      q.push(i);
      q.push(i + 1);
      s += q.front();
      q.pop();
    }
    bench::sink(s);
  });
  bench::run("priority_queue<int> push+pop", N, [] {
    std::priority_queue<int> q;
    bench::rng r;
    long s = 0;
    for (int i = 0; i < N; ++i) {
      q.push(static_cast<int>(r() >> 40));
      if (i & 1) {
        s += q.top();
        q.pop();
      }
    }
    bench::sink(s);
  });

  {
    std::vector<int> src = random_ints(N);
    std::list<int> l;
    bench::run_prepared("list.sort 1e5 int", N, [&] { l.assign(src.begin(), src.end()); },
                        [&] { l.sort(); bench::sink(l.front()); });
  }
  bench::run("list.push_back+clear", N, [] {
    std::list<int> l;
    for (int i = 0; i < N; ++i) l.push_back(i);
    bench::sink(l.size());
  });

  {
    std::vector<int> keys = random_ints(N);
    bench::run("map<int>.insert", N, [&] {
      std::map<int, int> m;
      for (int k : keys) m.emplace(k, k);
      bench::sink(m.size());
    });
    std::map<int, int> m;
    for (int k : keys) m.emplace(k, k);
    bench::run("map<int>.find", N, [&] {
      long s = 0;
      for (int k : keys) s += m.find(k)->second;
      bench::sink(s);
    });
    bench::run("map<int>.iterate", static_cast<double>(m.size()), [&] {
      long s = 0;
      for (auto& [k, v] : m) s += v;
      bench::sink(s);
    });

    bench::run("unordered_map<int>.insert", N, [&] {
      std::unordered_map<int, int> u;
      for (int k : keys) u.emplace(k, k);
      bench::sink(u.size());
    });
    bench::run("unordered_map<int>.insert (reserved)", N, [&] {
      std::unordered_map<int, int> u;
      u.reserve(N);
      for (int k : keys) u.emplace(k, k);
      bench::sink(u.size());
    });
    std::unordered_map<int, int> u;
    for (int k : keys) u.emplace(k, k);
    bench::run("unordered_map<int>.find hit", N, [&] {
      long s = 0;
      for (int k : keys) s += u.find(k)->second;
      bench::sink(s);
    });
    bench::run("unordered_map<int>.find miss", N, [&] {
      long s = 0;
      for (int k : keys) s += u.count(k ^ 0x40000000);
      bench::sink(s);
    });
    bench::run("unordered_map<int>.insert+erase", N, [&] {
      std::unordered_map<int, int> x;
      for (int k : keys) x.emplace(k, k);
      for (int k : keys) x.erase(k);
      bench::sink(x.size());
    });
    bench::run("unordered_map<int>.operator[] small keys", N, [] {
      std::unordered_map<int, int> x;
      for (int i = 0; i < N; ++i) ++x[i & 1023];
      bench::sink(x.size());
    });
    bench::run("unordered_map<int>.iterate", static_cast<double>(u.size()), [&] {
      long s = 0;
      for (auto& [k, v] : u) s += v;
      bench::sink(s);
    });
    bench::run("unordered_set<int>.insert", N, [&] {
      std::unordered_set<int> x;
      for (int k : keys) x.insert(k);
      bench::sink(x.size());
    });
    std::unordered_set<int> us(keys.begin(), keys.end());
    bench::run("unordered_set<int>.contains hit", N, [&] {
      long s = 0;
      for (int k : keys) s += us.contains(k);
      bench::sink(s);
    });
    bench::run("set<int>.insert", N, [&] {
      std::set<int> x;
      for (int k : keys) x.insert(k);
      bench::sink(x.size());
    });
    std::flat_map<int, int> fm;
    for (int k : keys) fm.try_emplace(k, k);
    bench::run("flat_map<int>.find", N, [&] {
      long s = 0;
      for (int k : keys) s += fm.find(k)->second;
      bench::sink(s);
    });
    bench::run("flat_map<int>.insert sorted range", N, [&] {
      std::vector<std::pair<int, int>> v;
      v.reserve(N);
      for (int i = 0; i < N; ++i) v.emplace_back(i, i);
      std::flat_map<int, int> x(std::sorted_unique, v.begin(), v.end());
      bench::sink(x.size());
    });
    bench::run("flat_set<int>.insert (random, 1e4)", 10000, [&] {
      std::flat_set<int> x;
      for (int i = 0; i < 10000; ++i) x.insert(keys[i]);
      bench::sink(x.size());
    });
  }
  {
    std::vector<std::string> keys = random_keys(N);
    bench::run("map<string>.insert", N, [&] {
      std::map<std::string, int> m;
      for (auto& k : keys) m.emplace(k, 1);
      bench::sink(m.size());
    });
    std::map<std::string, int> m;
    for (auto& k : keys) m.emplace(k, 1);
    bench::run("map<string>.find", N, [&] {
      long s = 0;
      for (auto& k : keys) s += m.find(k)->second;
      bench::sink(s);
    });
    bench::run("unordered_map<string>.insert", N, [&] {
      std::unordered_map<std::string, int> u;
      for (auto& k : keys) u.emplace(k, 1);
      bench::sink(u.size());
    });
    std::unordered_map<std::string, int> u;
    for (auto& k : keys) u.emplace(k, 1);
    bench::run("unordered_map<string>.find", N, [&] {
      long s = 0;
      for (auto& k : keys) s += u.find(k)->second;
      bench::sink(s);
    });
    bench::run("unordered_map<string>.find miss", N, [&] {
      long s = 0;
      std::string probe;
      for (auto& k : keys) {
        probe = k;
        probe[0] = 'K';
        s += u.find(probe) == u.end();
      }
      bench::sink(s);
    });
    bench::run("unordered_map<string>.insert+erase", N, [&] {
      std::unordered_map<std::string, int> x;
      for (auto& k : keys) x.emplace(k, 1);
      for (auto& k : keys) x.erase(k);
      bench::sink(x.size());
    });
  }
}
