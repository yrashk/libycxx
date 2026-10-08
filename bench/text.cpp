// Text conversion: <charconv>, <format>, iostreams, <regex>.
#include "bench.hpp"

#include <charconv>
#include <chrono>
#include <cstdio>
#include <format>
#include <fstream>
#include <iterator>
#include <print>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  bench::init(argc, argv);
  constexpr int N = 100000;

  std::vector<int> ints(N);
  std::vector<double> dbls(N);
  {
    bench::rng r;
    for (auto& x : ints) x = static_cast<int>(r() >> 32);
    for (auto& d : dbls) d = static_cast<double>(r() >> 11) / 9007199254740992.0 * 1e6;
  }
  char buf[64];

  bench::run("to_chars int", N, [&] {
    for (int x : ints) bench::sink(std::to_chars(buf, buf + 64, x).ptr);
  });
  bench::run("to_chars double (shortest)", N, [&] {
    for (double d : dbls) bench::sink(std::to_chars(buf, buf + 64, d).ptr);
  });
  bench::run("to_chars double fixed .6", N, [&] {
    for (double d : dbls) bench::sink(std::to_chars(buf, buf + 64, d, std::chars_format::fixed, 6).ptr);
  });
  std::vector<std::string> itexts, dtexts;
  for (int x : ints) itexts.push_back(std::to_string(x));
  for (double d : dbls) {
    auto r = std::to_chars(buf, buf + 64, d);
    dtexts.emplace_back(buf, r.ptr);
  }
  bench::run("from_chars int", N, [&] {
    int v;
    for (auto& s : itexts) {
      std::from_chars(s.data(), s.data() + s.size(), v);
      bench::sink(v);
    }
  });
  bench::run("from_chars double", N, [&] {
    double v;
    for (auto& s : dtexts) {
      std::from_chars(s.data(), s.data() + s.size(), v);
      bench::sink(v);
    }
  });
  {
    std::vector<float> flts(dbls.begin(), dbls.end());
    std::vector<std::string> ftexts;
    for (float f : flts) {
      auto r = std::to_chars(buf, buf + 64, f);
      ftexts.emplace_back(buf, r.ptr);
    }
    bench::run("to_chars float (shortest)", N, [&] {
      for (float f : flts) bench::sink(std::to_chars(buf, buf + 64, f).ptr);
    });
    bench::run("from_chars float", N, [&] {
      float v;
      for (auto& s : ftexts) {
        std::from_chars(s.data(), s.data() + s.size(), v);
        bench::sink(v);
      }
    });
    bench::run("to_chars double scientific .3", N, [&] {
      for (double d : dbls) bench::sink(std::to_chars(buf, buf + 64, d, std::chars_format::scientific, 3).ptr);
    });
  }

  bench::run("format {} int", N, [&] {
    for (int x : ints) bench::sink(std::format("{}", x));
  });
  bench::run("format {} double", N, [&] {
    for (double d : dbls) bench::sink(std::format("{}", d));
  });
  bench::run("format {:.3f} double", N, [&] {
    for (double d : dbls) bench::sink(std::format("{:.3f}", d));
  });
  bench::run("format mixed (str, int, pad)", N, [&] {
    for (int x : ints) bench::sink(std::format("name={} value={:>12} hex={:#x}", "abc", x, x & 0xffff));
  });
  bench::run("format_to buffer int", N, [&] {
    for (int x : ints) bench::sink(std::format_to(buf, "{}", x));
  });
  bench::run("to_string int", N, [&] {
    for (int x : ints) bench::sink(std::to_string(x));
  });
  bench::run("to_string double", N, [&] {
    for (double d : dbls) bench::sink(std::to_string(d));
  });
  bench::run("format_to back_inserter (3 args)", N, [&] {
    std::string out;
    for (int x : ints) {
      out.clear();
      std::format_to(std::back_inserter(out), "{}:{}:{}", x, x >> 3, "ab");
      bench::sink(out);
    }
  });
  {
    std::FILE* devnull = std::fopen("/dev/null", "w");
    bench::run("print to FILE (int, string)", N, [&] {
      for (int x : ints) std::print(devnull, "{} {}\n", x, "abc");
    });
    std::fclose(devnull);
  }
  {
    std::chrono::sys_seconds t{std::chrono::seconds{1759800000}};
    bench::run("format chrono {:%F %T}", N / 10, [&] {
      for (int i = 0; i < N / 10; ++i) bench::sink(std::format("{:%F %T}", t + std::chrono::seconds{i}));
    });
  }

  bench::run("ostringstream << int", N, [&] {
    std::ostringstream os;
    for (int x : ints) os << x << ' ';
    bench::sink(os);
  });
  bench::run("ostringstream << double", N, [&] {
    std::ostringstream os;
    for (double d : dbls) os << d << ' ';
    bench::sink(os);
  });
  bench::run("ostringstream << string", N, [&] {
    std::ostringstream os;
    for (int i = 0; i < N; ++i) os << "hello";
    bench::sink(os);
  });
  bench::run("ostringstream construct+str", N / 10, [&] {
    for (int i = 0; i < N / 10; ++i) {
      std::ostringstream os;
      os << i;
      bench::sink(os.str());
    }
  });
  {
    std::ostringstream os;
    for (int x : ints) os << x << ' ';
    std::string itext = os.str();
    std::ostringstream od;
    for (double d : dbls) od << d << ' ';
    std::string dtext = od.str();
    bench::run("istringstream >> int", N, [&] {
      std::istringstream is(itext);
      int v;
      long s = 0;
      while (is >> v) s += v;
      bench::sink(s);
    });
    bench::run("istringstream >> double", N, [&] {
      std::istringstream is(dtext);
      double v, s = 0;
      while (is >> v) s += v;
      bench::sink(s);
    });
    std::string lines;
    for (int i = 0; i < N; ++i) lines += "line number " + std::to_string(i) + " of some text\n";
    bench::run("getline (istringstream)", N, [&] {
      std::istringstream is(lines);
      std::string l;
      std::size_t s = 0;
      while (std::getline(is, l)) s += l.size();
      bench::sink(s);
    });
    const char* path = "/tmp/ycxx-bench-text.txt";
    bench::run("ofstream << line (file write)", N, [&] {
      std::ofstream os(path);
      for (int i = 0; i < N; ++i) os << "line " << i << '\n';
    });
    {
      std::ofstream os(path);
      os << lines;
    }
    bench::run("ifstream getline (file read)", N, [&] {
      std::ifstream is(path);
      std::string l;
      std::size_t s = 0;
      while (std::getline(is, l)) s += l.size();
      bench::sink(s);
    });
    std::remove(path);
  }

  {
    std::string text;
    bench::rng r;
    for (int i = 0; i < 20000; ++i) {
      text += static_cast<char>('a' + r() % 26);
      if (i % 7 == 0) text += ' ';
    }
    text += " user@example.com 2026-10-04 ";
    std::regex literal("example");
    std::regex email(R"([a-z]+@[a-z]+\.com)");
    std::regex date(R"((\d{4})-(\d\d)-(\d\d))");
    std::regex alt("(foo|bar|baz|qux)+z");
    double len = static_cast<double>(text.size());
    std::smatch m;
    bench::run("regex_search literal (per char)", len, [&] { bench::sink(std::regex_search(text, m, literal)); });
    bench::run("regex_search email (per char)", len, [&] { bench::sink(std::regex_search(text, m, email)); });
    bench::run("regex_search date (per char)", len, [&] { bench::sink(std::regex_search(text, m, date)); });
    bench::run("regex_search alternation (per char)", len, [&] { bench::sink(std::regex_search(text, m, alt)); });
    bench::run("regex construct", 1, [&] { std::regex x(R"((\w+)@(\w+)\.(com|org|net))"); bench::sink(x); });
    bench::run("regex_match short", 1, [&] {
      static const std::regex x(R"(\d+-\d+)");
      bench::sink(std::regex_match(bench::opaque("12345-6789"), x));
    });
  }
}
