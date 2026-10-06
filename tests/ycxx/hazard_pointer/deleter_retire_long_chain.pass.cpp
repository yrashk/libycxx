// Chains of millions of objects, each reclaimed by a deleter that retires the next one.
//   [saferecl.hp.base]/7 retire: "Move-assigns d to deleter, thereby setting it as the deleter of
//     x, then retires x. May reclaim possibly-reclaimable objects." [saferecl.hp.general]/5: a
//     retired object is reclaimed "by invoking its deleter with a pointer to x". The deleter is
//     program code and nothing in [saferecl.hp] limits what it does, so it may retire further
//     objects, and nothing limits how many objects such retirements lead to: a chain of
//     2,000,000 objects, each retired by the deleter of the previous one, is valid, and its
//     reclamation must not need resources (stack depth, memory) proportional to its length.
//   [saferecl.hp.general]/4: each object is retired at most once; /5-6 (6.1 "x is not
//     reclaimed"): a retired object is reclaimed by invoking its own deleter, so at most once.
//     /7: "The number of possibly-reclaimable objects has an unspecified bound", so retiring more
//     objects eventually reclaims every retired, unprotected object: driving reclamation by
//     retiring other objects ("fillers") reclaims the whole chain. The bound is a function of
//     the numbers of hazard pointers and threads (Note 3), not of the number of objects retired:
//     a chain whose every link is possibly-reclaimable as soon as it is retired must not pile up.
//   [saferecl.hp.general]/6.3.1 and [saferecl.hp.holder.mem]/2-5,9: a link protected by a hazard
//     pointer since before it was retired (here, before the chain's head was retired, which
//     happens before every retirement in the chain) is not reclaimed until the protection epoch
//     ends; the chain stops there (its next link is retired only by that link's deleter), and goes
//     on to the end once the epoch ends.
//   [saferecl.hp.base]/1,3: the deleters are stateful (Cpp17DefaultConstructible,
//     Cpp17MoveAssignable function objects); each link is reclaimed with the deleter its own
//     retire set (/7).
#include <hazard_pointer>
#include <atomic>
#include <cstdint>
#include "check.hpp"
#include "watchdog.hpp"

constexpr std::uint32_t N = 2'000'000;

static std::uint32_t key_of(std::uint32_t i) { return i * 2654435761u ^ 0x5bd1e995u; }

// Fillers: heap objects retired only to drive reclamation.
struct Filler;
struct FillerDeleter {
  void operator()(Filler* p) const noexcept;
};
struct Filler : std::hazard_pointer_obj_base<Filler, FillerDeleter> {};
static std::atomic<long> fillers_live{0};
void FillerDeleter::operator()(Filler* p) const noexcept {
  delete p;
  fillers_live.fetch_sub(1);
}
// Retires fillers until done(); false if that took more than `budget` retirements.
template<class Done> static bool drive(Done done, long budget) {
  for (long i = 0; !done(); ++i) {
    if (i == budget) return false;
    fillers_live.fetch_add(1);
    (new Filler)->retire();
  }
  return true;
}

// Phase 1: a chain built in advance; link M is protected before the head is retired.
struct Link;
struct LinkDeleter {
  std::uint32_t key = 0;
  void operator()(Link* p) const noexcept;
};
struct Link : std::hazard_pointer_obj_base<Link, LinkDeleter> {
  std::uint32_t index = 0;
  Link* next = nullptr;
};
static std::atomic<std::uint8_t> link_reclaimed[N];  // per link: times its deleter ran
static std::atomic<std::uint32_t> links_reclaimed{0};
static std::atomic<long> links_live{0};
static std::atomic<int> protected_index{-1};  // the link a hazard pointer protects, if any
static std::atomic<int> errors{0};
void LinkDeleter::operator()(Link* p) const noexcept {
  const std::uint32_t i = p->index;
  if (key != key_of(i)) errors.fetch_add(1);                          // not its own deleter
  if (link_reclaimed[i].fetch_add(1) != 0) errors.fetch_add(1);       // reclaimed twice
  if (protected_index.load() == int(i)) errors.fetch_add(1);          // reclaimed while protected
  if (Link* n = p->next) n->retire(LinkDeleter{key_of(n->index)});    // retire from a deleter
  delete p;
  links_live.fetch_sub(1);
  links_reclaimed.fetch_add(1);
}

// Phase 2: a chain made as it is reclaimed: each deleter makes the next link and retires it, so
// every link exists only between its retirement and its reclamation.
struct Lazy;
struct LazyDeleter {
  std::uint32_t key = 0;
  void operator()(Lazy* p) const noexcept;
};
struct Lazy : std::hazard_pointer_obj_base<Lazy, LazyDeleter> {
  std::uint32_t index = 0;
};
static std::atomic<std::uint8_t> lazy_reclaimed[N];
static std::atomic<std::uint32_t> lazies_reclaimed{0};
static std::atomic<long> lazies_live{0}, lazies_live_max{0};
static void retire_lazy(std::uint32_t i) {
  Lazy* l = new Lazy;
  l->index = i;
  const long live = lazies_live.fetch_add(1) + 1;
  long m = lazies_live_max.load();
  while (live > m && !lazies_live_max.compare_exchange_weak(m, live)) {}
  l->retire(LazyDeleter{key_of(i)});
}
void LazyDeleter::operator()(Lazy* p) const noexcept {
  const std::uint32_t i = p->index;
  if (key != key_of(i)) errors.fetch_add(1);
  if (lazy_reclaimed[i].fetch_add(1) != 0) errors.fetch_add(1);
  delete p;
  lazies_live.fetch_sub(1);
  lazies_reclaimed.fetch_add(1);
  if (i + 1 < N) retire_lazy(i + 1);
}

int main() {
  watchdog(150);

  // Phase 1.
  constexpr std::uint32_t M = N / 2;
  Link* head = nullptr;
  Link* protected_link = nullptr;
  for (std::uint32_t i = N; i-- > 0;) {
    Link* l = new Link;
    l->index = i;
    l->next = head;
    head = l;
    if (i == M) protected_link = l;
  }
  links_live.store(N);
  std::atomic<Link*> src{protected_link};
  std::hazard_pointer hp = std::make_hazard_pointer();
  CHECK(hp.protect(src) == protected_link);
  protected_index.store(int(M));
  head->retire(LinkDeleter{key_of(0)});
  // Links 0 .. M-1 are reclaimed; link M is retired (by link M-1's deleter) but protected.
  CHECK(drive([] { return links_reclaimed.load() >= M; }, 4L * N));
  CHECK(drive([] { return false; }, 20000) == false);  // more reclamation opportunities
  CHECK(links_reclaimed.load() == M);
  CHECK(link_reclaimed[M].load() == 0);
  CHECK(protected_link->index == M && protected_link->next != nullptr);  // still alive
  CHECK(errors.load() == 0);
  // End the protection epoch: the rest of the chain is reclaimed.
  protected_index.store(-1);
  src.store(nullptr);
  hp.reset_protection();
  CHECK(drive([] { return links_reclaimed.load() == N; }, 4L * N));
  CHECK(links_live.load() == 0);
  for (std::uint32_t i = 0; i < N; ++i) CHECK(link_reclaimed[i].load() == 1);
  CHECK(errors.load() == 0);

  // Phase 2.
  retire_lazy(0);
  CHECK(drive([] { return lazies_reclaimed.load() == N; }, 4L * N));
  CHECK(lazies_live.load() == 0);
  for (std::uint32_t i = 0; i < N; ++i) CHECK(lazy_reclaimed[i].load() == 1);
  CHECK(errors.load() == 0);
  // At most one link of this chain is retired and not reclaimed at a time when every deleter
  // runs as soon as possible; however reclamation is scheduled, the number pending is bounded
  // ([saferecl.hp.general]/7) independently of the chain's length.
  CHECK(lazies_live_max.load() < long(N / 4));
  return 0;
}
