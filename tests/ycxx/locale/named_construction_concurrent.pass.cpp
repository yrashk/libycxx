// Concurrent construction of named locales and byname facets: a regression stress test.
//   [res.on.data.races]/2-3: library functions do not access objects other than their arguments'
//     (directly or indirectly), so constructing distinct locale and facet objects from several
//     threads at once must be free of data races;
//   [locale.cons]/2 (locale(const char* std_name)): a locale using the named locale's facets, whose
//     name() is std_name; constructing it does not change the C locale (the global C++ locale
//     stays classic: [locale.statics]);
//   [locale.numpunct.byname], [locale.moneypunct.byname], [locale.ctype.byname],
//     [locale.collate.byname]: the byname facets take their values from the named locale.
// 8 threads, released together by a latch, each construct, use and destroy named locales and
// byname facets (each facet is destroyed at once) for several locales, mixing the kinds, for
// about 0.6 s. Every value must equal the value the same construction gave before any thread
// started (and decimal_point / currency symbols agree with the C library's localeconv). Run
// under ThreadSanitizer as well.
// FLAGS: -pthread
#include <locale.h>
#include <string.h>
#include <atomic>
#include <chrono>
#include <latch>
#include <locale>
#include <string>
#include <thread>
#include <vector>
#include "check.hpp"
#include "named_locale.hpp"

// Facets with a public destructor, so each is a local object constructed and destroyed at once.
struct numpunct_n : std::numpunct_byname<char> {
  using std::numpunct_byname<char>::numpunct_byname;
};
template <bool Intl>
struct moneypunct_n : std::moneypunct_byname<char, Intl> {
  using std::moneypunct_byname<char, Intl>::moneypunct_byname;
};
struct ctype_n : std::ctype_byname<wchar_t> {
  using std::ctype_byname<wchar_t>::ctype_byname;
};
struct collate_n : std::collate_byname<char> {
  using std::collate_byname<char>::collate_byname;
};

// Byte strings whose order is compared (including bytes >= 0x80, which name letters in the
// ISO 8859 locales and lead bytes in UTF-8 ones).
const std::string words[] = {"a", "b", "B", "abc", "abd", "Zebra", "apple", "\xe4pfel", "\xc3\xa4pfel", "b\xfcro", "10", "9"};
constexpr int n_words = sizeof(words) / sizeof(words[0]);

using mp_local = std::moneypunct<char, false>;
using mp_intl = std::moneypunct<char, true>;

int sign(int v) { return (v > 0) - (v < 0); }

struct expected {
  const char* name;
  char point, sep;
  std::string grouping;
  std::string curr[2], pos[2], neg[2];
  int frac[2];
  char mon_point;
  bool c_has_curr[2];
  int order[n_words][n_words];
  std::string xform[n_words];
  bool alpha_e_acute, upper_a, space;
};


template <bool Intl>
void fill_money(expected& e, const char* n) {
  moneypunct_n<Intl> m(n);
  e.curr[Intl] = m.curr_symbol();
  e.pos[Intl] = m.positive_sign();
  e.neg[Intl] = m.negative_sign();
  e.frac[Intl] = m.frac_digits();
  if (!Intl)
    e.mon_point = m.decimal_point();
}

expected reference(const char* n) {
  expected e{};
  e.name = n;
  {
    numpunct_n np(n);
    e.point = np.decimal_point();
    e.sep = np.thousands_sep();
    e.grouping = np.grouping();
  }
  fill_money<false>(e, n);
  fill_money<true>(e, n);
  {
    collate_n c(n);
    for (int i = 0; i < n_words; ++i) {
      e.xform[i] = c.transform(words[i].data(), words[i].data() + words[i].size());
      for (int j = 0; j < n_words; ++j)
        e.order[i][j] = sign(c.compare(words[i].data(), words[i].data() + words[i].size(), words[j].data(),
                                       words[j].data() + words[j].size()));
    }
  }
  {
    ctype_n ct(n);
    e.alpha_e_acute = ct.is(std::ctype_base::alpha, L'é');
    e.upper_a = ct.toupper(L'a') == L'A';
    e.space = ct.is(std::ctype_base::space, L' ');
  }
  // The C library's view, for sanity.
  in_c_locale(n, [&] {
    const lconv* lc = localeconv();
    if (strlen(lc->decimal_point) == 1)
      CHECK_SAY(e.point == lc->decimal_point[0], "%s: decimal_point '%c' vs C '%s'", n, e.point, lc->decimal_point);
    e.c_has_curr[0] = lc->currency_symbol && *lc->currency_symbol;
    e.c_has_curr[1] = lc->int_curr_symbol && *lc->int_curr_symbol;
    return 0;
  });
  for (int intl = 0; intl < 2; ++intl)
    if (e.c_has_curr[intl])
      CHECK_SAY(!e.curr[intl].empty(), "%s: curr_symbol(intl=%d) empty", n, intl);
  // Sanity of the collation: "a" < "b", "abc" < "abd", each word equal to itself.
  CHECK(e.order[0][1] < 0 && e.order[3][4] < 0);
  for (int i = 0; i < n_words; ++i)
    CHECK(e.order[i][i] == 0);
  CHECK(e.upper_a && e.space);
  return e;
}

int main() {
  const char* names[] = {require_locale("de_DE.UTF-8"), require_locale("fr_FR.ISO8859-15"),
                         require_locale("en_US.UTF-8"), require_locale("de_DE.ISO8859-1"),
                         require_locale("ja_JP.UTF-8"), require_locale("fr_FR.UTF-8"),
                         require_locale("ru_RU.UTF-8"), require_locale("en_US.ISO8859-1")};
  constexpr int n_names = sizeof(names) / sizeof(names[0]);
  std::vector<expected> ref;
  for (const char* n : names)
    ref.push_back(reference(n));
  CHECK(ref[0].point == ',' && ref[1].point == ',' && ref[2].point == '.');
  CHECK(!ref[0].curr[0].empty() && !ref[2].curr[0].empty() && !ref[2].curr[1].empty());

  constexpr int n_threads = 8;
  std::latch start(n_threads);
  std::atomic<long> total{0};
  std::vector<std::thread> ts;
  for (int k = 0; k < n_threads; ++k)
    ts.emplace_back([&, k] {
      start.arrive_and_wait();
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(600);
      long it = 0;
      for (; it < 200 || std::chrono::steady_clock::now() < deadline; ++it) {
        const expected& e = ref[(it * 3 + k) % n_names];
        const char* n = e.name;
        switch ((it + k) % 7) {
        case 0: {
          numpunct_n np(n);
          CHECK_SAY(np.decimal_point() == e.point && np.thousands_sep() == e.sep && np.grouping() == e.grouping,
                    "%s: numpunct_byname", n);
          break;
        }
        case 1: {
          moneypunct_n<false> m(n);
          CHECK_SAY(m.curr_symbol() == e.curr[0] && m.positive_sign() == e.pos[0] && m.negative_sign() == e.neg[0] &&
                        m.frac_digits() == e.frac[0] && m.decimal_point() == e.mon_point,
                    "%s: moneypunct_byname<char, false>", n);
          break;
        }
        case 2: {
          moneypunct_n<true> m(n);
          CHECK_SAY(m.curr_symbol() == e.curr[1] && m.positive_sign() == e.pos[1] && m.negative_sign() == e.neg[1] &&
                        m.frac_digits() == e.frac[1],
                    "%s: moneypunct_byname<char, true>", n);
          break;
        }
        case 3: {
          ctype_n ct(n);
          CHECK_SAY(ct.is(std::ctype_base::alpha, L'é') == e.alpha_e_acute && ct.toupper(L'a') == L'A' &&
                        ct.is(std::ctype_base::space, L' ') && ct.widen('x') == L'x',
                    "%s: ctype_byname<wchar_t>", n);
          break;
        }
        case 4: {
          collate_n c(n);
          const int i = static_cast<int>(it % n_words), j = static_cast<int>((it / n_words + k) % n_words);
          CHECK_SAY(sign(c.compare(words[i].data(), words[i].data() + words[i].size(), words[j].data(),
                                   words[j].data() + words[j].size())) == e.order[i][j] &&
                        c.transform(words[i].data(), words[i].data() + words[i].size()) == e.xform[i],
                    "%s: collate_byname, words %d and %d", n, i, j);
          break;
        }
        case 5: {
          std::locale l(n);
          CHECK_SAY(l.name() == n, "%s: locale name %s", n, l.name().c_str());
          CHECK_SAY(std::use_facet<std::numpunct<char>>(l).decimal_point() == e.point &&
                        std::use_facet<mp_intl>(l).curr_symbol() == e.curr[1],
                    "%s: locale facets", n);
          break;
        }
        default: {
          // A locale holding byname facets, destroyed with the locale.
          std::locale l(std::locale::classic(), new std::collate_byname<char>(n));
          std::locale m(l, new std::moneypunct_byname<char, false>(n));
          CHECK_SAY(std::use_facet<mp_local>(m).curr_symbol() == e.curr[0] &&
                        sign(std::use_facet<std::collate<char>>(m).compare(words[7].data(), words[7].data() + words[7].size(),
                                                                          words[1].data(), words[1].data() + words[1].size())) ==
                            e.order[7][1],
                    "%s: locale with byname facets", n);
          CHECK(std::use_facet<std::numpunct<char>>(m).decimal_point() == '.');
          break;
        }
        }
        if (it % 64 == 0) {
          // The C locale and the global C++ locale are untouched.
          CHECK(strcmp(setlocale(LC_ALL, nullptr), "C") == 0);
          CHECK(std::locale().name() == "C" && std::locale::classic().name() == "C");
        }
      }
      total += it;
    });
  for (auto& t : ts)
    t.join();
  CHECK(total.load() >= n_threads * 200L);
  CHECK(strcmp(setlocale(LC_ALL, nullptr), "C") == 0 && std::locale().name() == "C");
  // The values are still right after the storm.
  for (int i = 0; i < n_names; ++i) {
    expected again = reference(names[i]);
    CHECK(again.point == ref[i].point && again.curr[0] == ref[i].curr[0] && again.curr[1] == ref[i].curr[1]);
  }
  return 0;
}
