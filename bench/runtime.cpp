// Utilities and the language-support runtime: shared_ptr, <random>, function, exceptions,
// dynamic_cast, mutex, atomic.
#include "bench.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>

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
  }
}
