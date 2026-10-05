// Deleters of reclaimed objects that retire further objects and that create hazard pointers and
// protect objects themselves.
//   [saferecl.hp.base]/7 retire: "Move-assigns d to deleter, thereby setting it as the deleter of
//     x, then retires x. May reclaim possibly-reclaimable objects." Reclamation invokes the
//     deleter ([saferecl.hp.general]/5), which is program code; nothing restricts what it does
//     with hazard pointers, so a deleter (and so the destructor of a hazard-protectable object)
//     may retire other objects, make hazard pointers and protect objects. Since any retire may
//     run any deleter, retire is necessarily called while another retire is active: a library
//     that runs deleters inside retire supports that ([reentrancy]/1 lets the implementation say
//     which functions may be recursively reentered; this one cannot be excluded).
//   [saferecl.hp.general]/6-7: an object retired and not protected is possibly-reclaimable;
//     "The number of possibly-reclaimable objects has an unspecified bound", so retiring further
//     objects eventually reclaims every unprotected retired object, including those retired by
//     deleters. /4: each object is retired at most once; /5 reclaimed by invoking its deleter
//     (once, [saferecl.hp.general]/6.1).
//   [saferecl.hp.holder.mem]: an object protected by a hazard pointer since before it was
//     retired is not reclaimed during that protection epoch, also when the hazard pointer was
//     made, and the object retired, inside a deleter.
// FLAGS: -pthread
#include <hazard_pointer>
#include <atomic>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

constexpr int alive = 0x5eed, dead = 0xdead;

// A chain: reclaiming link k retires link k + 1 (from its deleter).
struct Link;
struct LinkDeleter {
  void operator()(Link* p) const noexcept;
};
struct Link : std::hazard_pointer_obj_base<Link, LinkDeleter> {
  std::atomic<int> magic{alive};
  std::atomic<int> reclaimed{0};
  Link* next = nullptr;
};
void LinkDeleter::operator()(Link* p) const noexcept {
  p->magic.store(dead);
  p->reclaimed.fetch_add(1);
  if (p->next) p->next->retire();
}

// Plain objects retired to drive reclamation.
struct Filler;
struct FillerDeleter {
  void operator()(Filler* p) const noexcept;
};
struct Filler : std::hazard_pointer_obj_base<Filler, FillerDeleter> {
  std::atomic<int> reclaimed{0};
};
void FillerDeleter::operator()(Filler* p) const noexcept { p->reclaimed.fetch_add(1); }

constexpr int ChainLen = 20000;
constexpr int Fillers = 400000;
static Link chain[ChainLen];
static Filler fillers[Fillers];
static std::atomic<int> next_filler{0};  // (deleters may run on any thread)

static bool all_reclaimed(const Link* first, int n) {
  for (int i = 0; i < n; ++i)
    if (first[i].reclaimed.load() != 1) return false;
  return true;
}
// Retires fillers until done() or none are left.
template<class Done> static bool drive(Done done) {
  while (!done()) {
    const int k = next_filler.fetch_add(1);
    if (k >= Fillers) return false;
    fillers[k].retire();
  }
  return true;
}

// Protector: its deleter makes a hazard pointer, protects a live Link through an atomic
// source, retires that Link, retires more fillers, and checks the Link survives while the
// protection lasts.
struct Protector;
struct ProtectorDeleter {
  void operator()(Protector* p) const noexcept;
};
struct Protector : std::hazard_pointer_obj_base<Protector, ProtectorDeleter> {
  std::atomic<Link*>* src = nullptr;
  std::atomic<int> result{0};  // 1: checks passed; -1: failed
};
void ProtectorDeleter::operator()(Protector* p) const noexcept {
  std::hazard_pointer h = std::make_hazard_pointer();
  Link* l = h.protect(*p->src);
  p->src->store(nullptr);
  l->retire();
  bool ok = true;
  for (int i = 0; i < 3000; ++i) {
    const int k = next_filler.fetch_add(1);
    if (k >= Fillers) break;
    fillers[k].retire();
    ok = ok && l->magic.load() == alive && l->reclaimed.load() == 0;
  }
  h.reset_protection();
  p->result.store(ok ? 1 : -1);
}

int main() {
  watchdog(20);
  // The chain reclaims itself link by link.
  for (int i = 0; i + 1 < ChainLen; ++i) chain[i].next = &chain[i + 1];
  chain[0].retire();
  CHECK(drive([] { return all_reclaimed(chain, ChainLen); }));

  // Deleters that protect.
  static Link protected_links[50];
  static Protector protectors[50];
  static std::atomic<Link*> srcs[50];
  for (int i = 0; i < 50; ++i) {
    srcs[i].store(&protected_links[i]);
    protectors[i].src = &srcs[i];
    protectors[i].retire();
  }
  CHECK(drive([] {
    for (auto& p : protectors)
      if (p.result.load() == 0) return false;
    return true;
  }));
  for (auto& p : protectors) CHECK(p.result.load() == 1);
  CHECK(drive([] { return all_reclaimed(protected_links, 50); }));

  // Concurrent readers protect the head of a list; the writer retires old heads whose deleters
  // retire a further private chain each.
  constexpr int Heads = 3000, Tail = 5;
  static Link heads[Heads];
  static Link tails[Heads][Tail];
  for (int i = 0; i < Heads; ++i) {
    heads[i].next = &tails[i][0];
    for (int k = 0; k + 1 < Tail; ++k) tails[i][k].next = &tails[i][k + 1];
  }
  std::atomic<Link*> current{&heads[0]};
  std::atomic<bool> stop{false};
  std::atomic<long> bad{0}, reads{0};
  std::vector<std::thread> readers;
  for (int t = 0; t < 3; ++t)
    readers.emplace_back([&] {
      std::hazard_pointer hp = std::make_hazard_pointer();
      while (!stop.load()) {
        Link* n = hp.protect(current);
        for (int spin = 0; spin < 20; ++spin)
          if (n->magic.load() != alive) bad.fetch_add(1);
        hp.reset_protection();
        reads.fetch_add(1);
      }
    });
  while (reads.load() < 3) std::this_thread::yield();
  for (int i = 1; i < Heads; ++i) current.exchange(&heads[i])->retire();
  stop.store(true);
  for (auto& th : readers) th.join();
  CHECK(bad.load() == 0);
  current.store(nullptr);
  heads[Heads - 1].retire();
  CHECK(drive([] {
    if (!all_reclaimed(heads, Heads)) return false;
    for (auto& t : tails)
      if (!all_reclaimed(t, Tail)) return false;
    return true;
  }));
  for (auto& f : fillers) CHECK(f.reclaimed.load() <= 1);
  return 0;
}
