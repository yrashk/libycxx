// emit() calls to one stream buffer stay mutually exclusive when the syncbufs that make them
// were move-constructed, move-assigned or swapped, also when that changes the stream buffer a
// syncbuf wraps.
//   [syncstream.syncbuf.members]/1: emit "Atomically transfers the associated output of *this to
//     the stream buffer *wrapped, so that it appears in the output stream as a contiguous
//     sequence of characters"; /2: "All emit() calls transferring characters to the same stream
//     buffer object appear to execute in a total order".
//   [syncstream.syncbuf.cons]/5: a moved-to syncbuf wraps what other wrapped, with other's
//     stored output; [syncstream.syncbuf.assign]/1: operator= "Calls emit() then move assigns
//     from rhs"; /6 swap "Exchanges the state of *this and other" (the wrapped buffers included).
// Two target stream buffers record any overlap of their xsputn/overflow calls (each call
// sleeps briefly to widen the window). Six threads write lines through syncbufs they move,
// move-assign and swap (between targets) before emitting; every line arrives whole and no two
// transfers to the same target overlap.
// FLAGS: -pthread
#include <atomic>
#include <chrono>
#include <mutex>
#include <streambuf>
#include <string>
#include <syncstream>
#include <thread>
#include <utility>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

struct Target : std::streambuf {
  std::atomic<int> inside{0};
  std::atomic<int> overlaps{0};
  std::mutex m;  // protects text (the test's own bookkeeping, not the exclusion under test)
  std::string text;
  void enter() {
    if (inside.fetch_add(1) != 0) overlaps.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::microseconds(50));
  }
  void leave() { inside.fetch_sub(1); }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    enter();
    {
      std::lock_guard<std::mutex> g(m);
      text.append(s, static_cast<std::size_t>(n));
    }
    leave();
    return n;
  }
  int_type overflow(int_type c) override {
    if (traits_type::eq_int_type(c, traits_type::eof())) return traits_type::not_eof(c);
    enter();
    {
      std::lock_guard<std::mutex> g(m);
      text.push_back(traits_type::to_char_type(c));
    }
    leave();
    return c;
  }
};

static std::string line_for(int t, int i, char target) {
  return "<" + std::to_string(t) + ":" + std::to_string(i) + ":" + target + std::string(40, char('a' + t)) + ">\n";
}

int main() {
  watchdog(20);
  Target a, b;
  constexpr int T = 6, Lines = 60;
  std::vector<std::thread> ts;
  for (int t = 0; t < T; ++t)
    ts.emplace_back([&, t] {
      for (int i = 0; i < Lines; ++i) {
        std::syncbuf s1(&a), s2(&b);
        const std::string la = line_for(t, i, 'A'), lb = line_for(t, i, 'B');
        s1.sputn(la.data(), static_cast<std::streamsize>(la.size()));
        s2.sputn(lb.data(), static_cast<std::streamsize>(lb.size()));
        switch (i % 4) {
          case 0: {
            std::syncbuf m(std::move(s1));  // m wraps a, holds la
            CHECK(m.emit());
            CHECK(s2.emit());
            break;
          }
          case 1: {
            std::syncbuf m(&b);
            m = std::move(s1);  // emits m's (empty) output to b, then wraps a with la
            CHECK(m.get_wrapped() == &a && s1.get_wrapped() == nullptr);
            CHECK(m.emit());
            break;  // s2 emits lb to b in its destructor
          }
          case 2: {
            s1.swap(s2);  // s1 wraps b with lb, s2 wraps a with la
            CHECK(s1.get_wrapped() == &b && s2.get_wrapped() == &a);
            CHECK(s1.emit() && s2.emit());
            break;
          }
          default: {
            swap(s1, s2);
            std::syncbuf m1(std::move(s1)), m2(std::move(s2));
            break;  // both emitted by the destructors
          }
        }
      }
    });
  for (auto& th : ts) th.join();
  CHECK(a.overlaps.load() == 0);
  CHECK(b.overlaps.load() == 0);
  // Every line once, whole.
  for (auto [target, tag] : {std::pair<Target*, char>{&a, 'A'}, {&b, 'B'}}) {
    const std::string& text = target->text;
    std::size_t lines = 0, pos = 0;
    while (pos < text.size()) {
      const std::size_t end = text.find('\n', pos);
      CHECK(end != std::string::npos);
      const std::string line = text.substr(pos, end + 1 - pos);
      CHECK(line.front() == '<' && line[line.size() - 2] == '>');
      const char c = line[line.size() - 3];
      CHECK(line.find(std::string(40, c)) != std::string::npos);
      ++lines;
      pos = end + 1;
    }
    CHECK(lines == static_cast<std::size_t>(T * Lines));
    for (int t = 0; t < T; ++t)
      for (int i = 0; i < Lines; ++i) CHECK(text.find(line_for(t, i, tag)) != std::string::npos);
  }
  return 0;
}
