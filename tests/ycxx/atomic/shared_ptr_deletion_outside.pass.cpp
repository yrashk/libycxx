// [util.smartptr.atomic.general]/2: "Associated use_count decrements are sequenced after the
// atomic operation, but are not required to be part of it. Any associated deletion and
// deallocation are sequenced after the atomic update step and are not part of the atomic
// operation." Note 2: locks "will not be held when any destruction or deallocation resulting
// from this is performed". So a deleter run because store / operator= released the atomic's
// last share of the old value may itself use the same atomic object (no deadlock), and it
// observes the new value. [util.smartptr.atomic.shared]/17: a failed compare-exchange assigns
// p to expected ("The write to expected itself is not required to be part of the atomic
// operation"); when expected held the last share of its object, that object's deleter may use
// the atomic too.
// FLAGS: -pthread
#include <memory>
#include <atomic>
#include "check.hpp"
#include "watchdog.hpp"

static std::atomic<std::shared_ptr<int>>* the_atomic;
static int* seen = nullptr;
static int deletions = 0;

struct Del {
  void operator()(int* p) const {
    ++deletions;
    if (!the_atomic) { delete p; return; }  // not while the atomic is being destroyed
    seen = the_atomic->load().get();  // the atomic update has already happened
    std::shared_ptr<int> e;           // a read-modify-write attempt on the same atomic
    the_atomic->compare_exchange_strong(e, nullptr);  // fails unless the atomic is empty
    the_atomic->notify_all();
    delete p;
  }
};

static std::shared_ptr<int> make(int v) { return std::shared_ptr<int>(new int(v), Del{}); }

int main() {
  watchdog(10);
  std::atomic<std::shared_ptr<int>> a(make(1));
  the_atomic = &a;

  auto n2 = make(2);
  a.store(n2);  // a was the only owner of 1
  CHECK(deletions == 1 && seen == n2.get());

  auto n3 = make(3);
  n2.reset();  // a is now the only owner of 2
  a = n3;
  CHECK(deletions == 2 && seen == n3.get());

  n3.reset();  // a is the only owner of 3
  a.store(make(4), std::memory_order::release);
  CHECK(deletions == 3 && *seen == 4);

  // a failed compare_exchange releases expected's previous value (its last owner)
  auto x = make(5);
  int* const four = a.load().get();
  CHECK(!a.compare_exchange_strong(x, make(6)));
  CHECK(deletions == 5);  // 5 (expected's old value) and 6 (the unused desired)
  CHECK(seen == four && x.get() == four);
  x.reset();

  a.store(nullptr);  // the last owner of 4
  CHECK(deletions == 6 && seen == nullptr);
  the_atomic = nullptr;
  return 0;
}
