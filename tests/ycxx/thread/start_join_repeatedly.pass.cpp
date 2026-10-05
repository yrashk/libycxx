// A program that starts and joins threads over and over, with library objects created between
// the threads' lifetimes (while no other thread exists), shared by the next threads, and
// still in use while a detached thread runs in the background. [thread.thread.constr]/6: "The
// completion of the invocation of the constructor synchronizes with the beginning of the
// invocation of the copy of f"; [thread.thread.member]/5: "The completion of the thread
// represented by *this synchronizes with the corresponding successful join() return".
// [util.smartptr.shared.general]/4 (concurrent copies/destruction of distinct shared_ptr
// objects), [thread.mutex.requirements.mutex.general] (mutual exclusion), [locale.facet]/2
// (a facet with refs 0 is deleted with the last locale referring to it), [thread.once.callonce]
// (call_once runs the function once even when threads come and go), and thread_local objects
// are constructed once per thread and destroyed at its exit ([basic.stc.thread]).
// FLAGS: -pthread
#include <atomic>
#include <locale>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

struct Counted {
  static inline std::atomic<int> live{0};
  int v;
  explicit Counted(int x) : v(x) { ++live; }
  ~Counted() { --live; }
};

struct Tag : std::locale::facet {
  static std::locale::id id;
  static inline std::atomic<int> live{0};
  int v;
  explicit Tag(int x) : v(x) { ++live; }
  ~Tag() override { --live; }
};
std::locale::id Tag::id;

struct PerThread {
  static inline std::atomic<int> ctors{0}, dtors{0};
  PerThread() { ++ctors; }
  ~PerThread() { ++dtors; }
};
thread_local PerThread per_thread;

static std::atomic<int> failures{0};

int main() {
  watchdog(50);
  std::atomic<bool> stop_bg{false};
  std::atomic<bool> bg_done{false};
  std::once_flag once;
  std::atomic<int> once_calls{0};
  int body_threads = 0;

  for (int round = 0; round < 150; ++round) {
    // Created while the program has (at most) the background thread.
    auto sp = std::make_shared<Counted>(round);
    std::weak_ptr<Counted> wp = sp;
    std::mutex m;
    long guarded = 0;
    std::locale loc(std::locale::classic(), new Tag(round));
    const int nthreads = 1 + round % 4;

    auto body = [&, round] {
      (void)per_thread;  // constructs this thread's object
      std::call_once(once, [&] { ++once_calls; });
      std::locale mine = loc;
      for (int i = 0; i < 2000; ++i) {
        std::shared_ptr<Counted> c = sp;
        std::shared_ptr<Counted> d = wp.lock();
        if (!d || d->v != round || c.get() != d.get()) ++failures;
        std::lock_guard g(m);
        ++guarded;
      }
      if (std::use_facet<Tag>(mine).v != round) ++failures;
    };
    std::vector<std::thread> ts;
    for (int i = 0; i < nthreads; ++i) ts.emplace_back(body);
    body_threads += nthreads;
    for (int i = 0; i < 500; ++i) {
      std::shared_ptr<Counted> c = sp;
      std::lock_guard g(m);
      ++guarded;
    }
    for (auto& t : ts) t.join();
    CHECK(guarded == 2000L * nthreads + 500);
    CHECK(sp.use_count() == 1);
    CHECK(Counted::live.load() >= 1);
    sp.reset();
    CHECK(wp.expired());

    // A thread that starts a thread.
    if (round % 10 == 0) {
      std::atomic<int> inner{0};
      std::thread outer([&] {
        std::thread in([&] { inner = 1; });
        in.join();
        CHECK(inner.load() == 1);
      });
      outer.join();
    }
    // A background thread detached for part of the run, using its own objects and one
    // created by main before it started.
    if (round == 40) {
      auto shared_with_bg = std::make_shared<Counted>(-1);
      std::thread([&stop_bg, &bg_done, s = shared_with_bg] {
        while (!stop_bg.load()) {
          auto c = s;
          std::weak_ptr<Counted> w = c;
          if (w.lock()->v != -1) ++failures;
        }
        bg_done = true;
      }).detach();
      // main keeps copying the same control block while the next rounds run
      for (int i = 0; i < 1000; ++i) {
        auto c = shared_with_bg;
        (void)c;
      }
    }
    if (round == 110) {
      stop_bg = true;
      while (!bg_done.load()) std::this_thread::yield();
    }
  }
  // the facets of every round's locale are gone with their locales
  CHECK(Tag::live.load() == 0);
  CHECK(once_calls.load() == 1);
  CHECK(failures.load() == 0);
  // The detached thread may still be finishing its lambda's destructor: wait for its Counted.
  while (Counted::live.load() != 0) std::this_thread::yield();
  // every joined thread constructed its thread_local and destroyed it before join returned
  // ([basic.start.term]/2 thread exit; [thread.thread.member]/5); main never used it
  CHECK(PerThread::ctors.load() == body_threads);
  CHECK(PerThread::dtors.load() == body_threads);
}
