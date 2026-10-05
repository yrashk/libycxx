// First uses of named locales, regular expressions and locale-dependent formatting happen in
// many threads at once (a fresh child process per scenario; the threads start before anything
// is used and are released together). Every thread must get the same, correct results:
//   [res.on.data.races]/2-3: library functions called from several threads, on distinct
//     objects or through const access, do not race;
//   [locale.cons]/2 locale(const char* std_name): "Constructs a locale using standard C locale
//     names ... The resulting locale implements semantics defined to be associated with that
//     name": the same semantics in every thread and afterwards (which semantics a UTF-8 locale
//     name has beyond the basic character set is implementation-defined, so only agreement is
//     checked there); the same name gives equal locales ([locale.operators]: "each has a name
//     and the names are identical");
//   [re.regex.construct], [re.alg.match], [re.alg.search], [re.alg.replace], [re.traits]
//     lookup_classname: the same patterns match the same strings in every thread;
//   [format.string.std]: the L option with the classic locale gives the non-L result.
// FLAGS: -pthread
#include <atomic>
#include <format>
#include <locale>
#include <regex>
#include <string>
#include <thread>
#include <locale.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

constexpr int N = 8;

template <class F>
static void together(F body) {
  std::atomic<int> ready{0};
  std::atomic<bool> go{false};
  std::thread ts[N];
  for (int i = 0; i < N; ++i)
    ts[i] = std::thread([&, i] {
      ready.fetch_add(1);
      while (!go.load()) {
      }
      body(i);
    });
  while (ready.load() != N) {
  }
  go = true;
  for (auto& t : ts) t.join();
}

static const char* utf8_name() {
  for (const char* n : {"C.UTF-8", "C.utf8", "en_US.UTF-8", "en_US.utf8"}) {
    locale_t l = newlocale(LC_ALL_MASK, n, locale_t(0));
    if (l) {
      freelocale(l);
      return n;
    }
  }
  return nullptr;
}

static void named_locales() {
  const char* name = utf8_name();
  if (!name) return;
  const wchar_t probes[] = {L'a', L'Z', L'5', L'é', L'É', L'α', L'中', L' ', L' '};
  constexpr int P = sizeof probes / sizeof probes[0];
  struct Result {
    bool cls[P][3];  // alpha, upper, space
    wchar_t upper[P];
    bool ascii_ok;
    bool equal_locales;
    bool valid;
  };
  auto classify = [&](Result& r) {
    std::locale loc;
    try {
      loc = std::locale(name);
    } catch (const std::runtime_error&) {
      r.valid = false;  // the set of valid names is implementation-defined ([locale.cons]/4)
      return;
    }
    r.valid = true;
    const auto& ct = std::use_facet<std::ctype<wchar_t>>(loc);
    for (int j = 0; j < P; ++j) {
      r.cls[j][0] = ct.is(std::ctype_base::alpha, probes[j]);
      r.cls[j][1] = ct.is(std::ctype_base::upper, probes[j]);
      r.cls[j][2] = ct.is(std::ctype_base::space, probes[j]);
      r.upper[j] = ct.toupper(probes[j]);
    }
    // The basic character set classifies as in the classic locale.
    r.ascii_ok = r.cls[0][0] && !r.cls[0][1] && r.upper[0] == L'A' && r.cls[1][1] && !r.cls[2][0] && r.cls[7][2] &&
                 std::use_facet<std::ctype<char>>(loc).toupper('q') == 'Q';
    r.equal_locales = loc == std::locale(name) && loc.name() == name;
  };
  Result results[N] = {};
  together([&](int i) { classify(results[i]); });
  Result alone = {};
  classify(alone);  // once more, after the first uses, in one thread
  for (int i = 0; i < N; ++i) {
    CHECK(results[i].valid == alone.valid);
    if (!alone.valid) continue;
    CHECK(results[i].ascii_ok && results[i].equal_locales);
    for (int j = 0; j < P; ++j) {
      for (int q = 0; q < 3; ++q) CHECK(results[i].cls[j][q] == alone.cls[j][q]);
      CHECK(results[i].upper[j] == alone.upper[j]);
    }
  }
}

static void regexes() {
  int results[N] = {};
  together([&](int i) {
    int r = 0;
    r += std::regex_match("Hello42", std::regex("[[:alpha:]]+[[:digit:]]{2}"));
    r += std::regex_search(std::string("x  y"), std::regex("\\s{2}")) * 2;
    r += std::regex_match(L"ABC", std::wregex(L"[[:upper:]]+")) * 4;
    r += std::regex_match("abc", std::regex("ABC", std::regex::icase)) * 8;
    r += !std::regex_match("abc", std::regex("[[:digit:]]+", std::regex::extended)) * 16;
    r += std::regex_replace(std::string("a1b22"), std::regex("\\d+"), "#") == "a#b#" ? 32 : 0;
    r += std::regex_match("w_9", std::regex("\\w+")) * 64;
    results[i] = r;
  });
  for (int r : results) CHECK(r == 127);
}

static void locale_formatting() {
  std::string out[N];
  together([&](int i) {
    out[i] = std::format(std::locale::classic(), "{:L}|{:L}|{:L}|{}", 1234567, 3.25, true, i * 0);
  });
  for (const auto& s : out) CHECK(s == "1234567|3.25|true|0");
}

static void in_child(void (*f)(), const char* name) {
  const pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) {
    f();
    _exit(0);
  }
  int status = 0;
  CHECK(waitpid(pid, &status, 0) == pid);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) dprintf(2, "%s failed (status %#x)\n", name, status);
  CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

int main() {
  in_child(named_locales, "named locales");
  in_child(regexes, "regular expressions");
  in_child(locale_formatting, "locale-dependent formatting");
  return 0;
}
