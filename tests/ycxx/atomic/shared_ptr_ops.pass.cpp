// [util.smartptr.atomic.shared]: atomic<shared_ptr<T>>.
// /1: "constexpr atomic() noexcept; Effects: Value-initializes p." (an empty shared_ptr);
// atomic(nullptr_t) delegates to it; /2: atomic(shared_ptr<T> desired) "Initializes the object
// with the value desired". /4: store "Atomically replaces the value pointed to by this with the
// value of desired as if by p.swap(desired)" (so the old value's ownership is released, and the
// stored object shares ownership with the argument); /5-6: operator=(shared_ptr<T>) and
// operator=(nullptr_t) are store(desired) / store(nullptr); /9: load "Atomically returns p";
// /10: the conversion is load(); /11-12: exchange returns the value of p immediately before.
// Every overload with explicit memory orders that the preconditions allow (/3, /7).
#include <memory>
#include <atomic>
#include "check.hpp"

struct Counted {
  static inline int live = 0;
  int v;
  explicit Counted(int x) : v(x) { ++live; }
  ~Counted() { --live; }
};

int main() {
  using SP = std::shared_ptr<Counted>;
  {
    std::atomic<SP> a;
    CHECK(a.load() == nullptr);
    CHECK(a.load().use_count() == 0);  // value-initialized: empty
    std::atomic<SP> n(nullptr);
    CHECK(static_cast<SP>(n) == nullptr);

    SP s = std::make_shared<Counted>(1);
    std::atomic<SP> b(s);
    CHECK(s.use_count() == 2);  // b's p shares ownership with s
    SP l = b.load();
    CHECK(l == s && s.use_count() == 3);
    l.reset();
    CHECK(b.load(std::memory_order::relaxed) == s);
    CHECK(b.load(std::memory_order::acquire) == s);
    CHECK(static_cast<SP>(b).get() == s.get());

    // store releases the old ownership (as if by p.swap(desired), desired then destroyed)
    SP t = std::make_shared<Counted>(2);
    b.store(t);
    CHECK(s.use_count() == 1);
    CHECK(t.use_count() == 2);
    CHECK(b.load()->v == 2);
    b.store(SP(s), std::memory_order::release);
    CHECK(t.use_count() == 1 && s.use_count() == 2);
    b.store(std::move(t), std::memory_order::relaxed);
    CHECK(t == nullptr);  // moved from: by-value parameter
    CHECK(b.load()->v == 2);
    CHECK(b.load().use_count() == 2);  // the stored one and this temporary
    CHECK(s.use_count() == 1);

    // the only owner of an object is the atomic: storing over it destroys the object
    b = std::make_shared<Counted>(3);
    CHECK(Counted::live == 2);  // s's object and the new one
    CHECK(b.load()->v == 3);
    b = nullptr;
    CHECK(Counted::live == 1);
    CHECK(b.load() == nullptr && b.load().use_count() == 0);

    // exchange returns the previous value with its ownership
    b = s;
    SP old = b.exchange(std::make_shared<Counted>(4));
    CHECK(old == s && s.use_count() == 2);  // s and old; b no longer owns it
    old = b.exchange(nullptr, std::memory_order::acq_rel);
    CHECK(old->v == 4 && old.use_count() == 1);
    old.reset();
    CHECK(Counted::live == 1);
    CHECK(b.exchange(SP(), std::memory_order::relaxed) == nullptr);

    // an aliasing shared_ptr keeps its stored pointer and ownership
    struct Two { int x = 10, y = 20; };
    auto two = std::make_shared<Two>();
    std::atomic<std::shared_ptr<int>> ai(std::shared_ptr<int>(two, &two->y));
    CHECK(*ai.load() == 20);
    CHECK(two.use_count() == 2);
    ai = nullptr;
    CHECK(two.use_count() == 1);

    // shared_ptr<const T> and to an array
    std::atomic<std::shared_ptr<const int>> ac(std::make_shared<const int>(7));
    CHECK(*ac.load() == 7);
    std::atomic<std::shared_ptr<int[]>> aa(std::make_shared<int[]>(3, 5));
    CHECK(aa.load()[2] == 5);
  }
  CHECK(Counted::live == 0);  // the atomics release what they own when destroyed
  return 0;
}
