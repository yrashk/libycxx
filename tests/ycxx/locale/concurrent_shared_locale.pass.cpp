// [res.on.data.races]/2-3: library functions access shared objects only through their arguments
// and modify them only through non-const arguments; /7 internal shared objects are protected.
// [locale]/1: a locale is an immutable set of facets ([locale.cons]: copies share the facets,
// reference counted, [locale.facet]/2). Several threads use one shared const locale (classic
// with a program-defined numpunct and a program-defined facet) at once: copying it, use_facet /
// has_facet ([locale.global.templates]), name(), ==, combine ([locale.members]), operator()
// ([locale.operators], collate::compare), the convenience interfaces ([classification],
// [conversions.character]), the facets' const members (ctype, numpunct, collate transform and
// hash, codecvt in/out with per-thread state), and their own string streams imbued with it
// for num_put/num_get, money_put and time_put, plus std::format with the locale ("{:L}",
// [format.string.std]/17). The results are compared with those computed before the threads
// started. Meant to be run under TSan.
// FLAGS: -pthread
#include <cstddef>
#include <cwchar>
#include <format>
#include <iomanip>
#include <latch>
#include <locale>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "check.hpp"

constexpr int K = 4;

struct Grouping : std::numpunct<char> {
  char do_thousands_sep() const override { return '\''; }
  std::string do_grouping() const override { return "\3"; }
  char do_decimal_point() const override { return ','; }
};

struct Tag : std::locale::facet {
  static std::locale::id id;
  int value;
  explicit Tag(int v) : value(v) {}
};
std::locale::id Tag::id;

struct Results {
  std::string num, money, time, fmt, upper, transformed;
  long hash = 0;
  double parsed = 0;
  std::wstring widened;
  bool operator==(const Results&) const = default;
};

static Results compute(const std::locale& shared, int r) {
  Results res;
  std::ostringstream o;
  o.imbue(shared);
  o << 1234567 + r << ' ' << std::fixed << std::setprecision(2) << 9876543.21 << ' ' << true;
  res.num = o.str();
  std::istringstream in("7'654'321,5");
  in.imbue(shared);
  in >> res.parsed;
  std::ostringstream mo;
  mo.imbue(shared);
  mo << std::showbase << std::put_money(static_cast<long double>(123456 + r));
  res.money = mo.str();
  std::tm t{};
  t.tm_year = 126;
  t.tm_mon = 9;
  t.tm_mday = 5 + r % 3;
  t.tm_hour = 13;
  std::ostringstream to;
  to.imbue(shared);
  to << std::put_time(&t, "%Y-%m-%d %H:%M %A %b");
  res.time = to.str();
  res.fmt = std::format(shared, "{:L} {:L} {:L}", 1234567 + r, 12345.5, true);
  std::string s = "Hello, World " + std::to_string(r);
  for (char& c : s) c = std::toupper(c, shared);
  res.upper = s;
  const auto& col = std::use_facet<std::collate<char>>(shared);
  std::string a = "apple" + std::to_string(r);
  res.transformed = col.transform(a.data(), a.data() + a.size());
  res.hash = col.hash(a.data(), a.data() + a.size());
  const auto& ct = std::use_facet<std::ctype<wchar_t>>(shared);
  std::string narrow = "wide " + std::to_string(r);
  std::wstring w(narrow.size(), L' ');
  ct.widen(narrow.data(), narrow.data() + narrow.size(), w.data());
  res.widened = w;
  return res;
}

static void checks(const std::locale& shared, int r) {
  std::locale copy = shared;
  CHECK(copy == shared && !(copy != shared));
  CHECK(copy.name() == "*" || copy.name() == shared.name());
  CHECK(std::has_facet<Tag>(shared) && std::use_facet<Tag>(shared).value == 42);
  CHECK(std::use_facet<std::numpunct<char>>(shared).thousands_sep() == '\'');
  CHECK(std::use_facet<std::numpunct<char>>(std::locale::classic()).thousands_sep() == ',');
  CHECK(std::locale::classic().name() == "C" && std::locale().name() == "C");
  std::locale mixed = std::locale::classic().combine<Tag>(shared);
  CHECK(std::use_facet<Tag>(mixed).value == 42);
  CHECK(std::use_facet<std::numpunct<char>>(mixed).thousands_sep() == ',');
  CHECK(shared(std::string("abc"), std::string("abd")) && !shared(std::string("b"), std::string("a")));
  CHECK(std::isalpha('q', shared) && !std::isdigit('q', shared) && std::isspace(' ', shared));
  CHECK(std::tolower('Q', shared) == 'q');
  const auto& cv = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(shared);
  std::mbstate_t st{};
  const char src[] = "codecvt";
  wchar_t dst[8];
  const char* from_next;
  wchar_t* to_next;
  CHECK(cv.in(st, src, src + 7, from_next, dst, dst + 8, to_next) == std::codecvt_base::ok);
  CHECK(to_next == dst + 7 && dst[0] == L'c' && dst[6] == L't');
  // distinct locales, constructed concurrently
  std::locale own(std::locale::classic(), new Tag(r));
  CHECK(std::use_facet<Tag>(own).value == r);
  std::locale named("C");
  CHECK(named == std::locale::classic() || named.name() == "C");
}

int main() {
  const std::locale shared(std::locale(std::locale::classic(), new Grouping), new Tag(42));
  std::vector<Results> expected;
  for (int r = 0; r < 8; ++r) expected.push_back(compute(shared, r));
  CHECK(expected[0].num == "1'234'567 9'876'543,21 1");
  CHECK(expected[0].parsed == 7654321.5);
  CHECK(expected[0].fmt.starts_with("1'234'567 12'345,5 true"));
  CHECK(expected[0].upper == "HELLO, WORLD 0");
  std::latch go(K);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      for (int r = 0; r < 40; ++r) {
        const int i = (k + r) % 8;
        CHECK(compute(shared, i) == expected[static_cast<std::size_t>(i)]);
        checks(shared, k * 100 + r);
      }
    });
  for (auto& t : ts) t.join();
}
