// Many threads emit at the same time, each to a stream buffer of its own whose output
// functions block until every thread is inside its emit(); and syncbufs whose wrapped buffer
// itself emits through another syncbuf to a further buffer of its own.
//   [syncstream.syncbuf.members]/1-2: emit() transfers the output to *wrapped; only emits to
//     the SAME stream buffer are ordered ("All emit() calls transferring characters to the same
//     stream buffer object appear to execute in a total order"); /5: "May call member
//     functions of wrapped while holding a lock uniquely associated with wrapped": a lock per
//     buffer, so emits to distinct buffers never wait for each other, however many there are,
//     and a nested emit (buffer A's member function emitting to buffer B) takes A's lock and
//     then B's, which cannot form a cycle when every A_i and B_i is distinct.
// So every thread reaches the rendezvous inside its emit and every emit returns true with the
// whole text transferred. A wait that has not completed after 20 seconds is reported as a
// deadlock (the threads are then abandoned: the process exits).
// FLAGS: -pthread
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <streambuf>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>
#include <unistd.h>
#include "check.hpp"

constexpr int K = 160;  // more threads than any small fixed number of locks

static std::atomic<int> arrived{0};
static std::atomic<bool> deadlocked{false};

// Waits until all K threads have arrived, or reports a deadlock.
static void rendezvous() {
  arrived.fetch_add(1);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
  while (arrived.load() < K) {
    if (std::chrono::steady_clock::now() > deadline) {
      deadlocked = true;
      return;
    }
    std::this_thread::yield();
  }
}

// A buffer that records what it receives and blocks once at the rendezvous.
struct Blocking : std::streambuf {
  std::string text;
  bool met = false;
  int_type overflow(int_type c) override {
    meet();
    if (!traits_type::eq_int_type(c, traits_type::eof())) text += traits_type::to_char_type(c);
    return traits_type::not_eof(c);
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    meet();
    text.append(s, std::size_t(n));
    return n;
  }
  void meet() {
    if (!met) {
      met = true;
      rendezvous();
    }
  }
};

// A buffer that forwards what it receives through an osyncstream to a further buffer.
struct Relay : std::streambuf {
  Blocking* target;
  explicit Relay(Blocking* t) : target(t) {}
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    std::osyncstream out(target);
    out.write(s, n);
    out.emit();
    return out ? n : 0;
  }
  int_type overflow(int_type c) override {
    if (traits_type::eq_int_type(c, traits_type::eof())) return traits_type::not_eof(c);
    const char ch = traits_type::to_char_type(c);
    return xsputn(&ch, 1) == 1 ? c : traits_type::eof();
  }
};

static void run(bool nested) {
  arrived = 0;
  deadlocked = false;
  std::vector<Blocking> finals(K);
  std::vector<Relay> relays;
  relays.reserve(K);
  for (int i = 0; i < K; ++i) relays.emplace_back(&finals[i]);
  bool ok[K] = {};
  std::vector<std::thread> ts;
  for (int i = 0; i < K; ++i)
    ts.emplace_back([&, i] {
      std::streambuf* wrapped = nested ? static_cast<std::streambuf*>(&relays[i]) : &finals[i];
      std::syncbuf sb(wrapped);
      std::ostream os(&sb);
      os << "thread " << i << " says hello";
      ok[i] = sb.emit();
    });
  // Give the threads time; if they deadlock, report and leave without joining.
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
  while (arrived.load() < K && !deadlocked.load() && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  if (deadlocked.load() || arrived.load() < K) {
    dprintf(2, "%s: only %d of %d emits reached their buffers: emits to distinct buffers wait for each other\n",
            nested ? "nested" : "direct", arrived.load(), K);
    _exit(1);
  }
  for (auto& t : ts) t.join();
  for (int i = 0; i < K; ++i) {
    CHECK(ok[i]);
    CHECK(finals[i].text == "thread " + std::to_string(i) + " says hello");
  }
}

int main() {
  run(false);
  run(true);
  return 0;
}
