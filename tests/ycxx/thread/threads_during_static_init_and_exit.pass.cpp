// Threads started and joined during dynamic initialization of static objects (before main)
// and during their destruction (after main returns), using library objects created before any
// thread existed. [basic.start.dynamic]: ordered initialization within a translation unit
// follows the definition order; [basic.start.term]/1,/3: static objects are destroyed in the
// reverse order of the completion of their constructors, after main returns
// ([basic.start.main]/5: returning from main calls exit). A std::thread may be created and
// joined at any of these points ([thread.thread.constr], [thread.thread.member]); shared_ptr,
// mutex and condition_variable objects keep their guarantees ([util.smartptr.shared.general]/4,
// [thread.mutex.requirements.mutex.general], [thread.condition.condvar]).
// The program writes its progress with write(2) so that the parent (run_self) sees what
// happened after main returned.
// FLAGS: -pthread
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include "child_process.hpp"
#include "check.hpp"

static void say(const char* s) {
  ssize_t r = write(1, s, std::char_traits<char>::length(s));
  (void)r;
}

struct Payload {
  static inline std::atomic<int> dtors{0};
  int v = 7;
  ~Payload() { ++dtors; }
};

static bool in_child() { return child_mode() != nullptr; }

// 1. created first, single-threaded
static std::shared_ptr<Payload> g_sp = std::make_shared<Payload>();
static std::mutex g_m;
static long g_counter = 0;

static void hammer(int iters) {
  for (int i = 0; i < iters; ++i) {
    std::shared_ptr<Payload> c = g_sp;
    std::weak_ptr<Payload> w = c;
    if (w.lock()->v != 7) say("BAD-VALUE\n");
    std::lock_guard g(g_m);
    ++g_counter;
  }
}

// 2. its constructor starts threads (before main)
struct StartsThreads {
  StartsThreads() {
    if (!in_child()) return;
    g_m.lock();  // held while the threads start
    std::vector<std::thread> ts;
    for (int k = 0; k < 4; ++k) ts.emplace_back(hammer, 5000);
    g_m.unlock();
    hammer(5000);
    for (auto& t : ts) t.join();
    if (g_counter == 25000 && g_sp.use_count() == 1) say("init-ok\n");
    else say("init-BAD\n");
  }
  // 4. destroyed after main and after `late`; threads again, with a condition variable
  ~StartsThreads() {
    if (!in_child()) return;
    std::mutex m;
    std::condition_variable cv;
    int turn = 0;
    std::thread t([&] {
      for (int i = 0; i < 100; ++i) {
        std::unique_lock l(m);
        cv.wait(l, [&] { return turn % 2 == 1; });
        ++turn;
        cv.notify_one();
      }
    });
    for (int i = 0; i < 100; ++i) {
      std::unique_lock l(m);
      cv.wait(l, [&] { return turn % 2 == 0; });
      ++turn;
      cv.notify_one();
    }
    t.join();
    hammer(1000);
    if (turn == 200 && g_counter == 25000 + 2 * 1000 + 1000) say("exit-ok\n");
    else say("exit-BAD\n");
  }
};
static StartsThreads g_starts;

// 3. constructed after g_starts, destroyed before it: also uses threads at exit
struct Late {
  ~Late() {
    if (!in_child()) return;
    std::thread t(hammer, 1000);
    hammer(1000);
    t.join();
    say("late-ok\n");
  }
};
static Late g_late;

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  if (in_child()) {
    say("main\n");
    return 0;
  }
  ChildResult r = run_self("child");
  CHECK(r.status == 0);
  CHECK(same_text(r.out, "init-ok\nmain\nlate-ok\nexit-ok\n", "child stdout"));
  CHECK(r.err.empty());
}
