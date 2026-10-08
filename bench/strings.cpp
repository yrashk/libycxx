// basic_string and string_view hot paths.
#include "bench.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

int main(int argc, char** argv) {
  bench::init(argc, argv);
  constexpr int N = 100000;

  bench::run("string.SSO construct", N, [] {
    for (int i = 0; i < N; ++i) {
      std::string s(bench::opaque("hello, world"));
      bench::sink(s);
    }
  });
  bench::run("string.heap construct (40 chars)", N, [] {
    for (int i = 0; i < N; ++i) {
      std::string s(bench::opaque("a fairly long string of forty characters"));
      bench::sink(s);
    }
  });
  bench::run("string.copy SSO", N, [] {
    static const std::string src = "short";
    for (int i = 0; i < N; ++i) {
      std::string s(src);
      bench::sink(s);
    }
  });
  bench::run("string.append char", N, [] {
    std::string s;
    for (int i = 0; i < N; ++i) s.push_back(static_cast<char>('a' + (i & 15)));
    bench::sink(s);
  });
  bench::run("string.append 8-char literal", N, [] {
    std::string s;
    for (int i = 0; i < N; ++i) s.append("abcdefgh");
    bench::sink(s);
  });
  bench::run("string.operator+ small", N, [] {
    static const std::string a = "foo", b = "bar";
    for (int i = 0; i < N; ++i) {
      std::string s = a + "/" + b;
      bench::sink(s);
    }
  });

  std::string hay;
  {
    bench::rng r;
    for (int i = 0; i < 1000000; ++i) hay.push_back(static_cast<char>('a' + r() % 20));
  }
  bench::run("string.find char (1 MB, miss)", 1000000, [&] {
    bench::sink(hay.find(bench::opaque('z')));
  });
  bench::run("string.find substr (1 MB, miss)", 1000000, [&] {
    bench::sink(hay.find(bench::opaque("needle")));
  });
  bench::run("string.find_first_of (1 MB, miss)", 1000000, [&] {
    bench::sink(hay.find_first_of(bench::opaque("xyz")));
  });
  bench::run("string.rfind char (1 MB, miss)", 1000000, [&] {
    bench::sink(hay.rfind(bench::opaque('z')));
  });
  {
    std::string a = hay, b = hay;
    b.back() = 'Z';
    bench::run("string.compare (1 MB, differ at end)", 1000000, [&] {
      bench::sink(a.compare(b));
    });
    bench::run("string.operator== (1 MB, differ at end)", 1000000, [&] {
      bench::sink(a == b);
    });
  }
  bench::run("string.compare short", N, [] {
    static const std::string a = "alpha-beta", b = "alpha-gamma";
    int s = 0;
    for (int i = 0; i < N; ++i) s += bench::opaque(a).compare(b) < 0;
    bench::sink(s);
  });
  bench::run("string_view.find substr (1 MB, miss)", 1000000, [&] {
    std::string_view v = hay;
    bench::sink(v.find(bench::opaque("qqqqq")));
  });
  {
    std::vector<std::string> keys;
    bench::rng r;
    for (int i = 0; i < 1000; ++i) keys.push_back("key_" + std::to_string(r() % 100000000));
    bench::run("hash<string> (12 chars)", 1000, [&] {
      std::size_t h = 0;
      for (auto& k : keys) h += std::hash<std::string>{}(k);
      bench::sink(h);
    });
    std::string long_key(200, 'x');
    bench::run("hash<string> (200 chars)", 1, [&] { bench::sink(std::hash<std::string>{}(bench::opaque(long_key))); });
    bench::run("string.operator== short (equal)", 1000, [&] {
      int s = 0;
      for (auto& k : keys) s += k == keys[0];
      bench::sink(s);
    });
    bench::run("string.append string (to 1e5)", N, [&] {
      std::string s;
      for (int i = 0; i < N / 10; ++i) s += keys[static_cast<std::size_t>(i) % 1000].substr(0, 10);
      bench::sink(s);
    });
  }
  bench::run("string.find char (64 B)", 64, [&] {
    std::string_view v(hay.data(), 64);
    bench::sink(v.find(bench::opaque('z')));
  });
}
