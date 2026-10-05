// [util.smartptr.shared.general]/4: "For purposes of determining the presence of a data race,
// member functions shall access and modify only the shared_ptr and weak_ptr objects
// themselves and not objects they refer to. Changes in use_count() do not reflect
// modifications that can introduce data races." So distinct shared_ptr/weak_ptr objects
// sharing one control block may be copied, destroyed and locked concurrently; the object is
// destroyed exactly once, and lock() yields either an empty or a valid owner.
// FLAGS: -pthread
#include <atomic>
#include <memory>
#include <pthread.h>
#include <sched.h>
#include "check.hpp"

struct Payload {
  static inline std::atomic<int> dtors{0};
  int value = 42;
  ~Payload() {
    value = -1;
    ++dtors;
  }
};

static std::shared_ptr<Payload>* source;
static std::weak_ptr<Payload>* weak_source;
static std::atomic<int> go{0};
static std::atomic<int> bad_locks{0};

static void* copier(void*) {
  std::shared_ptr<Payload> local = *source;  // copy made before go: source not modified after
  while (!go) sched_yield();
  for (int i = 0; i < 20000; ++i) {
    std::shared_ptr<Payload> a = local;
    std::shared_ptr<Payload> b = a;
    std::weak_ptr<Payload> w = b;
    if (w.lock()->value != 42) bad_locks = 1;
  }
  return nullptr;
}

static void* locker(void*) {
  std::weak_ptr<Payload> w = *weak_source;
  while (!go) sched_yield();
  for (int i = 0; i < 20000; ++i) {
    if (std::shared_ptr<Payload> s = w.lock()) {
      if (s->value != 42) bad_locks = 1;
    }
  }
  return nullptr;
}

int main() {
  constexpr int N = 4;
  {
    auto sp = std::make_shared<Payload>();
    std::weak_ptr<Payload> wp = sp;
    source = &sp;
    weak_source = &wp;
    pthread_t th[2 * N];
    for (int i = 0; i < N; ++i) CHECK(pthread_create(&th[i], nullptr, copier, nullptr) == 0);
    for (int i = N; i < 2 * N; ++i) CHECK(pthread_create(&th[i], nullptr, locker, nullptr) == 0);
    // give the threads time to take their copies, then drop the main owner mid-flight
    for (int i = 0; i < 100; ++i) sched_yield();
    go = 1;
    for (int i = 0; i < 1000; ++i) sched_yield();
    for (int i = 0; i < 2 * N; ++i) CHECK(pthread_join(th[i], nullptr) == 0);
    CHECK(sp.use_count() == 1);
    CHECK(Payload::dtors == 0);
    sp.reset();
    CHECK(Payload::dtors == 1);
    CHECK(wp.expired());
  }
  CHECK(bad_locks == 0);

  // last owners released concurrently: exactly one destruction
  for (int round = 0; round < 50; ++round) {
    int before = Payload::dtors;
    auto* holders = new std::shared_ptr<Payload>[N];
    holders[0] = std::make_shared<Payload>();
    for (int i = 1; i < N; ++i) holders[i] = holders[0];
    go = 0;
    pthread_t th[N];
    for (int i = 0; i < N; ++i)
      CHECK(pthread_create(&th[i], nullptr, [](void* h) -> void* {
              while (!go) sched_yield();
              static_cast<std::shared_ptr<Payload>*>(h)->reset();
              return nullptr;
            }, &holders[i]) == 0);
    go = 1;
    for (int i = 0; i < N; ++i) CHECK(pthread_join(th[i], nullptr) == 0);
    CHECK(Payload::dtors == before + 1);
    delete[] holders;
  }
  return 0;
}
