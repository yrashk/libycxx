// [iostream.objects.overview]/7: "Concurrent access to a synchronized ([ios.members.static])
// standard iostream object's formatted and unformatted input ([istream]) ... functions ... by
// multiple threads does not result in a data race". Input functions update the stream's
// bookkeeping (gcount, [istream.unformatted]/1; the state on failure and at end of file,
// [istream.formatted.reqmts], [istream.unformatted]) and flush the tied cout first
// ([istream.sentry]/2: "if is.tie() is not a null pointer, the function calls
// is.tie()->flush()"), while other threads write to cout.
// A child process makes its standard input a pipe fed by a thread and runs reader threads on
// cin:
// - mode "get": every reader uses only get() (one character per call; with the stream
//   synchronized with stdin each call takes one character from the C stream, [ios.members.
//   static]/2), so the characters the readers obtained are exactly the characters sent;
// - mode "mixed": readers use >> long long, >> string, getline, ignore, read, readsome, get()
//   and peek, while two threads write to cout; every reader must reach end of file, and the
//   text it extracted consists of characters that were sent (digits, spaces, newlines).
// Run under TSan this also checks for data races.
// FLAGS: -pthread
#include <atomic>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <latch>
#include <map>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"
#include "watchdog.hpp"

constexpr int K = 4;

static std::string input_text() {
  std::string s;
  for (int i = 0; i < 20000; ++i) {
    s += std::to_string(i * 37 % 10007);
    s += (i % 13 == 0) ? '\n' : ' ';
  }
  return s;
}

// Makes fd 0 a pipe fed with text by a detached thread.
static void feed_stdin(const std::string& text) {
  int p[2];
  if (pipe(p) != 0) std::_Exit(4);
  dup2(p[0], 0);
  close(p[0]);
  int w = p[1];
  std::thread([w, text] {
    std::size_t off = 0;
    while (off < text.size()) {
      ssize_t n = write(w, text.data() + off, std::min<std::size_t>(text.size() - off, 777));
      if (n <= 0) std::_Exit(5);
      off += static_cast<std::size_t>(n);
    }
    close(w);
  }).detach();
}

static int child_get() {
  const std::string text = input_text();
  feed_stdin(text);
  std::vector<std::string> got(K);
  std::latch go(K);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      for (;;) {
        auto c = std::cin.get();
        if (c == std::char_traits<char>::eof()) break;
        got[static_cast<std::size_t>(k)] += static_cast<char>(c);
      }
    });
  for (auto& t : ts) t.join();
  std::map<char, long> want, have;
  std::size_t total = 0;
  for (char c : text) ++want[c];
  for (auto& g : got) {
    total += g.size();
    for (char c : g) ++have[c];
  }
  if (total != text.size() || have != want) {
    dprintf(2, "got %zu characters, want %zu\n", total, text.size());
    return 6;
  }
  if (!std::cin.eof()) return 7;
  return 0;
}

static int child_mixed() {
  const std::string text = input_text();
  feed_stdin(text);
  std::atomic<int> finished{0};
  std::atomic<bool> bad{false};
  std::latch go(K + 2);
  std::vector<std::thread> ts;
  auto valid = [&](const std::string& s) {
    for (char c : s)
      if (!((c >= '0' && c <= '9') || c == ' ' || c == '\n')) bad = true;
  };
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      std::string s;
      char buf[16];
      // Only input functions are called on cin here (its state is not queried: rdstate and
      // clear are not input functions); peek() returns eof once the input is exhausted.
      for (int r = 0;; ++r) {
        switch ((k + r) % 7) {
          case 0: {
            // 0 when the conversion fails ([facet.num.get.virtuals]); left unchanged when the
            // sentry fails ([istream.formatted.reqmts]/1), which happens when another reader
            // reached end of file after this one's peek().
            long long x = 0;
            std::cin >> x;
            if (x < 0) bad = true;
            break;
          }
          case 1:
            s = "0";
            std::cin >> s;
            valid(s);
            break;
          case 2:
            s.clear();
            std::getline(std::cin, s);
            valid(s);
            break;
          case 3:
            std::cin.ignore(5);
            break;
          case 4:
            for (char& c : buf) c = ' ';
            std::cin.read(buf, 10);
            valid(std::string(buf, 10));
            break;
          case 5:
            std::cin.readsome(buf, 8);
            break;
          default: {
            auto c = std::cin.get();
            if (c != std::char_traits<char>::eof()) valid(std::string(1, static_cast<char>(c)));
          }
        }
        if (std::cin.peek() == std::char_traits<char>::eof()) break;
      }
      ++finished;
    });
  for (int k = 0; k < 2; ++k)
    ts.emplace_back([&] {
      go.arrive_and_wait();
      for (int r = 0; r < 3000; ++r) std::cout << r << ' ' << "out" << '\n';
    });
  for (auto& t : ts) t.join();
  std::cout << std::flush;
  if (finished != K) return 6;
  if (bad) return 7;
  return 0;
}

int main(int argc, char** argv) {
  if (child_mode()) {
    watchdog(120);
    return std::string(argv[1]) == "get" ? child_get() : child_mixed();
  }
  (void)argc;
  ChildResult g = run_self("get");
  if (g.status != 0) dprintf(2, "child status %d, stderr:\n%s\n", g.status, g.err.c_str());
  CHECK(g.status == 0);
  CHECK(g.err.empty());
  ChildResult m = run_self("mixed");
  if (m.status != 0) dprintf(2, "child status %d, stderr:\n%s\n", m.status, m.err.c_str());
  CHECK(m.status == 0);
  CHECK(m.err.empty());
  CHECK(m.out.size() > 2 * 3000 * 6);
}
