// Library objects created, copied and locked while the program still has a single thread keep
// their full guarantees once further threads start and use them:
// [util.smartptr.shared.general]/4 (shared_ptr/weak_ptr copies and destruction from several
// threads; the owned object is destroyed exactly once, [util.smartptr.shared.dest]),
// [util.smartptr.weak.obs] (lock() gives an owner or an empty pointer, expired()),
// [thread.mutex.requirements.mutex.general]/15,/22 ("If one thread owns a mutex object,
// attempts by another thread to acquire ownership of that object will fail (for try_lock())
// or block (for lock())"; /6: lock and unlock synchronize), [thread.mutex.recursive]/3 (another
// thread acquires a recursive_mutex only when all levels are released),
// [thread.condition.condvar] (wait/notify), [locale.cons]/[locale.facet]/2 (a facet with refs 0
// is deleted when the last locale referring to it is destroyed; [res.on.data.races]: copying
// and destroying distinct locale objects from several threads is not a data race),
// [facet.num.put.virtuals] (formatting follows the imbued numpunct in every thread), and
// [mem.res.pool.overview]/2 (a synchronized_pool_resource "may be accessed from multiple threads
// without external synchronization"; storage allocated by one thread is deallocated by another).
// The first thread of each child process is created by a different facility: std::thread,
// std::jthread, std::async(launch::async) and POSIX pthread_create.
// FLAGS: -pthread
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <future>
#include <locale>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <pthread.h>
#include "child_process.hpp"
#include "check.hpp"
#include "watchdog.hpp"

constexpr int K = 4;          // worker threads
constexpr int ITERS = 20000;  // hammer iterations per thread

struct Payload {
  static inline std::atomic<int> dtors{0};
  int value = 42;
  ~Payload() {
    value = -1;
    ++dtors;
  }
};

struct Tag : std::locale::facet {
  static std::locale::id id;
  static inline std::atomic<int> dtors{0};
  int v;
  explicit Tag(int x) : v(x) {}
  ~Tag() override { ++dtors; }
};
std::locale::id Tag::id;

struct Commas : std::numpunct<char> {
  char do_thousands_sep() const override { return ','; }
  std::string do_grouping() const override { return "\3"; }
};

// ---- the facility that starts threads in this child ----
static const char* mode;
static std::vector<std::thread> threads;
static std::vector<std::jthread> jthreads;
static std::vector<std::future<void>> futures;
static std::vector<pthread_t> pthreads;
static void (*pthread_body[16])(int);
static void* pthread_tramp(void* p) {
  int k = static_cast<int>(reinterpret_cast<std::intptr_t>(p));
  pthread_body[k](k);
  return nullptr;
}

static void spawn(void (*f)(int), int k) {
  if (std::strcmp(mode, "thread") == 0) threads.emplace_back(f, k);
  else if (std::strcmp(mode, "jthread") == 0) jthreads.emplace_back(f, k);
  else if (std::strcmp(mode, "async") == 0) futures.push_back(std::async(std::launch::async, f, k));
  else {
    pthread_body[k] = f;
    pthread_t t;
    CHECK(pthread_create(&t, nullptr, pthread_tramp, reinterpret_cast<void*>(static_cast<std::intptr_t>(k))) == 0);
    pthreads.push_back(t);
  }
}
static void join_all() {
  for (auto& t : threads) t.join();
  for (auto& t : jthreads) t.join();
  for (auto& f : futures) f.get();
  for (auto& t : pthreads) CHECK(pthread_join(t, nullptr) == 0);
  threads.clear();
  jthreads.clear();
  futures.clear();
  pthreads.clear();
}

// ---- objects created before the first thread ----
static std::shared_ptr<Payload>* sp;
static std::shared_ptr<Payload>* sp_copies;  // one per worker + one for main
static std::weak_ptr<Payload>* wp;
static std::mutex* m;
static std::recursive_mutex* rm;
static std::condition_variable* cv;
static std::mutex* cv_m;
static int token = 0;
static std::locale* tagged;
static std::locale* commas;
static std::pmr::synchronized_pool_resource* pool;
constexpr int PRE_BLOCKS = 64;
static void* pre_blocks[PRE_BLOCKS];
static std::size_t block_size(int i) { return 8u << (i % 7); }

static long counter = 0;   // guarded by *m
static long counter2 = 0;  // guarded by *rm
static std::atomic<int> arrived{0};
static std::atomic<int> phase{0};
static std::atomic<int> failures{0};

static void wait_phase(int p) {
  while (phase.load() < p) std::this_thread::yield();
}

static void hammer(int k) {
  std::shared_ptr<Payload> mine = sp_copies[k];
  std::locale my_loc = *tagged;
  for (int i = 0; i < ITERS; ++i) {
    {
      std::lock_guard g(*m);
      ++counter;
    }
    {
      std::lock_guard g1(*rm);
      std::lock_guard g2(*rm);
      ++counter2;
    }
    std::shared_ptr<Payload> a = mine;
    std::shared_ptr<Payload> b = a;
    std::weak_ptr<Payload> w = b;
    if (auto l = wp->lock(); !l || l->value != 42) ++failures;
    if (w.expired() || w.lock()->value != 42) ++failures;
    if (i % 16 == 0) {
      std::locale l2 = my_loc;
      std::locale l3(l2);
      if (!std::has_facet<Tag>(l3) || std::use_facet<Tag>(l3).v != 7) ++failures;
      my_loc = l3;
    }
    if (i % 64 == 0) {
      std::ostringstream os;
      os.imbue(*commas);
      os << 1234567 + i;
      int v = 1234567 + i;
      char want[32];
      std::snprintf(want, sizeof want, "%d,%03d,%03d", v / 1000000, v / 1000 % 1000, v % 1000);
      if (os.str() != want) ++failures;
    }
    if (i % 8 == 0) {
      std::size_t n = block_size(i + k);
      auto* p = static_cast<unsigned char*>(pool->allocate(n, 8));
      std::memset(p, k + 1, n);
      for (std::size_t j = 0; j < n; ++j)
        if (p[j] != k + 1) ++failures;
      pool->deallocate(p, n, 8);
    }
  }
}

static void worker(int k) {
  // Locks taken before this thread existed are still held.
  if (m->try_lock()) {
    ++failures;
    m->unlock();
  }
  if (rm->try_lock()) {
    ++failures;
    rm->unlock();
  }
  ++arrived;
  wait_phase(1);
  if (rm->try_lock()) {  // main still holds one level
    ++failures;
    rm->unlock();
  }
  ++arrived;
  wait_phase(2);
  {
    std::lock_guard g(*m);  // blocked until main released it
    ++counter;
  }
  // Blocks allocated by main before any thread existed are released here.
  for (int i = k; i < PRE_BLOCKS; i += K) {
    auto* p = static_cast<unsigned char*>(pre_blocks[i]);
    for (std::size_t j = 0; j < block_size(i); ++j)
      if (p[j] != static_cast<unsigned char>(i)) ++failures;
    pool->deallocate(p, block_size(i), 8);
  }
  hammer(k);
  // A token ring through the condition variable created before the threads.
  for (int r = 0; r < 300; ++r) {
    std::unique_lock l(*cv_m);
    cv->wait(l, [&] { return token % (K + 1) == k; });
    ++token;
    cv->notify_all();
  }
  sp_copies[k].reset();  // drop this worker's original copy
}

static int child() {
  watchdog(50);
  auto& payload = *new std::shared_ptr<Payload>(std::make_shared<Payload>());
  sp = &payload;
  sp_copies = new std::shared_ptr<Payload>[K + 1];
  for (int i = 0; i <= K; ++i) sp_copies[i] = payload;
  wp = new std::weak_ptr<Payload>(payload);
  CHECK(payload.use_count() == K + 2);
  m = new std::mutex;
  rm = new std::recursive_mutex;
  cv = new std::condition_variable;
  cv_m = new std::mutex;
  tagged = new std::locale(std::locale::classic(), new Tag(7));
  commas = new std::locale(std::locale::classic(), new Commas);
  pool = new std::pmr::synchronized_pool_resource;
  for (int i = 0; i < PRE_BLOCKS; ++i) {
    pre_blocks[i] = pool->allocate(block_size(i), 8);
    std::memset(pre_blocks[i], i, block_size(i));
  }
  m->lock();
  rm->lock();
  rm->lock();
  rm->lock();

  for (int k = 0; k < K; ++k) spawn(worker, k);
  while (arrived.load() < K) std::this_thread::yield();
  rm->unlock();
  rm->unlock();
  phase = 1;
  while (arrived.load() < 2 * K) std::this_thread::yield();
  rm->unlock();
  m->unlock();
  phase = 2;
  hammer(K);
  for (int r = 0; r < 300; ++r) {
    std::unique_lock l(*cv_m);
    cv->wait(l, [&] { return token % (K + 1) == K; });
    ++token;
    cv->notify_all();
  }
  join_all();

  CHECK(failures.load() == 0);
  CHECK(counter == (K + 1) * static_cast<long>(ITERS) + K);
  CHECK(counter2 == (K + 1) * static_cast<long>(ITERS));
  CHECK(token == 300 * (K + 1));
  CHECK(payload.use_count() == 2);  // payload and sp_copies[K]
  CHECK(Payload::dtors.load() == 0);
  sp_copies[K].reset();
  CHECK(!wp->expired());
  payload.reset();
  CHECK(Payload::dtors.load() == 1);
  CHECK(wp->expired());
  CHECK(wp->lock() == nullptr);
  CHECK(Tag::dtors.load() == 0);
  delete tagged;
  CHECK(Tag::dtors.load() == 1);  // the last locale referring to the facet is gone
  delete commas;
  pool->release();
  delete pool;
  delete m;
  delete rm;
  delete cv;
  delete cv_m;
  delete wp;
  delete[] sp_copies;
  return 0;
}

int main(int argc, char** argv) {
  if (child_mode()) {
    mode = argv[1];
    return child();
  }
  for (const char* md : {"thread", "jthread", "async", "pthread"}) {
    ChildResult r = run_self(md);
    if (r.status != 0) dprintf(2, "mode %s: status %d\n%s", md, r.status, r.err.c_str());
    CHECK(r.status == 0);
  }
  (void)argc;
}
