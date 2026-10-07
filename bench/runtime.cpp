// Utilities and the language-support runtime: shared_ptr, <random>, function, exceptions,
// dynamic_cast, mutex, atomic.
#include "bench.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

struct base {
  virtual ~base() = default;
  int b = 0;
};
struct left : virtual base {
  int l = 1;
};
struct right : virtual base {
  int r = 2;
};
struct diamond : left, right {
  int d = 3;
};
struct a1 {
  virtual ~a1() = default;
};
struct a2 : a1 {};
struct a3 : a2 {};
struct a4 : a3 {};
struct other {
  virtual ~other() = default;
};
struct mixed : a4, other {};

struct my_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

[[gnu::noinline]] void thrower(int x) {
  if (bench::opaque(x) >= 0) throw x;
}
[[gnu::noinline]] void thrower_derived() { throw my_error("boom"); }
[[gnu::noinline]] void deep(int n) {
  if (n == 0) throw 42;
  deep(bench::opaque(n - 1));
  bench::clobber();
}

}  // namespace

int main(int argc, char** argv) {
  bench::init(argc, argv);
  constexpr int N = 100000;

  {
    auto p = std::make_shared<int>(1);
    bench::run("shared_ptr copy+destroy", N, [&] {
      for (int i = 0; i < N; ++i) {
        std::shared_ptr<int> q(p);
        bench::sink(q);
      }
    });
    bench::run("make_shared<int>", N, [] {
      for (int i = 0; i < N; ++i) bench::sink(std::make_shared<int>(i));
    });
    std::weak_ptr<int> w = p;
    bench::run("weak_ptr lock", N, [&] {
      for (int i = 0; i < N; ++i) bench::sink(w.lock());
    });
    bench::run("unique_ptr make+destroy", N, [] {
      for (int i = 0; i < N; ++i) bench::sink(std::make_unique<int>(i));
    });
    bench::run("make_shared<string>", N, [] {
      for (int i = 0; i < N; ++i) bench::sink(std::make_shared<std::string>("abc"));
    });
    bench::run("new/delete 16..4096 bytes", N, [] {
      for (int i = 0; i < N; ++i) {
        char* p = new char[16u << (i & 8)];
        bench::sink(p);
        delete[] p;
      }
    });
    bench::run("vector<int>(1000) construct+destroy", N / 10, [] {
      for (int i = 0; i < N / 10; ++i) {
        std::vector<int> v(1000);
        bench::sink(v.data());
      }
    });
  }

  {
    std::mt19937 g(5489u);
    bench::run("mt19937 raw", N, [&] {
      unsigned s = 0;
      for (int i = 0; i < N; ++i) s += g();
      bench::sink(s);
    });
    std::uniform_int_distribution<int> d(0, 999);
    bench::run("mt19937 + uniform_int(0,999)", N, [&] {
      int s = 0;
      for (int i = 0; i < N; ++i) s += d(g);
      bench::sink(s);
    });
    std::uniform_real_distribution<double> u(0.0, 1.0);
    bench::run("mt19937 + uniform_real", N, [&] {
      double s = 0;
      for (int i = 0; i < N; ++i) s += u(g);
      bench::sink(s);
    });
    std::mt19937_64 g64(1);
    std::normal_distribution<double> nd;
    bench::run("mt19937_64 + normal", N, [&] {
      double s = 0;
      for (int i = 0; i < N; ++i) s += nd(g64);
      bench::sink(s);
    });
  }

  {
    int k = 3;
    std::function<int(int)> f = [k](int x) { return x * k; };
    bench::run("function call", N, [&] {
      int s = 0;
      for (int i = 0; i < N; ++i) s += (*bench::opaque(&f))(i);
      bench::sink(s);
    });
    bench::run("function construct small", N, [&] {
      for (int i = 0; i < N; ++i) {
        std::function<int(int)> g = [i](int x) { return x + i; };
        bench::sink(g);
      }
    });
#ifdef __cpp_lib_move_only_function // not in libc++ 23
    std::move_only_function<int(int)> mf = [k](int x) { return x * k; };
    bench::run("move_only_function call", N, [&] {
      int s = 0;
      for (int i = 0; i < N; ++i) s += (*bench::opaque(&mf))(i);
      bench::sink(s);
    });
#endif
    bench::run("function construct large (5 captures)", N, [&] {
      for (int i = 0; i < N; ++i) {
        long a = i, b = i + 1, c = i + 2, d = i + 3, e = i + 4;
        std::function<long(int)> g = [a, b, c, d, e](int x) { return x + a + b + c + d + e; };
        bench::sink(g);
      }
    });
  }
  {
    std::mt19937_64 g(7);
    std::uniform_int_distribution<long long> d(0, 1LL << 40);
    bench::run("mt19937_64 + uniform_int<long long>", N, [&] {
      long long s = 0;
      for (int i = 0; i < N; ++i) s += d(g);
      bench::sink(s);
    });
    bench::run("hash<int> + hash<double>", N, [&] {
      std::size_t s = 0;
      for (int i = 0; i < N; ++i) s += std::hash<int>{}(i) + std::hash<double>{}(i * 0.5);
      bench::sink(s);
    });
  }

  bench::run("throw/catch int", 1, [] {
    try {
      thrower(1);
    } catch (int x) {
      bench::sink(x);
    }
  });
  bench::run("throw/catch derived by base&", 1, [] {
    try {
      thrower_derived();
    } catch (const std::exception& e) {
      bench::sink(e);
    }
  });
  bench::run("throw/catch through 20 frames", 1, [] {
    try {
      deep(20);
    } catch (int x) {
      bench::sink(x);
    }
  });

  {
    diamond dd;
    base* pb = &dd;
    a4 x4;
    a1* p1 = &x4;
    mixed mx;
    other* po = &mx;
    bench::run("dynamic_cast to most derived", N, [&] {
      for (int i = 0; i < N; ++i) bench::sink(dynamic_cast<a4*>(bench::opaque(p1)));
    });
    bench::run("dynamic_cast to intermediate", N, [&] {
      for (int i = 0; i < N; ++i) bench::sink(dynamic_cast<a2*>(bench::opaque(p1)));
    });
    bench::run("dynamic_cast virtual base -> left", N, [&] {
      for (int i = 0; i < N; ++i) bench::sink(dynamic_cast<left*>(bench::opaque(pb)));
    });
    bench::run("dynamic_cast cross cast", N, [&] {
      for (int i = 0; i < N; ++i) bench::sink(dynamic_cast<a2*>(bench::opaque(po)));
    });
    bench::run("dynamic_cast failure", N, [&] {
      for (int i = 0; i < N; ++i) bench::sink(dynamic_cast<other*>(bench::opaque(p1)));
    });
  }

  {
    std::mutex m;
    bench::run("mutex lock/unlock", N, [&] {
      for (int i = 0; i < N; ++i) {
        std::lock_guard<std::mutex> g(m);
        bench::clobber();
      }
    });
    std::atomic<int> a{0};
    bench::run("atomic<int>.fetch_add seq_cst", N, [&] {
      for (int i = 0; i < N; ++i) a.fetch_add(1);
      bench::sink(a);
    });
    bench::run("atomic<int>.load", N, [&] {
      int s = 0;
      for (int i = 0; i < N; ++i) s += a.load();
      bench::sink(s);
    });
    std::once_flag of;
    bench::run("call_once (done)", N, [&] {
      for (int i = 0; i < N; ++i) std::call_once(of, [] {});
    });
    std::shared_mutex sm;
    bench::run("shared_mutex lock_shared/unlock", N, [&] {
      for (int i = 0; i < N; ++i) {
        std::shared_lock<std::shared_mutex> l(sm);
        bench::clobber();
      }
    });
    std::condition_variable cv;
    bench::run("condition_variable notify_one (no waiter)", N, [&] {
      for (int i = 0; i < N; ++i) cv.notify_one();
    });
    std::atomic<int> w{0};
    bench::run("atomic notify_one (no waiter)", N, [&] {
      for (int i = 0; i < N; ++i) w.notify_one();
    });
    bench::run("mutex ping-pong 2 threads (per handoff)", 20000, [&] {
      std::mutex mm;
      std::condition_variable c2;
      int turn = 0;
      std::thread t([&] {
        for (int i = 0; i < 10000; ++i) {
          std::unique_lock<std::mutex> l(mm);
          c2.wait(l, [&] { return turn == 1; });
          turn = 0;
          c2.notify_one();
        }
      });
      for (int i = 0; i < 10000; ++i) {
        std::unique_lock<std::mutex> l(mm);
        turn = 1;
        c2.notify_one();
        c2.wait(l, [&] { return turn == 0; });
      }
      t.join();
    });
    bench::run("atomic wait/notify ping-pong (per handoff)", 20000, [&] {
      std::atomic<int> f{0};
      std::thread t([&] {
        for (int i = 0; i < 10000; ++i) {
          f.wait(0);
          f.store(0);
          f.notify_one();
        }
      });
      for (int i = 0; i < 10000; ++i) {
        f.store(1);
        f.notify_one();
        f.wait(1);
      }
      t.join();
    });
    bench::run("atomic<int> fetch_add 4 threads (contended)", 400000, [&] {
      std::atomic<int> c{0};
      std::vector<std::thread> ts;
      for (int k = 0; k < 4; ++k)
        ts.emplace_back([&] {
          for (int i = 0; i < 100000; ++i) c.fetch_add(1, std::memory_order_relaxed);
        });
      for (auto& t : ts) t.join();
      bench::sink(c);
    });
  }
  bench::run("throw/catch runtime_error with what()", 1, [] {
    try {
      throw std::runtime_error("a message long enough to need the heap");
    } catch (const std::exception& e) {
      bench::sink(e.what());
    }
  });
}
