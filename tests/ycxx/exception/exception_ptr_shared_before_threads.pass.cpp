// exception_ptr objects made while the program has a single thread (make_exception_ptr and
// current_exception) and then copied, compared, rethrown and destroyed by several threads at
// once, the last reference going away in one of those threads.
// [propagation]/7: "Changes in the number of exception_ptr objects that refer to a particular
// exception object do not introduce a data race"; "The referenced object remains valid at
// least as long as there is an exception_ptr object that refers to it"; /3: two
// exception_ptrs compare equal iff both are null or both point to the same exception object;
// /10 rethrow_exception throws the referenced object or a copy. Every object constructed is
// destroyed exactly once (counted by construction/destruction balance), and only after the
// last exception_ptr is gone.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <atomic>
#include <exception>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

struct Obj {
  static inline std::atomic<int> live{0};
  int value;
  explicit Obj(int v) : value(v) { ++live; }
  Obj(const Obj& o) : value(o.value) { ++live; }
  ~Obj() {
    value = -1;
    --live;
  }
};

constexpr int K = 4;
static std::atomic<int> failures{0};
static std::atomic<int> ready{0};
static std::atomic<bool> main_dropped{false};

static void use(std::vector<std::exception_ptr> mine, int expect) {
  ++ready;
  for (int i = 0; i < 4000; ++i) {
    std::exception_ptr a = mine[static_cast<std::size_t>(i) % mine.size()];
    std::exception_ptr b = a;
    if (!(a == b) || a == nullptr) ++failures;
    if (i % 8 == 0) {
      try {
        std::rethrow_exception(b);
      } catch (const Obj& o) {
        if (o.value != expect) ++failures;
      } catch (...) {
        ++failures;
      }
    }
    if (i == 2000) {
      // wait until main has dropped all its references: from now on only the threads own it
      while (!main_dropped.load()) std::this_thread::yield();
    }
  }
}

int main() {
  watchdog(50);
  std::exception_ptr made = std::make_exception_ptr(Obj(1));
  std::exception_ptr caught;
  try {
    throw Obj(2);
  } catch (...) {
    caught = std::current_exception();
  }
  CHECK(made != caught);
  CHECK(Obj::live.load() >= 2);
  std::vector<std::exception_ptr> copies1(8, made), copies2(8, caught);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k) {
    if (k % 2) ts.emplace_back(use, copies1, 1);
    else ts.emplace_back(use, copies2, 2);
  }
  while (ready.load() < K) std::this_thread::yield();
  // main keeps using its copies concurrently for a while, then drops all of them
  for (int i = 0; i < 2000; ++i) {
    std::exception_ptr x = copies1[static_cast<std::size_t>(i % 8)];
    std::exception_ptr y = x;
    CHECK(x == y && x == made);
  }
  copies1.clear();
  copies2.clear();
  made = nullptr;
  caught = nullptr;
  CHECK(Obj::live.load() >= 2);  // the threads still refer to both objects
  main_dropped = true;
  for (auto& t : ts) t.join();
  CHECK(failures.load() == 0);
  CHECK(Obj::live.load() == 0);  // destroyed (exactly once each) with the last reference

  // and again, created after threads have existed
  {
    std::exception_ptr p = std::make_exception_ptr(Obj(3));
    std::thread t([q = p] {
      try {
        std::rethrow_exception(q);
      } catch (const Obj& o) {
        CHECK(o.value == 3);
      }
    });
    p = nullptr;
    t.join();
  }
  CHECK(Obj::live.load() == 0);
}
