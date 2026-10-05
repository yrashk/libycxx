// text_encoding::environment() reports the encoding of the environment's locale even when the
// program changed the C locale with setlocale before its first call, and from many threads at
// once on first use:
//   [text.encoding.members]/14: "Returns: A text_encoding object representing the
//     implementation-defined character encoding scheme of the environment. On a POSIX
//     implementation, this is the encoding scheme associated with the POSIX locale denoted by
//     the empty string """; /15: "[Note 2: This function is not affected by calls to
//     setlocale.]"; /17 environment_is<i>(): "Returns: environment() == i".
//   [res.on.data.races]: calls from several threads do not race; each returns the same value.
// The program reruns itself with LC_ALL set (a UTF-8 locale, or "C"), and the child compares
// environment() with the codeset POSIX reports for the locale "" (nl_langinfo_l(CODESET) of
// newlocale(LC_CTYPE_MASK, "", 0)), after setting the C locale to the other one.
// FLAGS: -pthread
#include <atomic>
#include <cstdlib>
#include <string>
#include <text_encoding>
#include <thread>
#include <langinfo.h>
#include <locale.h>
#include "child_process.hpp"
#include "check.hpp"

static const char* const utf8_candidates[] = {"C.UTF-8", "C.utf8", "en_US.UTF-8", "en_US.utf8", "UTF-8"};

static const char* find_utf8_locale() {
  for (const char* n : utf8_candidates) {
    locale_t l = newlocale(LC_CTYPE_MASK, n, locale_t(0));
    if (l) {
      freelocale(l);
      return n;
    }
  }
  return nullptr;
}

// The codeset of the environment's locale, as POSIX reports it.
static std::string environment_codeset() {
  locale_t l = newlocale(LC_CTYPE_MASK, "", locale_t(0));
  CHECK(l != locale_t(0));
  std::string s = nl_langinfo_l(CODESET, l);
  freelocale(l);
  return s;
}

static int child(const char* other_locale, bool threads) {
  const std::text_encoding want(environment_codeset());
  CHECK(setlocale(LC_ALL, other_locale) != nullptr);  // before the first environment() call
  std::text_encoding got[8];
  if (threads) {
    std::atomic<bool> go{false};
    std::thread ts[8];
    for (int i = 0; i < 8; ++i)
      ts[i] = std::thread([&, i] {
        while (!go.load()) {
        }
        got[i] = std::text_encoding::environment();
      });
    go = true;
    for (auto& t : ts) t.join();
  } else {
    for (auto& g : got) g = std::text_encoding::environment();
  }
  for (const auto& g : got) {
    if (!(g == want)) dprintf(2, "environment() is %s, the locale \"\" has %s\n", g.name(), want.name());
    CHECK(g == want);
  }
  CHECK(std::text_encoding::environment_is<std::text_encoding::id::UTF8>() == (want == std::text_encoding::id::UTF8));
  CHECK(setlocale(LC_ALL, "") != nullptr);
  CHECK(std::text_encoding::environment() == want);
  return 0;
}

int main(int, char** argv) {
  const char* utf8 = find_utf8_locale();
  if (const char* m = child_mode()) {
    const bool threads = std::string(m).ends_with("-threads");
    return std::string(m).starts_with("utf8") ? child("C", threads) : child(utf8, threads);
  }
  (void)argv;
  if (!utf8) {
    dprintf(1, "no UTF-8 locale installed: nothing to compare\n");
    return 0;
  }
  // The environment's locale is UTF-8, the program switches to "C" first; then the reverse.
  for (const char* mode : {"utf8", "utf8-threads", "c", "c-threads"}) {
    setenv("LC_ALL", mode[0] == 'u' ? utf8 : "C", 1);
    const ChildResult r = run_self(mode);
    if (r.status != 0) dprintf(2, "mode %s: status %d\n%s", mode, r.status, r.err.c_str());
    CHECK(r.status == 0);
  }
  return 0;
}
