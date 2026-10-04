// [atomics.types.operations]/23: "If and only if the comparison is false then, after the
// atomic operation, the value in expected is replaced by the value pointed to by this during
// the atomic comparison." Both the single-order and the two-order overloads, all valid orders
// (with one order, acq_rel / release are replaced for the failure path); /24 Returns: the
// result of the comparison. Weak forms may fail spuriously, but then keep expected's value (/27).
#include <atomic>
#include "check.hpp"

int main() {
  const std::memory_order orders[] = {std::memory_order::relaxed, std::memory_order::acquire,
                                      std::memory_order::release, std::memory_order::acq_rel,
                                      std::memory_order::seq_cst};
  for (auto o : orders) {
    std::atomic<long> a(10);
    long e = 3;
    CHECK(!a.compare_exchange_strong(e, 20, o));
    CHECK(e == 10);
    CHECK(a.load() == 10);
    CHECK(a.compare_exchange_strong(e, 20, o));
    CHECK(e == 10);
    CHECK(a.load() == 20);
    e = 3;
    CHECK(!a.compare_exchange_weak(e, 30, o));
    CHECK(e == 20);
    int n = 0;
    while (!a.compare_exchange_weak(e, 30, o)) { CHECK(e == 20); CHECK(++n < 1000); }
    CHECK(a.load() == 30);
  }
  const std::memory_order failures[] = {std::memory_order::relaxed, std::memory_order::acquire,
                                        std::memory_order::seq_cst};
  for (auto s : orders)
    for (auto f : failures) {
      std::atomic<unsigned> a(1);
      unsigned e = 0;
      CHECK(!a.compare_exchange_strong(e, 2, s, f));
      CHECK(e == 1);
      CHECK(a.compare_exchange_strong(e, 2, s, f));
      CHECK(a.load() == 2);
    }
  // the list-insertion idiom of Example 2: expected updated only on failure
  struct Node { Node* next; };
  Node n1{nullptr}, n2{nullptr};
  std::atomic<Node*> head(&n1);
  Node* p = &n2;
  do {
    p->next = head;
  } while (!head.compare_exchange_weak(p->next, p));
  CHECK(head.load() == &n2 && n2.next == &n1);
  return 0;
}
