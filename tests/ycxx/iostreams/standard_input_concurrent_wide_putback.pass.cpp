// [iostream.objects.overview]/7: "Concurrent access to a synchronized ([ios.members.static])
// standard iostream object's formatted and unformatted input ([istream]) and output ([ostream])
// functions or a standard C stream by multiple threads does not result in a data race". This
// covers the wide objects and the input functions that give characters back
// ([istream.unformatted]: putback(c) calls rdbuf()->sputbackc(c), unget() calls
// rdbuf()->sungetc(), each setting badbit when that returns eof; peek() returns rdbuf()->sgetc()
// or eof when !good()). [wide.stream.objects]/2: "After the object wcin is initialized,
// wcin.tie() returns &wcout", and [istream.sentry]/2: input first flushes the tied stream, while
// other threads write to wcout. (iostreams/standard_input_concurrent covers cin with get, >>,
// getline, ignore, read, readsome and peek.)
// A child process makes its standard input a pipe fed by a thread, and runs reader threads:
// - mode "wget": every reader uses only wcin.get(); with the stream synchronized with stdin
//   each call takes one character from the C stream ([ios.members.static]/2), so the
//   characters the readers obtained are exactly the characters sent;
// - mode "wmixed": readers use wcin >> long long, >> wstring, getline, read, readsome, get,
//   peek, putback and unget while two threads write to wcout; every reader must stop (peek()
//   reaches eof: end of input, or a failed putback/unget made the stream bad) and everything it
//   extracted consists of characters that were sent;
// - mode "putback": the same on cin with get, putback(c), unget(), peek and ignore, with two
//   threads writing to cout.
// Run under TSan this also checks for data races. Whether a putback succeeds when another
// thread has read in between is not specified (sputbackc may fail), so the stream state is not
// checked.
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
  for (int i = 0; i < 12000; ++i) {
    s += std::to_string(i * 37 % 10007);
    s += (i % 11 == 0) ? '\n' : ' ';
  }
  return s;
}

static void feed_stdin(const std::string& text) {
  int p[2];
  if (pipe(p) != 0) std::_Exit(4);
  dup2(p[0], 0);
  close(p[0]);
  int w = p[1];
  std::thread([w, text] {
    std::size_t off = 0;
    while (off < text.size()) {
      ssize_t n = write(w, text.data() + off, std::min<std::size_t>(text.size() - off, 613));
      if (n <= 0) std::_Exit(5);
      off += static_cast<std::size_t>(n);
    }
    close(w);
  }).detach();
}

template <class C>
static bool sent_char(C c) {
  return (c >= C('0') && c <= C('9')) || c == C(' ') || c == C('\n');
}

static int child_wget() {
  const std::string text = input_text();
  feed_stdin(text);
  std::vector<std::wstring> got(K);
  std::latch go(K);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      for (;;) {
        auto c = std::wcin.get();
        if (c == std::char_traits<wchar_t>::eof()) break;
        got[static_cast<std::size_t>(k)] += static_cast<wchar_t>(c);
      }
    });
  for (auto& t : ts) t.join();
  std::map<wchar_t, long> want, have;
  std::size_t total = 0;
  for (char c : text) ++want[static_cast<wchar_t>(c)];
  for (auto& g : got) {
    total += g.size();
    for (wchar_t c : g) ++have[c];
  }
  if (total != text.size() || have != want) {
    dprintf(2, "got %zu characters, want %zu\n", total, text.size());
    return 6;
  }
  if (!std::wcin.eof()) return 7;
  return 0;
}

static int child_wmixed() {
  feed_stdin(input_text());
  std::atomic<int> finished{0};
  std::atomic<bool> bad{false};
  std::latch go(K + 2);
  std::vector<std::thread> ts;
  auto valid = [&](const std::wstring& s) {
    for (wchar_t c : s)
      if (!sent_char(c)) bad = true;
  };
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      std::wstring s;
      wchar_t buf[16];
      for (int r = 0;; ++r) {
        switch ((k + r) % 8) {
          case 0: {
            long long x = 0;  // unchanged when the sentry fails, 0 when nothing was extracted
            std::wcin >> x;
            if (x < 0) bad = true;
            break;
          }
          case 1:
            s = L"0";
            std::wcin >> s;
            valid(s);
            break;
          case 2:
            s.clear();
            std::getline(std::wcin, s);
            valid(s);
            break;
          case 3:
            for (wchar_t& c : buf) c = L' ';
            std::wcin.read(buf, 10);
            valid(std::wstring(buf, 10));
            break;
          case 4:
            std::wcin.readsome(buf, 8);
            break;
          case 5: {
            auto c = std::wcin.get();
            if (c != std::char_traits<wchar_t>::eof()) {
              if (!sent_char(static_cast<wchar_t>(c))) bad = true;
              std::wcin.putback(static_cast<wchar_t>(c));
            }
            break;
          }
          case 6:
            if (std::wcin.get() != std::char_traits<wchar_t>::eof()) std::wcin.unget();
            std::wcin.ignore(3);
            break;
          default: {
            auto c = std::wcin.get();
            if (c != std::char_traits<wchar_t>::eof() && !sent_char(static_cast<wchar_t>(c))) bad = true;
          }
        }
        if (std::wcin.peek() == std::char_traits<wchar_t>::eof()) break;
      }
      ++finished;
    });
  for (int k = 0; k < 2; ++k)
    ts.emplace_back([&] {
      go.arrive_and_wait();
      for (int r = 0; r < 2000; ++r) std::wcout << r << L' ' << L"out" << L'\n';
    });
  for (auto& t : ts) t.join();
  std::wcout << std::flush;
  if (finished != K) return 6;
  if (bad) return 7;
  return 0;
}

static int child_putback() {
  feed_stdin(input_text());
  std::atomic<int> finished{0};
  std::atomic<bool> bad{false};
  std::latch go(K + 2);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      for (int r = 0;; ++r) {
        auto c = std::cin.get();
        if (c != std::char_traits<char>::eof()) {
          if (!sent_char(static_cast<char>(c))) bad = true;
          switch ((k + r) % 4) {
            case 0:
              std::cin.putback(static_cast<char>(c));
              break;
            case 1:
              std::cin.unget();
              break;
            case 2:
              std::cin.ignore(2);
              break;
            default:
              break;
          }
          auto d = std::cin.get();
          if (d != std::char_traits<char>::eof() && !sent_char(static_cast<char>(d))) bad = true;
        }
        if (std::cin.peek() == std::char_traits<char>::eof()) break;
      }
      ++finished;
    });
  for (int k = 0; k < 2; ++k)
    ts.emplace_back([&] {
      go.arrive_and_wait();
      for (int r = 0; r < 2000; ++r) std::cout << r << ' ' << "out" << '\n';
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
    std::string m = argv[1];
    return m == "wget" ? child_wget() : m == "wmixed" ? child_wmixed() : child_putback();
  }
  (void)argc;
  for (const char* mode : {"wget", "wmixed", "putback"}) {
    ChildResult r = run_self(mode);
    if (r.status != 0) dprintf(2, "%s: child status %d, stderr:\n%s\n", mode, r.status, r.err.c_str());
    CHECK(r.status == 0);
    CHECK(r.err.empty());
    if (mode[0] != 'w' || mode[1] == 'm') CHECK(r.out.size() > 2 * 2000 * 6);
  }
}
